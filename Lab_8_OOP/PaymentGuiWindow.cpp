#include "PaymentGuiWindow.h"

namespace {

    // Допоміжна функція форматування грошових сум з двома знаками після коми
    std::string formatMoney(double value) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value << " UAH";
        return oss.str();
    }

}  // namespace

PaymentGuiWindow::PaymentGuiWindow()
    : instance_(nullptr),
    main_hwnd_(nullptr),
    ui_font_(nullptr),
    header_font_(nullptr),
    balance_font_(nullptr),
    lbl_owner_(nullptr),
    lbl_acc_number_(nullptr),
    lbl_balance_(nullptr),
    edit_amount_(nullptr),
    edit_recipient_(nullptr),
    list_history_(nullptr),
    lbl_status_(nullptr),
    account_("UA213052990000026001", "Іван Петренко", 15000.00, "UAH", true,
        "ПриватБанк", "20.11.2026", 0),
    history_("Іван Петренко", 1, "20.11.2026"),
    next_tx_id_(1) {
}

PaymentGuiWindow::~PaymentGuiWindow() {
    if (ui_font_) DeleteObject(ui_font_);
    if (header_font_) DeleteObject(header_font_);
    if (balance_font_) DeleteObject(balance_font_);
}

bool PaymentGuiWindow::create(HINSTANCE instance, int cmd_show) {
    instance_ = instance;
    const char* kClassName = "PaymentSystemGuiClass";

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = PaymentGuiWindow::windowProc;
    wc.hInstance = instance_;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kClassName;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        return false;
    }

    // Центрування вікна на екрані користувача (правило зручності UX)
    int width = 620;
    int height = 560;
    int screen_x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int screen_y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    main_hwnd_ = CreateWindowExA(
        0, kClassName, "Система «Платежі» — Банківський кабінет клієнта",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, screen_x,
        screen_y, width, height, nullptr, nullptr, instance_, this);

    if (!main_hwnd_) {
        return false;
    }

    ShowWindow(main_hwnd_, cmd_show);
    UpdateWindow(main_hwnd_);
    return true;
}

