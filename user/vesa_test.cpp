#include "app_api.h"
#include "vesa_driver.h"

// Заглушки для C++ без стандартной библиотеки
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

    // --- ТЕСТ 1: Вызовы до инициализации ---
    int res = driver.write(&ctx, "test", 4);
    print_result("[1] Write before init", -1, res);

    res = driver.draw_pixel(10, 10, 15);
    print_result("[2] Draw before init", -1, res);

    // --- Инициализация ---
    App::print("Initializing driver...\n");
    res = driver.init(&ctx);
    if (res != 0) {
        App::print("Driver Init FAILED!\n");
        return 1;
    }

    // --- ТЕСТ 3: Bounds Checking (Выход за границы X) ---
    res = driver.draw_pixel(320, 10, 15);
    print_result("[3] Bounds check X (> 319)", -2, res);

    // --- ТЕСТ 4: Bounds Checking (Выход за границы Y) ---
    res = driver.draw_pixel(10, 200, 15);
    print_result("[4] Bounds check Y (> 199)", -2, res);

    // --- ТЕСТ 5: Корректная отрисовка пикселя ---
    res = driver.draw_pixel(160, 100, 4); // Красный центр
    print_result("[5] Valid pixel draw", 0, res);

    // --- ТЕСТ 6: Запись с нулевым указателем (nullptr) ---
    res = driver.write(&ctx, nullptr, 100);
    print_result("[6] Write with nullptr", -1, res);

    // --- ТЕСТ 7: Buffer Overflow при записи ---
    uint8_t dummy_buf[10];
    res = driver.write(&ctx, dummy_buf, 320 * 200 + 1);
    print_result("[7] Buffer Overflow check (Write)", -2, res);

    // --- ТЕСТ 8: Buffer Overflow при чтении ---
    res = driver.read(&ctx, dummy_buf, 320 * 200 + 1);
    print_result("[8] Buffer Overflow check (Read)", -2, res);

    // --- ТЕСТ 9: ioctl с неизвестной командой ---
    res = driver.ioctl(&ctx, 999, nullptr);
    print_result("[9] ioctl invalid command", -4, res);

    // --- ТЕСТ 10: ioctl GET_RES с нулевым указателем ---
    res = driver.ioctl(&ctx, VESA_CMD_GET_RES, nullptr);
    print_result("[10] ioctl GET_RES nullptr arg", -3, res);

    // Успешный IOCTL
    VesaResolution vres;
    res = driver.ioctl(&ctx, VESA_CMD_GET_RES, &vres);
    print_result("[11] ioctl GET_RES valid", 0, res);

    App::print("\nTests finished.\n");
    App::sleep(2000);
    return 0;
}
