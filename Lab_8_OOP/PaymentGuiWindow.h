#pragma once
#ifndef PAYMENT_GUI_WINDOW_H_
#define PAYMENT_GUI_WINDOW_H_

#include <windows.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "PaymentSystem.h"

// Клас графічного інтерфейсу користувача (GUI) для системи «Платежі».
// Реалізований за принципами UX/UI: групування елементів, візуальна ієрархія,
// миттєвий зворотний зв'язок (статус-бар) та захист від помилок вводу.
class PaymentGuiWindow {
public:
    PaymentGuiWindow();
    ~PaymentGuiWindow();

    // Створення та відображення головного вікна програми
    bool create(HINSTANCE instance, int cmd_show);

    // Головний цикл обробки повідомлень Windows
    int runMessageLoop();

private:
    // Ідентифікатори елементів керування (UX Controls)
    enum ControlId {
        kEditAmount = 101,
        kEditRecipient = 102,
        kBtnDeposit = 201,
        kBtnWithdraw = 202,
        kBtnTransfer = 203,
        kBtnBonus = 204,
        kBtnClearHistory = 205,
        kListHistory = 301
    };

    // Статична віконна процедура Win32, що делегує події екземпляру класу
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam,
        LPARAM lparam);

    // Обробник подій графічного інтерфейсу
    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    // Побудова всіх елементів інтерфейсу за правилами UX/UI
    void initializeControls(HWND hwnd);

    // Застосування єдиного сучасного шрифту Segoe UI до всіх елементів
    void applyModernFont();

    // Оновлення даних рахунку та журналу транзакцій на екрані
    void refreshAccountDisplay();

    // Зчитування та валідація суми з поля вводу (захист від помилок користувача)
    bool parseInputAmount(double* out_amount);

    // Додавання запису в журнал транзакцій та список ListBox
    void recordTransaction(const std::string& type, double amount,
        double commission, const std::string& description);

    // Відображення повідомлення у нижньому рядку стану (UX зворотний зв'язок)
    void updateStatus(const std::string& status_text);

    // Поля стану вікна та елементів керування
    HINSTANCE instance_;
    HWND main_hwnd_;
    HFONT ui_font_;
    HFONT header_font_;
    HFONT balance_font_;

    // Елементи інтерфейсу (Controls)
    HWND lbl_owner_;
    HWND lbl_acc_number_;
    HWND lbl_balance_;
    HWND edit_amount_;
    HWND edit_recipient_;
    HWND list_history_;
    HWND lbl_status_;

    // Об'єкти предметної області з лабораторних робіт №3–7
    BankAccount account_;
    PaymentHistory history_;
    int next_tx_id_;
};

#endif  // PAYMENT_GUI_WINDOW_H_