int PaymentGuiWindow::runMessageLoop() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK PaymentGuiWindow::windowProc(HWND hwnd, UINT msg,
    WPARAM wparam, LPARAM lparam) {
    PaymentGuiWindow* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* create_struct = reinterpret_cast<CREATESTRUCTA*>(lparam);
        self = static_cast<PaymentGuiWindow*>(create_struct->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else {
        self = reinterpret_cast<PaymentGuiWindow*>(
            GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    }

    if (self) {
        return self->handleMessage(hwnd, msg, wparam, lparam);
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

LRESULT PaymentGuiWindow::handleMessage(HWND hwnd, UINT msg, WPARAM wparam,
    LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        initializeControls(hwnd);
        applyModernFont();
        refreshAccountDisplay();
        recordTransaction("Відкриття", 15000.00, 0.0, "Початковий баланс рахунку");
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wparam);
        double amount = 0.0;

        switch (id) {
        case kBtnDeposit:
            if (parseInputAmount(&amount)) {
                // Використання перевантаженого оператора += з 6-ї лабораторної
                account_ += amount;
                recordTransaction("Поповнення", amount, 0.0, "Поповнення рахунку");
                refreshAccountDisplay();
                updateStatus("Рахунок успішно поповнено на " + formatMoney(amount));
            }
            break;

        case kBtnWithdraw:
            if (parseInputAmount(&amount)) {
                if (account_.getBalance() < amount) {
                    MessageBoxA(hwnd, "Недостатньо коштів на рахунку для зняття!",
                        "Помилка операції", MB_OK | MB_ICONWARNING);
                    updateStatus("Відхилено: недостатньо коштів.");
                }
                else {
                    // Використання перевантаженого оператора -= з 6-ї лабораторної
                    account_ -= amount;
                    recordTransaction("Зняття", amount, 0.0, "Зняття готівки");
                    refreshAccountDisplay();
                    updateStatus("Виконано зняття коштів: " + formatMoney(amount));
                }
            }
            break;

        case kBtnTransfer:
            if (parseInputAmount(&amount)) {
                char recipient_buf[128] = {};
                GetWindowTextA(edit_recipient_, recipient_buf,
                    sizeof(recipient_buf));
                std::string recipient(recipient_buf);

                if (recipient.empty()) {
                    MessageBoxA(hwnd, "Вкажіть рахунок або ім'я отримувача!",
                        "Перевірка даних", MB_OK | MB_ICONINFORMATION);
                    SetFocus(edit_recipient_);
                    break;
                }

                double commission = account_.calculateCommission(amount);
                double total = amount + commission;

                if (account_.getBalance() < total) {
                    MessageBoxA(
                        hwnd,
                        "Недостатньо коштів з урахуванням комісії банку (1.5%)!",
                        "Помилка переказу", MB_OK | MB_ICONWARNING);
                    updateStatus("Відхилено: недостатньо коштів для переказу.");
                }
                else {
                    account_ -= total;
                    recordTransaction("Переказ", amount, commission,
                        "Отримувач: " + recipient);
                    refreshAccountDisplay();
                    updateStatus("Переказ на " + recipient + " виконано.");
                }
            }
            break;

        case kBtnBonus:
            // Нарахування +5% на залишок через оператор *=
            account_ *= 1.05;
            recordTransaction("Відсотки", 0.0, 0.0,
                "Нарахування +5% на залишок");
            refreshAccountDisplay();
            updateStatus("Нараховано бонусні +5% на залишок рахунку.");
            break;

        case kBtnClearHistory:
            SendMessageA(list_history_, LB_RESETCONTENT, 0, 0);
            history_.clearHistory();
            updateStatus("Журнал транзакцій очищено.");
            break;
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

void PaymentGuiWindow::initializeControls(HWND hwnd) {
    // 1. Заголовок інтерфейсу (Візуальна ієрархія)
    HWND title = CreateWindowExA(
        0, "STATIC", "БАНКІВСЬКИЙ ТЕРМІНАЛ КЕРУВАННЯ РАХУНКОМ",
        WS_CHILD | WS_VISIBLE, 20, 15, 560, 26, hwnd, nullptr, instance_, nullptr);
    SendMessageA(title, WM_SETFONT, reinterpret_cast<WPARAM>(header_font_), TRUE);

    // 2. Група «Стан рахунку» (Принцип групування пов'язаних даних)
    CreateWindowExA(0, "BUTTON", " Інформація про поточний рахунок ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 48, 565, 95, hwnd,
        nullptr, instance_, nullptr);

    lbl_owner_ = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 35, 72,
        300, 20, hwnd, nullptr, instance_, nullptr);
    lbl_acc_number_ =
        CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 35, 96, 300, 20,
            hwnd, nullptr, instance_, nullptr);
    lbl_balance_ =
        CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_RIGHT, 340,
            78, 225, 32, hwnd, nullptr, instance_, nullptr);

    // 3. Група «Фінансові операції» (Зручні відступи та логічний порядок дій)
    CreateWindowExA(0, "BUTTON", " Виконання платіжних операцій ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 152, 565, 130, hwnd,
        nullptr, instance_, nullptr);

    CreateWindowExA(0, "STATIC", "Сума (UAH):", WS_CHILD | WS_VISIBLE, 35, 180,
        90, 22, hwnd, nullptr, instance_, nullptr);
    edit_amount_ = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "1000", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        125, 177, 130, 25, hwnd, reinterpret_cast<HMENU>(kEditAmount), instance_,
        nullptr);

    CreateWindowExA(0, "STATIC", "Отримувач:", WS_CHILD | WS_VISIBLE, 280, 180,
        85, 22, hwnd, nullptr, instance_, nullptr);
    edit_recipient_ = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", "UA99002 (Олена)",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 365, 177, 200, 25, hwnd,
        reinterpret_cast<HMENU>(kEditRecipient), instance_, nullptr);

    // Кнопки дій (з чіткими дієсловами дії згідно правил UX)
    CreateWindowExA(0, "BUTTON", "Поповнити (+)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 35, 222, 120, 34, hwnd,
        reinterpret_cast<HMENU>(kBtnDeposit), instance_, nullptr);

    CreateWindowExA(0, "BUTTON", "Зняти (-)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 165, 222, 110, 34, hwnd,
        reinterpret_cast<HMENU>(kBtnWithdraw), instance_, nullptr);

    CreateWindowExA(0, "BUTTON", "Переказати (1.5%)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 285, 222, 140, 34, hwnd,
        reinterpret_cast<HMENU>(kBtnTransfer), instance_, nullptr);

    CreateWindowExA(0, "BUTTON", "Бонус +5%",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 435, 222, 130, 34, hwnd,
        reinterpret_cast<HMENU>(kBtnBonus), instance_, nullptr);

    // 4. Група «Історія транзакцій»
    CreateWindowExA(0, "BUTTON", " Журнал виконаних транзакцій ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 292, 565, 190, hwnd,
        nullptr, instance_, nullptr);

    list_history_ = CreateWindowExA(
        WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 35, 315, 535,
        125, hwnd, reinterpret_cast<HMENU>(kListHistory), instance_, nullptr);

    CreateWindowExA(0, "BUTTON", "Очистити журнал",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 430, 445, 140, 28, hwnd,
        reinterpret_cast<HMENU>(kBtnClearHistory), instance_, nullptr);

    // 5. Нижній рядок стану (Зворотний зв'язок системи з користувачем)
    lbl_status_ = CreateWindowExA(
        WS_EX_STATICEDGE, "STATIC", " Готово до роботи.",
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 20, 490, 565, 24, hwnd, nullptr,
        instance_, nullptr);
}

