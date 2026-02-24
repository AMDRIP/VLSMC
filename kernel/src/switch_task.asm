[BITS 32]

global switch_task

; void switch_task(uint32_t* old_esp, uint32_t new_esp)
;
; Аргументы (cdecl):
;   [esp+4] = uint32_t* old_esp  (указатель, куда сохранить ESP текущего потока)
;   [esp+8] = uint32_t  new_esp  (значение ESP нового потока для загрузки)
;
; 1. Сохраняем регистры текущего потока на ЕГО стек
; 2. Записываем текущий ESP в *old_esp
; 3. Загружаем new_esp в ESP
; 4. Восстанавливаем регистры нового потока с ЕГО стека
; 5. ret → возвращаемся в точку, откуда новый поток вызвал switch_task

switch_task:
    pushf
    push ebx
    push esi
    push edi
    push ebp

    mov eax, [esp + 24]     ; old_esp (первый аргумент, +20 от 5 пушей + 4 от ret addr)
    mov [eax], esp          ; *old_esp = текущий ESP

    mov esp, [esp + 28]     ; new_esp (второй аргумент) — НО! мы уже сменили esp,
                            ; поэтому нужно пересчитать: 5 пушей (20) + ret (4) + arg1 (4) = 28

    pop ebp
    pop edi
    pop esi
    pop ebx
    popf

    ret
