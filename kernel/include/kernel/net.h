#pragma once

#include <stdint.h>
#include <stddef.h>

namespace re36 {

#define NET_MAX_FRAME_SIZE 1518
#define NET_MTU 1500
#define NET_MAX_UDP_PAYLOAD 1472

#define NET_ETHERTYPE_IPV4 0x0800
#define NET_ETHERTYPE_ARP  0x0806

#define NET_IP_PROTO_ICMP 1
#define NET_IP_PROTO_UDP  17

struct NetStats {
    uint32_t rx_frames;
    uint32_t tx_frames;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t rx_dropped;
    uint32_t tx_dropped;
    uint32_t arp_rx;
    uint32_t arp_tx;
    uint32_t ipv4_rx;
    uint32_t ipv4_tx;
    uint32_t icmp_rx;
    uint32_t icmp_tx;
    uint32_t udp_rx;
    uint32_t udp_tx;
};

struct NetInfo {
    bool link_up;
    char driver[16];
    uint8_t mac[6];
    uint32_t ipv4_addr;
    uint32_t netmask;
    uint32_t gateway;
    NetStats stats;
};

struct NetUdpPacket {
    bool used;
    uint32_t src_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint8_t payload[NET_MAX_UDP_PAYLOAD];
};

typedef bool (*NetTxFunc)(const uint8_t* frame, uint16_t length);

class NetStack {
public:
    static void init();
    static bool attach_device(const char* name, const uint8_t* mac, NetTxFunc tx);
    static bool has_device();
    static bool is_link_up();

    static void configure_ipv4(uint32_t ip, uint32_t mask, uint32_t gateway);
    static void fill_info(NetInfo* out);
    static const NetStats& stats();

    static uint32_t parse_ipv4(const char* text, bool* ok);
    static void format_ipv4(uint32_t ip, char* out);
    static void format_mac(const uint8_t* mac, char* out);

    static void receive_frame(const uint8_t* frame, uint16_t length);
    static bool send_udp(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port,
                         const uint8_t* payload, uint16_t length);
    static int recv_udp(uint16_t local_port, uint32_t* src_ip, uint16_t* src_port,
                        uint8_t* payload, uint16_t max_length);
    static bool send_icmp_echo(uint32_t dst_ip, uint16_t id, uint16_t seq);
    static bool consume_ping_reply(uint16_t id, uint16_t seq, uint32_t* src_ip, uint8_t* ttl);

    static void print_info();
    static void print_arp_cache();

private:
    static bool send_ethernet(const uint8_t* dst_mac, uint16_t ethertype,
                              const uint8_t* payload, uint16_t length);
    static bool send_ipv4(uint32_t dst_ip, uint8_t protocol,
                          const uint8_t* payload, uint16_t length);
    static bool resolve_mac(uint32_t dst_ip, uint8_t* out_mac);
    static void send_arp_request(uint32_t target_ip);
    static void send_arp_reply(const uint8_t* dst_mac, uint32_t dst_ip);

    static void handle_arp(const uint8_t* payload, uint16_t length);
    static void handle_ipv4(const uint8_t* payload, uint16_t length);
    static void handle_icmp(uint32_t src_ip, uint8_t ttl, const uint8_t* payload, uint16_t length);
    static void handle_udp(uint32_t src_ip, const uint8_t* payload, uint16_t length);
};

} // namespace re36
