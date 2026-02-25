#include "app_api.h"

// Драйвер FS мы будем запускать в tid 1 (0 - shell, 1 - FS, 2 - CAT_TEST)
#define FS_TID 1

// Заглушки для C++ рантайма
extern "C" void __cxa_pure_virtual() {
    while (1);
}

void* operator new(unsigned int size) {
    return (void*)0; // Пока нет кучи
}

void operator delete(void* p) {
}

void operator delete(void* p, unsigned int size) {
}

int main() {
    vlsmc::App::print("[CAT] Requesting file 'HELLO.TXT' from FS driver...\n");

    const char* req = "HELLO   TXT";
    int req_len = 11;
    
    // Пытаемся отправить запрос драйверу файловой системы
    int ret = vlsmc::App::msg_send(FS_TID, req, req_len);
    if (ret < 0) {
        vlsmc::App::print("[CAT] FS Driver not found or queue is full!\n");
        return 1;
    }

    vlsmc::App::print("[CAT] Request sent. Waiting for response...\n");

    uint8_t buf[512];
    int sender_tid;
    
    // Ждем ответ от FS
    while (true) {
        int sz = vlsmc::App::msg_recv(&sender_tid, buf, sizeof(buf));
        if (sz > 0) {
            vlsmc::App::print("\n--- FILE CONTENT ---\n");
            
            // Выводим полученный текст (ограничивая размер, чтобы не мусорить)
            for (int i = 0; i < sz && i < 200; i++) {
                vlsmc::App::print((const char*)&buf[i]); // Нужно написать print для 1 символа, но у нас print принимает null-terminated.
                // Сделаем хак:
                char tmp[2] = {(char)buf[i], 0};
                vlsmc::App::print(tmp);
            }
            
            vlsmc::App::print("\n--------------------\n");
            break;
        } else {
            vlsmc::App::yield();
        }
    }

    vlsmc::App::print("[CAT] Finished.\n");
    return 0;
}
