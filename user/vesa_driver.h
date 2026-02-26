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

public:
    VesaDriver() : framebuffer(nullptr), width(320), height(200), bpp(8), is_initialized(false) {}

    virtual int init(DriverContext* ctx) override {
        if (!ctx) return -1;
        
        // В нашем упрощенном примере мы харкдодим параметры Mode 13h
        // В реальной ОС эти данные (адрес LFB, ширина) передаются ядром через ctx->private_data
        uint32_t phys_lfb = 0xA0000;
        
        // Мапим физический адрес видеобуфера в наше виртуальное пространство 
        // Mode 13h (320x200 = 64000 байт), нам нужно 16 страниц по 4096
        framebuffer = (uint8_t*)App::map_mmio(0xC0000000, phys_lfb, 16); 
        
        if (!framebuffer) return -2;

        is_initialized = true;
        return 0; // Успех
    }

    virtual int read(DriverContext* ctx, void* buffer, size_t size) override {
        (void)ctx;
        if (!is_initialized || !buffer || !framebuffer) return -1;
        
        // Защита от переполнения: нельзя прочитать больше чем размер экрана
        size_t max_size = width * height * (bpp / 8);
        if (size > max_size) return -2;

        // Копируем из LFB (чтение пикселей)
        uint8_t* dst = (uint8_t*)buffer;
        for (size_t i = 0; i < size; i++) {
            dst[i] = framebuffer[i];
        }
        return size;
    }

    virtual int write(DriverContext* ctx, const void* buffer, size_t size) override {
        (void)ctx;
        if (!is_initialized || !buffer || !framebuffer) return -1;

        // Защита от переполнения: нельзя записать больше чем размер экрана (Bounds checking)
        size_t max_size = width * height * (bpp / 8);
        if (size > max_size) return -2;

        const uint8_t* src = (const uint8_t*)buffer;
        for (size_t i = 0; i < size; i++) {
            framebuffer[i] = src[i];
        }
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
                // В Mode 13h память маппится напрямую, flush не требуется
                return 0;
            }
            default:
                return -4; // Неизвестная команда
        }
    }

    virtual void stop(DriverContext* ctx) override {
        (void)ctx;
        is_initialized = false;
        framebuffer = nullptr;
    }

    // Вспомогательный безопасный метод отрисовки точки
    int draw_pixel(uint32_t x, uint32_t y, uint8_t color) {
        if (!is_initialized || !framebuffer) return -1;
        if (x >= width || y >= height) return -2; // Bounds checking
        
        framebuffer[y * width + x] = color;
        return 0;
    }
};