void PaymentGuiWindow::applyModernFont() {
    ui_font_ = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    header_font_ = CreateFontA(21, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    balance_font_ = CreateFontA(26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Segoe UI");

    // Застосовуємо базовий шрифт до всіх дочірніх вікон
    HWND child = GetWindow(main_hwnd_, GW_CHILD);
    while (child) {
        SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(ui_font_), TRUE);
        child = GetWindow(child, GW_HWNDNEXT);
    }

    // Виділяємо ключові елементи (заголовок та суму балансу)
    HWND title = GetWindow(main_hwnd_, GW_CHILD);
    if (title) {
        SendMessageA(title, WM_SETFONT, reinterpret_cast<WPARAM>(header_font_),
            TRUE);
    }
    SendMessageA(lbl_balance_, WM_SETFONT, reinterpret_cast<WPARAM>(balance_font_),
        TRUE);
}

void PaymentGuiWindow::refreshAccountDisplay() {
    SetWindowTextA(lbl_owner_, "Власник: Іван Петренко (ПриватБанк)");
    std::string acc_text = "Рахунок: " + account_.getAccountNumber();
    SetWindowTextA(lbl_acc_number_, acc_text.c_str());

    std::string bal_text = formatMoney(account_.getBalance());
    SetWindowTextA(lbl_balance_, bal_text.c_str());
}

bool PaymentGuiWindow::parseInputAmount(double* out_amount) {
    char buf[64] = {};
    GetWindowTextA(edit_amount_, buf, sizeof(buf));
    std::string text(buf);

    if (text.empty()) {
        MessageBoxA(main_hwnd_, "Поле суми не може бути порожнім!",
            "Помилка вводу", MB_OK | MB_ICONWARNING);
        SetFocus(edit_amount_);
        return false;
    }

    try {
        size_t processed = 0;
        double val = std::stod(text, &processed);
        if (processed != text.length() || val <= 0.0) {
            throw std::invalid_argument("Invalid number");
        }
        *out_amount = val;
        return true;
    }
    catch (...) {
        MessageBoxA(main_hwnd_,
            "Введіть коректне додатне число (наприклад: 500 або 250.50).",
            "Некоректна сума", MB_OK | MB_ICONERROR);
        SetFocus(edit_amount_);
        return false;
    }
}

void PaymentGuiWindow::recordTransaction(const std::string& type, double amount,
    double commission,
    const std::string& description) {
    Transaction tx(next_tx_id_++, "20.11.2026", amount, type, "UAH", commission,
        "Виконано", description);
    history_.addTransaction(tx);

    std::ostringstream line;
    line << "#" << (next_tx_id_ - 1) << " | " << type << " | " << std::fixed
        << std::setprecision(2) << amount << " UAH";
    if (commission > 0.0) {
        line << " (комісія " << commission << ")";
    }
    line << " — " << description;

    // Додаємо новий запис на початок списку, щоб останні дії були зверху (UX)
    SendMessageA(list_history_, LB_INSERTSTRING, 0,
        reinterpret_cast<LPARAM>(line.str().c_str()));
}

void PaymentGuiWindow::updateStatus(const std::string& status_text) {
    std::string full_status = " " + status_text;
    SetWindowTextA(lbl_status_, full_status.c_str());
}