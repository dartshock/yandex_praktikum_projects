#pragma once

#include "enums.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void SetDigitKeyCallback(std::function<void(int key)> cb);

    void SetProcessOperationKeyCallback(std::function<void(Operation key)> cb);

    void SetProcessControlKeyCallback(std::function<void(ControlKey key)> cb);

    void SetControllerCallback(std::function<void(ControllerType controller)> cb);

    void SetInputText(const std::string& text);

    void SetErrorText(const std::string& text);

    void SetFormulaText(const std::string& text);

    void SetMemText(const std::string& text);

    void SetExtraKey(const std::optional<std::string>& key);

private slots:

    // Слот удаления ячейки памяти
    void on_pb_memory_clear_clicked();

    // Слот вызова ячейки памяти
    void on_pb_memory_recover_clicked();

    // Слот сохранения в ячейку памяти
    void on_pb_memory_save_clicked();

    // Слоты цифр 0-9
    void on_pb_dig_0_clicked();
    void on_pb_dig_1_clicked();
    void on_pb_dig_2_clicked();
    void on_pb_dig_3_clicked();
    void on_pb_dig_4_clicked();
    void on_pb_dig_5_clicked();
    void on_pb_dig_6_clicked();
    void on_pb_dig_7_clicked();
    void on_pb_dig_8_clicked();
    void on_pb_dig_9_clicked();

    // Слот точки
    void on_tb_extra_clicked();

    // Слот backspace
    void on_pb_backspace_clicked();

    // Слот равно
    void on_pb_equal_clicked();

    // Слот сложения
    void on_pb_addition_clicked();

    // Слот вычитания
    void on_pb_subtraction_clicked();

    // Слот умножения
    void on_pb_multiplication_clicked();

    // Слот деления
    void on_pb_division_clicked();

    // Слот степени
    void on_pb_power_clicked();

    // Слот очистки калькулятора
    void on_pb_clear_clicked();

    // Слот изменения знака
    void on_pb_change_sign_clicked();

    // Слот изменения типа числа
    void on_cmb_controller_currentIndexChanged(int index);

private:
    Ui::MainWindow* ui;
    std::function<void(Operation key)> operation_cb_;
    std::function<void(int key)> digit_cb_;
    std::function<void(ControlKey key)> control_cb_;
    std::function<void(ControllerType controller)> controller_cb_;


};
