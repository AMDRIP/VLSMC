#pragma once

#include "driver_api.h"
#include "app_api.h"

using namespace vlsmc;

#define VESA_CMD_GET_RES 1
#define VESA_CMD_FLUSH   2

struct VesaResolution {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
};

class VesaDriver : public DriverAPI {
private:
    uint8_t* framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    bool is_initialized;
    uint32_t cursor_offset;
    uint32_t virt_addr;
    uint32_t mapped_pages;

public:
    VesaDriver() : framebuffer(nullptr), width(0), height(0), bpp(0), is_initialized(false), cursor_offset(0), virt_addr(0), mapped_pages(0) {}

    virtual int init(DriverContext* ctx) override {
        if (!ctx) return -1;
        if (is_initialized) return -3; // Prevent leaks from double init
        
        uint32_t phys_lfb = 0;
        if (App::get_vga_info(&width, &height, &bpp, &phys_lfb) != 0) return -4;

        uint32_t max_size = width * height * (bpp / 8);
        mapped_pages = (max_size + 4095) / 4096;
        
        virt_addr = 0xD0000000;
        framebuffer = (uint8_t*)App::map_mmio(virt_addr, phys_lfb, mapped_pages); 
        
        if (!framebuffer) return -2;

        is_initialized = true;
        cursor_offset = 0;
        return 0;
    }

    virtual int seek(DriverContext* ctx, uint32_t offset) override {
        (void)ctx;
        if (!is_initialized) return -1;
        uint32_t max_size = width * height * (bpp / 8);
        if (offset > max_size) return -2;
        cursor_offset = offset;
        return 0;
    }

    virtual int read(DriverContext* ctx, void* buffer, size_t size) override {
        (void)ctx;
        if (!is_initialized || !buffer || !framebuffer) return -1;
        
        size_t max_size = width * height * (bpp / 8);
        if (cursor_offset >= max_size) return 0; // EOF

        size_t available = max_size - cursor_offset;
        if (size > available) size = available; // Truncate safely

        uint8_t* dst = (uint8_t*)buffer;
        for (size_t i = 0; i < size; i++) {
            dst[i] = framebuffer[cursor_offset + i];
        }
        cursor_offset += size;
        return size;
    }

    virtual int write(DriverContext* ctx, const void* buffer, size_t size) override {
        (void)ctx;
        if (!is_initialized || !buffer || !framebuffer) return -1;

        size_t max_size = width * height * (bpp / 8);
        if (cursor_offset >= max_size) return 0; // EOF

        size_t available = max_size - cursor_offset;
        if (size > available) size = available; // Truncate safely

        const uint8_t* src = (const uint8_t*)buffer;
        for (size_t i = 0; i < size; i++) {
            framebuffer[cursor_offset + i] = src[i];
        }
        cursor_offset += size;
        return size;
    }

    virtual int ioctl(DriverContext* ctx, uint32_t cmd, void* arg) override {
        (void)ctx;
        if (!is_initialized) return -1;

        switch (cmd) {
            case VESA_CMD_GET_RES: {
                if (!arg) return -3;
                VesaResolution* res = (VesaResolution*)arg;
                res->width = width;
                res->height = height;
                res->bpp = bpp;
                return 0;
            }
            case VESA_CMD_FLUSH: {
                return 0;
            }
            default:
                return -4;
        }
    }

    virtual void stop(DriverContext* ctx) override {
        (void)ctx;
        if (is_initialized) {
            App::unmap_mmio(virt_addr, mapped_pages);
        }
        is_initialized = false;
        framebuffer = nullptr;
    }

    int draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
        if (!is_initialized || !framebuffer) return -1;
        if (x >= width || y >= height) return -2; 
        
        uint32_t byte_offset = y * width * (bpp / 8) + x * (bpp / 8);
        if (bpp == 8) {
            framebuffer[byte_offset] = (uint8_t)color;
        } else if (bpp == 32) {
            framebuffer[byte_offset] = color & 0xFF;
            framebuffer[byte_offset+1] = (color >> 8) & 0xFF;
            framebuffer[byte_offset+2] = (color >> 16) & 0xFF;
            framebuffer[byte_offset+3] = (color >> 24) & 0xFF;
        }
        return 0;
    }
};
