#include "app_api.h"
#include "vesa_driver.h"


void operator delete(void*, unsigned int) {}
void operator delete(void*) {}
extern "C" void __cxa_pure_virtual() {
    vlsmc::App::print("Pure virtual function call!\n");
    vlsmc::App::exit(1);
}

using namespace vlsmc;

void print_result(const char* test_name, int expected, int actual) {
    App::print(test_name);
    App::print(": ");
    if (expected == actual) {
        App::print("PASSED\n");
    } else {
        App::print("FAILED\n");
    }
}

int main() {
    App::print("=========================\n");
    App::print("  VESA Driver Security Test (Ring 3)\n");
    App::print("=========================\n");

    VesaDriver driver;
    DriverContext ctx;
    ctx.private_data = nullptr;
    ctx.private_data_size = 0;
    ctx.device_id = 1;

    int res = driver.write(&ctx, "test", 4);
    print_result("[1] Write before init", -1, res);

    res = driver.draw_pixel(10, 10, 15);
    print_result("[2] Draw before init", -1, res);

    App::print("Initializing driver...\n");
    res = driver.init(&ctx);
    if (res != 0) {
        App::print("Driver Init FAILED!\n");
        return 1;
    }

    res = driver.init(&ctx);
    print_result("[3] Init when already init (Double-init leak prevention)", -3, res);

    VesaResolution vres;
    res = driver.ioctl(&ctx, VESA_CMD_GET_RES, &vres);
    print_result("[4] ioctl GET_RES valid", 0, res);

    res = driver.draw_pixel(vres.width, 10, 15);
    print_result("[5] Bounds check X (> width-1)", -2, res);

    res = driver.draw_pixel(10, vres.height, 15);
    print_result("[6] Bounds check Y (> height-1)", -2, res);

    res = driver.draw_pixel(160, 100, 4); 
    print_result("[7] Valid pixel draw", 0, res);

    res = driver.write(&ctx, nullptr, 100);
    print_result("[8] Write with nullptr", -1, res);

    res = driver.seek(&ctx, 0xFFFFFFFF);
    print_result("[9] Seek out of bounds", -2, res);

    uint32_t total_size = vres.width * vres.height * (vres.bpp / 8);
    res = driver.seek(&ctx, total_size - 5);
    print_result("[10] Seek to last 5 bytes", 0, res);

    uint8_t dummy_buf[10] = {0};
    res = driver.write(&ctx, dummy_buf, 10);
    print_result("[11] Write truncating at boundary (5 bytes written)", 5, res);

    res = driver.seek(&ctx, total_size - 4);
    res = driver.read(&ctx, dummy_buf, 10);
    print_result("[12] Read truncating at boundary (4 bytes read)", 4, res);

    driver.stop(&ctx);
    App::print("\nTests finished. Driver stopped smoothly.\n");
    App::sleep(2000);
    return 0;
}
