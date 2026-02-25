#include "app_api.h"

using namespace vlsmc;

int main() {
    App::print("=========================\n");
    App::print("  Hello from C++ AppAPI!\n");
    App::print("  Running in Ring 3\n");
    App::print("=========================\n");

    uint32_t pid = App::get_pid();
    char buf[] = "  PID: X\n";
    buf[7] = '0' + (pid % 10);
    App::print(buf);

    App::print("\n  Counting: ");
    for (int i = 0; i < 5; i++) {
        char c[] = "X ";
        c[0] = '0' + i;
        App::print(c);
        App::sleep(500);
    }
    App::print("\n  Done!\n");

    return 0;
}
