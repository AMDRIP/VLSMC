#include "kernel/bga.h"
#include "kernel/pci.h"
#include "kernel/vmm.h"
#include "kernel/pic.h" // For inw/outw
#include "libc.h"

#define VBE_DISPI_IOPORT_INDEX 0x01CE
#define VBE_DISPI_IOPORT_DATA  0x01CF

#define VBE_DISPI_INDEX_ID           0x0
#define VBE_DISPI_INDEX_XRES         0x1
#define VBE_DISPI_INDEX_YRES         0x2
#define VBE_DISPI_INDEX_BPP          0x3
#define VBE_DISPI_INDEX_ENABLE       0x4
#define VBE_DISPI_INDEX_BANK         0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH   0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT  0x7
#define VBE_DISPI_INDEX_X_OFFSET     0x8
#define VBE_DISPI_INDEX_Y_OFFSET     0x9

#define VBE_DISPI_DISABLED           0x00
#define VBE_DISPI_ENABLED            0x01
#define VBE_DISPI_LFB_ENABLED        0x40
#define VBE_DISPI_NOCLEARMEM         0x80

namespace re36 {

uint16_t BgaDriver::width_ = 0;
uint16_t BgaDriver::height_ = 0;
uint16_t BgaDriver::bpp_ = 0;
uint32_t BgaDriver::pitch_ = 0;

uint32_t BgaDriver::framebuffer_phys_ = 0;
uint32_t BgaDriver::framebuffer_virt_ = 0xE0000000; // Choose a high virtual address
uint32_t BgaDriver::framebuffer_size_ = 0;

// inw/outw are included from kernel/pic.h

void BgaDriver::write_register(uint16_t index, uint16_t val) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, val);
}

uint16_t BgaDriver::read_register(uint16_t index) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

void BgaDriver::init(uint16_t width, uint16_t height, uint16_t bpp) {
    printf("[BGA] Initializing Bochs Graphics Adapter...\n");

    // 1. Find the BGA device on the PCI bus
    PCIDevice* devices = PCI::get_devices();
    int count = PCI::get_device_count();
    PCIDevice* info = nullptr;
    
    for (int i = 0; i < count; i++) {
        if (devices[i].vendor_id == 0x1234 && devices[i].device_id == 0x1111) {
            info = &devices[i];
            break;
        }
    }

    if (!info) {
        printf("[BGA] Error: QEMU/Bochs VGA (0x1234:0x1111) not found on PCI bus!\n");
        return;
    }

    // 2. Extract BAR0 (Physical Address of the Framebuffer)
    uint32_t bar0 = PCI::config_read_dword(info->bus, info->slot, info->func, 0x10);
    framebuffer_phys_ = bar0 & 0xFFFFFFF0; // Strip lower flag bits

    if (framebuffer_phys_ == 0) {
        printf("[BGA] Error: BAR0 is 0. Framebuffer physical address is invalid.\n");
        return;
    }

    width_ = width;
    height_ = height;
    bpp_ = bpp;
    pitch_ = (width * bpp) / 8; // Default calculation, BGA will enforce this as virtual width
    
    // Calculate total memory needed
    framebuffer_size_ = pitch_ * height_;

    // 3. Map the Framebuffer in VMM
    printf("[BGA] Mapping %d bytes at Phys: 0x%x to Virt: 0x%x\n", 
           framebuffer_size_, framebuffer_phys_, framebuffer_virt_);
           
    for (uint32_t i = 0; i < framebuffer_size_; i += PAGE_SIZE) {
        VMM::map_page(framebuffer_virt_ + i, framebuffer_phys_ + i, PAGE_PRESENT | PAGE_WRITABLE);
    }

    // 4. Configure BGA Registers (Crucial steps to avoid artifacting and bugs)
    
    // CRITICAL: Disable the BGA before attempting to change settings
    write_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);

    write_register(VBE_DISPI_INDEX_XRES, width_);
    write_register(VBE_DISPI_INDEX_YRES, height_);
    write_register(VBE_DISPI_INDEX_VIRT_WIDTH, width_);
    write_register(VBE_DISPI_INDEX_BPP, bpp_);
    write_register(VBE_DISPI_INDEX_X_OFFSET, 0);
    write_register(VBE_DISPI_INDEX_Y_OFFSET, 0);

    // Enable BGA and Linear Framebuffer
    write_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    // Verify Virtual Width to correct our Pitch
    uint16_t actual_virt_width = read_register(VBE_DISPI_INDEX_VIRT_WIDTH);
    pitch_ = actual_virt_width * (bpp_ / 8);

    printf("[BGA] Enabled mode %dx%d %dbpp (Pitch: %d)\n", width_, height_, bpp_, pitch_);
}

void BgaDriver::put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= width_ || y >= height_) return;
    
    // CRITICAL: Use pitch and bytes_per_pixel, not `y * width`
    uint32_t offset = (y * pitch_) + (x * (bpp_ / 8));
    
    uint8_t* fb = (uint8_t*)framebuffer_virt_;
    if (bpp_ == 32) {
        *(uint32_t*)(fb + offset) = color; // Write 4 bytes directly
    } else if (bpp_ == 24) {
        fb[offset] = color & 0xFF;
        fb[offset + 1] = (color >> 8) & 0xFF;
        fb[offset + 2] = (color >> 16) & 0xFF;
    } else if (bpp_ == 16) {
        *(uint16_t*)(fb + offset) = (uint16_t)color;
    }
}

void BgaDriver::clear_screen(uint32_t color) {
    if (bpp_ == 32) {
        uint32_t* fb32 = (uint32_t*)framebuffer_virt_;
        int total_pixels = (pitch_ * height_) / 4;
        for (int i = 0; i < total_pixels; i++) {
            fb32[i] = color;
        }
    } else {
        for (uint32_t y = 0; y < height_; y++) {
            for (uint32_t x = 0; x < width_; x++) {
                put_pixel(x, y, color);
            }
        }
    }
}

} // namespace re36
