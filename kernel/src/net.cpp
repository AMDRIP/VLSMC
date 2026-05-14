#include "kernel/net.h"
#include "kernel/spinlock.h"
#include "kernel/timer.h"
#include "libc.h"

namespace re36 {

namespace {

struct ArpEntry {
    bool valid;
    uint32_t ip;
    uint8_t mac[6];
    uint32_t updated_ticks;
};

constexpr int kArpCacheSize = 8;
constexpr int kUdpQueueSize = 8;

bool s_initialized = false;
bool s_has_device = false;
bool s_link_up = false;
char s_driver_name[16];
uint8_t s_mac[6];
uint32_t s_ipv4 = 0;
uint32_t s_netmask = 0;
uint32_t s_gateway = 0;
NetTxFunc s_tx = nullptr;
NetStats s_stats;
ArpEntry s_arp_cache[kArpCacheSize];
NetUdpPacket s_udp_queue[kUdpQueueSize];
uint32_t s_udp_write_index = 0;
uint16_t s_ip_id = 1;

bool s_ping_pending = false;
uint16_t s_ping_id = 0;
uint16_t s_ping_seq = 0;
uint32_t s_ping_src_ip = 0;
uint8_t s_ping_ttl = 0;

const uint8_t kBroadcastMac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint8_t kZeroMac[6] = {0, 0, 0, 0, 0, 0};

uint16_t read_be16(const uint8_t* p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

uint32_t read_be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

void write_be16(uint8_t* p, uint16_t value) {
    p[0] = (uint8_t)(value >> 8);
    p[1] = (uint8_t)value;
}

void write_be32(uint8_t* p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

bool mac_equals(const uint8_t* a, const uint8_t* b) {
    for (int i = 0; i < 6; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void copy_mac(uint8_t* dst, const uint8_t* src) {
    for (int i = 0; i < 6; i++) dst[i] = src[i];
}

uint16_t internet_checksum(const uint8_t* data, uint16_t length) {
    uint32_t sum = 0;

    while (length > 1) {
        sum += read_be16(data);
        data += 2;
        length -= 2;
    }

    if (length) {
        sum += (uint16_t)data[0] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

bool is_same_subnet(uint32_t ip) {
    return (ip & s_netmask) == (s_ipv4 & s_netmask);
}

bool is_local_or_broadcast(uint32_t ip) {
    return ip == s_ipv4 || ip == 0xFFFFFFFF;
}

void copy_name(char* dst, const char* src, int max_len) {
    int i = 0;
    for (; i < max_len - 1 && src && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

void cache_arp(uint32_t ip, const uint8_t* mac) {
    if (ip == 0 || mac_equals(mac, kZeroMac)) return;

    int free_slot = -1;
    int oldest = 0;
    uint32_t oldest_ticks = 0xFFFFFFFF;

    for (int i = 0; i < kArpCacheSize; i++) {
        if (s_arp_cache[i].valid && s_arp_cache[i].ip == ip) {
            copy_mac(s_arp_cache[i].mac, mac);
            s_arp_cache[i].updated_ticks = Timer::get_ticks();
            return;
        }
        if (!s_arp_cache[i].valid && free_slot < 0) {
            free_slot = i;
        }
        if (s_arp_cache[i].valid && s_arp_cache[i].updated_ticks < oldest_ticks) {
            oldest_ticks = s_arp_cache[i].updated_ticks;
            oldest = i;
        }
    }

    int slot = free_slot >= 0 ? free_slot : oldest;
    s_arp_cache[slot].valid = true;
    s_arp_cache[slot].ip = ip;
    copy_mac(s_arp_cache[slot].mac, mac);
    s_arp_cache[slot].updated_ticks = Timer::get_ticks();
}

bool lookup_arp(uint32_t ip, uint8_t* mac) {
    for (int i = 0; i < kArpCacheSize; i++) {
        if (s_arp_cache[i].valid && s_arp_cache[i].ip == ip) {
            copy_mac(mac, s_arp_cache[i].mac);
            return true;
        }
    }
    return false;
}

void push_udp_packet(uint16_t dst_port, uint32_t src_ip, uint16_t src_port,
                     const uint8_t* payload, uint16_t length) {
    if (length > NET_MAX_UDP_PAYLOAD) {
        s_stats.rx_dropped++;
        return;
    }

    NetUdpPacket& pkt = s_udp_queue[s_udp_write_index % kUdpQueueSize];
    pkt.used = true;
    pkt.src_ip = src_ip;
    pkt.src_port = src_port;
    pkt.dst_port = dst_port;
    pkt.length = length;
    if (length > 0) {
        memcpy(pkt.payload, payload, length);
    }
    s_udp_write_index = (s_udp_write_index + 1) % kUdpQueueSize;
}

} // namespace

void NetStack::init() {
    InterruptGuard guard;
    s_initialized = true;
    s_has_device = false;
    s_link_up = false;
    s_tx = nullptr;
    copy_name(s_driver_name, "none", sizeof(s_driver_name));
    memset(s_mac, 0, sizeof(s_mac));
    memset(&s_stats, 0, sizeof(s_stats));
    memset(s_arp_cache, 0, sizeof(s_arp_cache));
    memset(s_udp_queue, 0, sizeof(s_udp_queue));
    s_udp_write_index = 0;
    s_ip_id = 1;
    s_ping_pending = false;

    configure_ipv4((10u << 24) | (0u << 16) | (2u << 8) | 15u,
                   0xFFFFFF00u,
                   (10u << 24) | (0u << 16) | (2u << 8) | 2u);
}

bool NetStack::attach_device(const char* name, const uint8_t* mac, NetTxFunc tx) {
    if (!s_initialized) init();
    if (!tx || !mac) return false;

    InterruptGuard guard;
    s_has_device = true;
    s_link_up = true;
    s_tx = tx;
    copy_mac(s_mac, mac);
    copy_name(s_driver_name, name ? name : "net0", sizeof(s_driver_name));
    return true;
}

bool NetStack::has_device() {
    return s_has_device;
}

bool NetStack::is_link_up() {
    return s_has_device && s_link_up && s_tx != nullptr;
}

void NetStack::configure_ipv4(uint32_t ip, uint32_t mask, uint32_t gateway) {
    s_ipv4 = ip;
    s_netmask = mask;
    s_gateway = gateway;
}

void NetStack::fill_info(NetInfo* out) {
    if (!out) return;
    out->link_up = is_link_up();
    copy_name(out->driver, s_driver_name, sizeof(out->driver));
    copy_mac(out->mac, s_mac);
    out->ipv4_addr = s_ipv4;
    out->netmask = s_netmask;
    out->gateway = s_gateway;
    out->stats = s_stats;
}

const NetStats& NetStack::stats() {
    return s_stats;
}

uint32_t NetStack::parse_ipv4(const char* text, bool* ok) {
    uint32_t octets[4] = {0, 0, 0, 0};
    int part = 0;
    int digits = 0;

    if (ok) *ok = false;
    if (!text) return 0;

    for (const char* p = text; ; p++) {
        char c = *p;
        if (c >= '0' && c <= '9') {
            octets[part] = octets[part] * 10 + (uint32_t)(c - '0');
            if (octets[part] > 255) return 0;
            digits++;
        } else if (c == '.' || c == '\0' || c == ' ') {
            if (digits == 0) return 0;
            digits = 0;
            if (c == '.') {
                part++;
                if (part >= 4) return 0;
            } else {
                if (part != 3) return 0;
                if (ok) *ok = true;
                return (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
            }
        } else {
            return 0;
        }
    }
}

void NetStack::format_ipv4(uint32_t ip, char* out) {
    if (!out) return;
    // Kernel printf already has decimal formatting; keep conversion small and local.
    uint8_t o0 = (uint8_t)(ip >> 24);
    uint8_t o1 = (uint8_t)(ip >> 16);
    uint8_t o2 = (uint8_t)(ip >> 8);
    uint8_t o3 = (uint8_t)ip;

    int pos = 0;
    uint8_t octets[4] = {o0, o1, o2, o3};
    for (int i = 0; i < 4; i++) {
        char tmp[3];
        int n = 0;
        uint8_t v = octets[i];
        if (v >= 100) tmp[n++] = (char)('0' + v / 100);
        if (v >= 10) tmp[n++] = (char)('0' + (v / 10) % 10);
        tmp[n++] = (char)('0' + v % 10);
        for (int j = 0; j < n; j++) out[pos++] = tmp[j];
        if (i != 3) out[pos++] = '.';
    }
    out[pos] = '\0';
}

void NetStack::format_mac(const uint8_t* mac, char* out) {
    static const char hex[] = "0123456789abcdef";
    if (!mac || !out) return;
    int pos = 0;
    for (int i = 0; i < 6; i++) {
        out[pos++] = hex[(mac[i] >> 4) & 0x0F];
        out[pos++] = hex[mac[i] & 0x0F];
        if (i != 5) out[pos++] = ':';
    }
    out[pos] = '\0';
}

bool NetStack::send_ethernet(const uint8_t* dst_mac, uint16_t ethertype,
                             const uint8_t* payload, uint16_t length) {
    if (!is_link_up() || length > NET_MTU) {
        s_stats.tx_dropped++;
        return false;
    }

    uint8_t frame[NET_MAX_FRAME_SIZE];
    copy_mac(frame, dst_mac);
    copy_mac(frame + 6, s_mac);
    write_be16(frame + 12, ethertype);
    if (length > 0) {
        memcpy(frame + 14, payload, length);
    }

    uint16_t frame_len = 14 + length;
    if (frame_len < 60) {
        memset(frame + frame_len, 0, 60 - frame_len);
        frame_len = 60;
    }

    bool ok = s_tx(frame, frame_len);
    if (ok) {
        s_stats.tx_frames++;
        s_stats.tx_bytes += frame_len;
    } else {
        s_stats.tx_dropped++;
    }
    return ok;
}

void NetStack::send_arp_request(uint32_t target_ip) {
    if (!is_link_up() || s_ipv4 == 0) return;

    uint8_t payload[28];
    write_be16(payload + 0, 1);
    write_be16(payload + 2, NET_ETHERTYPE_IPV4);
    payload[4] = 6;
    payload[5] = 4;
    write_be16(payload + 6, 1);
    copy_mac(payload + 8, s_mac);
    write_be32(payload + 14, s_ipv4);
    copy_mac(payload + 18, kZeroMac);
    write_be32(payload + 24, target_ip);

    if (send_ethernet(kBroadcastMac, NET_ETHERTYPE_ARP, payload, sizeof(payload))) {
        s_stats.arp_tx++;
    }
}

void NetStack::send_arp_reply(const uint8_t* dst_mac, uint32_t dst_ip) {
    uint8_t payload[28];
    write_be16(payload + 0, 1);
    write_be16(payload + 2, NET_ETHERTYPE_IPV4);
    payload[4] = 6;
    payload[5] = 4;
    write_be16(payload + 6, 2);
    copy_mac(payload + 8, s_mac);
    write_be32(payload + 14, s_ipv4);
    copy_mac(payload + 18, dst_mac);
    write_be32(payload + 24, dst_ip);

    if (send_ethernet(dst_mac, NET_ETHERTYPE_ARP, payload, sizeof(payload))) {
        s_stats.arp_tx++;
    }
}

bool NetStack::resolve_mac(uint32_t dst_ip, uint8_t* out_mac) {
    if (dst_ip == 0xFFFFFFFF) {
        copy_mac(out_mac, kBroadcastMac);
        return true;
    }

    uint32_t next_hop = is_same_subnet(dst_ip) ? dst_ip : s_gateway;
    if (next_hop == 0) next_hop = dst_ip;

    if (lookup_arp(next_hop, out_mac)) {
        return true;
    }

    send_arp_request(next_hop);
    return false;
}

bool NetStack::send_ipv4(uint32_t dst_ip, uint8_t protocol,
                         const uint8_t* payload, uint16_t length) {
    if (length > NET_MTU - 20 || s_ipv4 == 0) {
        s_stats.tx_dropped++;
        return false;
    }

    uint8_t dst_mac[6];
    if (!resolve_mac(dst_ip, dst_mac)) {
        return false;
    }

    uint8_t packet[NET_MTU];
    memset(packet, 0, 20);
    packet[0] = 0x45;
    packet[1] = 0;
    write_be16(packet + 2, (uint16_t)(20 + length));
    write_be16(packet + 4, s_ip_id++);
    write_be16(packet + 6, 0x4000);
    packet[8] = 64;
    packet[9] = protocol;
    write_be32(packet + 12, s_ipv4);
    write_be32(packet + 16, dst_ip);
    write_be16(packet + 10, internet_checksum(packet, 20));
    if (length > 0) {
        memcpy(packet + 20, payload, length);
    }

    bool ok = send_ethernet(dst_mac, NET_ETHERTYPE_IPV4, packet, 20 + length);
    if (ok) s_stats.ipv4_tx++;
    return ok;
}

bool NetStack::send_udp(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port,
                        const uint8_t* payload, uint16_t length) {
    if (length > NET_MAX_UDP_PAYLOAD) return false;

    uint8_t udp[8 + NET_MAX_UDP_PAYLOAD];
    write_be16(udp + 0, src_port);
    write_be16(udp + 2, dst_port);
    write_be16(udp + 4, (uint16_t)(8 + length));
    write_be16(udp + 6, 0);
    if (length > 0) {
        memcpy(udp + 8, payload, length);
    }

    bool ok = send_ipv4(dst_ip, NET_IP_PROTO_UDP, udp, 8 + length);
    if (ok) s_stats.udp_tx++;
    return ok;
}

int NetStack::recv_udp(uint16_t local_port, uint32_t* src_ip, uint16_t* src_port,
                       uint8_t* payload, uint16_t max_length) {
    for (int i = 0; i < kUdpQueueSize; i++) {
        NetUdpPacket& pkt = s_udp_queue[i];
        if (!pkt.used || pkt.dst_port != local_port) continue;

        uint16_t copy_len = pkt.length < max_length ? pkt.length : max_length;
        if (copy_len > 0 && payload) {
            memcpy(payload, pkt.payload, copy_len);
        }
        if (src_ip) *src_ip = pkt.src_ip;
        if (src_port) *src_port = pkt.src_port;
        pkt.used = false;
        return copy_len;
    }

    return 0;
}

bool NetStack::send_icmp_echo(uint32_t dst_ip, uint16_t id, uint16_t seq) {
    uint8_t icmp[16];
    memset(icmp, 0, sizeof(icmp));
    icmp[0] = 8;
    icmp[1] = 0;
    write_be16(icmp + 4, id);
    write_be16(icmp + 6, seq);
    write_be32(icmp + 8, Timer::get_ticks());
    write_be32(icmp + 12, 0x52453336);
    write_be16(icmp + 2, internet_checksum(icmp, sizeof(icmp)));

    s_ping_pending = false;
    bool ok = send_ipv4(dst_ip, NET_IP_PROTO_ICMP, icmp, sizeof(icmp));
    if (ok) s_stats.icmp_tx++;
    return ok;
}

bool NetStack::consume_ping_reply(uint16_t id, uint16_t seq, uint32_t* src_ip, uint8_t* ttl) {
    if (!s_ping_pending || s_ping_id != id || s_ping_seq != seq) {
        return false;
    }

    if (src_ip) *src_ip = s_ping_src_ip;
    if (ttl) *ttl = s_ping_ttl;
    s_ping_pending = false;
    return true;
}

void NetStack::receive_frame(const uint8_t* frame, uint16_t length) {
    if (!frame || length < 14 || length > NET_MAX_FRAME_SIZE) {
        s_stats.rx_dropped++;
        return;
    }

    s_stats.rx_frames++;
    s_stats.rx_bytes += length;

    const uint8_t* dst = frame;
    if (!mac_equals(dst, s_mac) && !mac_equals(dst, kBroadcastMac)) {
        return;
    }

    uint16_t ethertype = read_be16(frame + 12);
    const uint8_t* payload = frame + 14;
    uint16_t payload_len = length - 14;

    if (ethertype == NET_ETHERTYPE_ARP) {
        handle_arp(payload, payload_len);
    } else if (ethertype == NET_ETHERTYPE_IPV4) {
        handle_ipv4(payload, payload_len);
    }
}

void NetStack::handle_arp(const uint8_t* payload, uint16_t length) {
    if (length < 28) {
        s_stats.rx_dropped++;
        return;
    }

    if (read_be16(payload + 0) != 1 || read_be16(payload + 2) != NET_ETHERTYPE_IPV4 ||
        payload[4] != 6 || payload[5] != 4) {
        return;
    }

    s_stats.arp_rx++;
    uint16_t op = read_be16(payload + 6);
    const uint8_t* sender_mac = payload + 8;
    uint32_t sender_ip = read_be32(payload + 14);
    uint32_t target_ip = read_be32(payload + 24);

    cache_arp(sender_ip, sender_mac);

    if (op == 1 && target_ip == s_ipv4) {
        send_arp_reply(sender_mac, sender_ip);
    }
}

void NetStack::handle_ipv4(const uint8_t* payload, uint16_t length) {
    if (length < 20) {
        s_stats.rx_dropped++;
        return;
    }

    uint8_t ihl = (payload[0] & 0x0F) * 4;
    if ((payload[0] >> 4) != 4 || ihl < 20 || length < ihl) {
        s_stats.rx_dropped++;
        return;
    }

    uint16_t total_len = read_be16(payload + 2);
    if (total_len < ihl || total_len > length) {
        s_stats.rx_dropped++;
        return;
    }

    uint16_t fragment = read_be16(payload + 6);
    if (fragment & 0x3FFF) {
        s_stats.rx_dropped++;
        return;
    }

    uint16_t header_sum = internet_checksum(payload, ihl);
    if (header_sum != 0) {
        s_stats.rx_dropped++;
        return;
    }

    uint32_t src_ip = read_be32(payload + 12);
    uint32_t dst_ip = read_be32(payload + 16);
    if (!is_local_or_broadcast(dst_ip)) {
        return;
    }

    s_stats.ipv4_rx++;
    uint8_t protocol = payload[9];
    const uint8_t* body = payload + ihl;
    uint16_t body_len = total_len - ihl;

    if (protocol == NET_IP_PROTO_ICMP) {
        handle_icmp(src_ip, payload[8], body, body_len);
    } else if (protocol == NET_IP_PROTO_UDP) {
        handle_udp(src_ip, body, body_len);
    }
}

void NetStack::handle_icmp(uint32_t src_ip, uint8_t ttl, const uint8_t* payload, uint16_t length) {
    if (length < 8) return;
    if (internet_checksum(payload, length) != 0) {
        s_stats.rx_dropped++;
        return;
    }

    s_stats.icmp_rx++;
    uint8_t type = payload[0];

    if (type == 8) {
        uint8_t reply[NET_MTU - 20];
        if (length > sizeof(reply)) return;
        memcpy(reply, payload, length);
        reply[0] = 0;
        write_be16(reply + 2, 0);
        write_be16(reply + 2, internet_checksum(reply, length));
        if (send_ipv4(src_ip, NET_IP_PROTO_ICMP, reply, length)) {
            s_stats.icmp_tx++;
        }
    } else if (type == 0) {
        s_ping_id = read_be16(payload + 4);
        s_ping_seq = read_be16(payload + 6);
        s_ping_src_ip = src_ip;
        s_ping_ttl = ttl;
        s_ping_pending = true;
    }
}

void NetStack::handle_udp(uint32_t src_ip, const uint8_t* payload, uint16_t length) {
    if (length < 8) {
        s_stats.rx_dropped++;
        return;
    }

    uint16_t src_port = read_be16(payload + 0);
    uint16_t dst_port = read_be16(payload + 2);
    uint16_t udp_len = read_be16(payload + 4);
    if (udp_len < 8 || udp_len > length) {
        s_stats.rx_dropped++;
        return;
    }

    s_stats.udp_rx++;
    push_udp_packet(dst_port, src_ip, src_port, payload + 8, udp_len - 8);
}

void NetStack::print_info() {
    NetInfo info;
    fill_info(&info);
    char mac[18];
    char ip[16];
    char mask[16];
    char gw[16];
    format_mac(info.mac, mac);
    format_ipv4(info.ipv4_addr, ip);
    format_ipv4(info.netmask, mask);
    format_ipv4(info.gateway, gw);

    printf("net0: %s (%s)\n", info.link_up ? "up" : "down", info.driver);
    printf("  mac %s\n", mac);
    printf("  inet %s mask %s gateway %s\n", ip, mask, gw);
    printf("  rx frames=%u bytes=%u dropped=%u\n",
           info.stats.rx_frames, info.stats.rx_bytes, info.stats.rx_dropped);
    printf("  tx frames=%u bytes=%u dropped=%u\n",
           info.stats.tx_frames, info.stats.tx_bytes, info.stats.tx_dropped);
    printf("  arp rx/tx=%u/%u ipv4 rx/tx=%u/%u icmp rx/tx=%u/%u udp rx/tx=%u/%u\n",
           info.stats.arp_rx, info.stats.arp_tx,
           info.stats.ipv4_rx, info.stats.ipv4_tx,
           info.stats.icmp_rx, info.stats.icmp_tx,
           info.stats.udp_rx, info.stats.udp_tx);
}

void NetStack::print_arp_cache() {
    printf("IP address       MAC address        age\n");
    for (int i = 0; i < kArpCacheSize; i++) {
        if (!s_arp_cache[i].valid) continue;
        char ip[16];
        char mac[18];
        format_ipv4(s_arp_cache[i].ip, ip);
        format_mac(s_arp_cache[i].mac, mac);
        uint32_t age = Timer::get_ticks() - s_arp_cache[i].updated_ticks;
        printf("%s  %s  %u ticks\n", ip, mac, age);
    }
}

} // namespace re36
