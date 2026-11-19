#include <windows.h>

#include "PaymentGuiWindow.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE /*prev_instance*/,
    LPSTR /*cmd_line*/, int cmd_show) {
    PaymentGuiWindow app_window;

    if (!app_window.create(instance, cmd_show)) {
        MessageBoxA(nullptr, "Не вдалося ініціалізувати головне вікно програми!",
            "Критична помилка", MB_OK | MB_ICONERROR);
        return 1;
    }

    return app_window.runMessageLoop();
}