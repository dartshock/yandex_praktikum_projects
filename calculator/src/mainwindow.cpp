#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::SetDigitKeyCallback(std::function<void(int)> cb) {
    if (cb) {
        digit_cb_ = cb;
    }
}

void MainWindow::SetProcessOperationKeyCallback(std::function<void(Operation)> cb) {
    if (cb) {
        operation_cb_ = cb;
    }
}

void MainWindow::SetProcessControlKeyCallback(
    std::function<void(ControlKey)> cb) {
    if (cb) {
        control_cb_ = cb;
    }
}

void MainWindow::SetControllerCallback(std::function<void(ControllerType)> cb) {
    if (cb) {
        controller_cb_ = cb;
    }
}

void MainWindow::SetInputText(const std::string& text) {
    ui->l_result->setStyleSheet("");
    ui->l_result->setText(QString::fromStdString(text));
}

void MainWindow::SetErrorText(const std::string& text) {
    ui->l_result->setStyleSheet("color: red;");
    ui->l_result->setText(QString::fromStdString(text));
}

void MainWindow::SetFormulaText(const std::string& text) {
    ui->l_formula->setText(QString::fromStdString(text));
}

void MainWindow::SetMemText(const std::string& text) {
    ui->l_memory->setText(QString::fromStdString(text));
}

void MainWindow::SetExtraKey(const std::optional<std::string>& key) {
    if(!key.has_value()){
        ui->tb_extra->hide();
        return;
    }
    ui->tb_extra->setText(QString::fromStdString(*key));
    ui->tb_extra->show();
}

void MainWindow::on_pb_dig_0_clicked() {
    if (digit_cb_) {
        digit_cb_(0);
    }
}

void MainWindow::on_pb_dig_1_clicked() {
    if (digit_cb_) {
        digit_cb_(1);
    }
}

void MainWindow::on_pb_dig_2_clicked() {
    if (digit_cb_) {
        digit_cb_(2);
    }
}

void MainWindow::on_pb_dig_3_clicked() {
    if (digit_cb_) {
        digit_cb_(3);
    }
}

void MainWindow::on_pb_dig_4_clicked() {
    if (digit_cb_) {
        digit_cb_(4);
    }
}

void MainWindow::on_pb_dig_5_clicked() {
    if (digit_cb_) {
        digit_cb_(5);
    }
}

void MainWindow::on_pb_dig_6_clicked() {
    if (digit_cb_) {
        digit_cb_(6);
    }
}

void MainWindow::on_pb_dig_7_clicked() {
    if (digit_cb_) {
        digit_cb_(7);
    }
}

void MainWindow::on_pb_dig_8_clicked() {
    if (digit_cb_) {
        digit_cb_(8);
    }
}

void MainWindow::on_pb_dig_9_clicked() {
    if (digit_cb_) {
        digit_cb_(9);
    }
}

void MainWindow::on_pb_addition_clicked() {
    if (operation_cb_) {
        operation_cb_(Operation::ADDITION);
    }
}

void MainWindow::on_pb_subtraction_clicked() {
    if (operation_cb_) {
        operation_cb_(Operation::SUBTRACTION);
    }
}

void MainWindow::on_pb_multiplication_clicked() {
    if (operation_cb_) {
        operation_cb_(Operation::MULTIPLICATION);
    }
}

void MainWindow::on_pb_division_clicked() {
    if (operation_cb_) {
        operation_cb_(Operation::DIVISION);
    }
}

void MainWindow::on_pb_power_clicked() {
    if (operation_cb_) {
        operation_cb_(Operation::POWER);
    }
}

void MainWindow::on_pb_clear_clicked() {
    if (control_cb_) {
        control_cb_(ControlKey::CLEAR);
    }
}

void MainWindow::on_tb_extra_clicked() {
    if (control_cb_) {
        control_cb_(ControlKey::EXTRA_KEY);
    }
}

void MainWindow::on_pb_backspace_clicked() {
    if (control_cb_) {
        control_cb_(ControlKey::BACKSPACE);
    }
}

void MainWindow::on_pb_equal_clicked() {
    if (control_cb_) {
        control_cb_(ControlKey::EQUALS);
    }
}

void MainWindow::on_pb_memory_clear_clicked() {
    if (control_cb_) {
        control_cb_(ControlKey::MEM_CLEAR);
    }
}

void MainWindow::on_pb_memory_recover_clicked() {
    if (control_cb_) {
        control_cb_(ControlKey::MEM_LOAD);
    }
}

void MainWindow::on_pb_memory_save_clicked() {
    if (control_cb_) {
        control_cb_(ControlKey::MEM_SAVE);
    }
}

void MainWindow::on_pb_change_sign_clicked() {
    if (control_cb_) {
        control_cb_(ControlKey::PLUS_MINUS);
    }
}

void MainWindow::on_cmb_controller_currentIndexChanged(int index) {
    if (controller_cb_) {
        controller_cb_(static_cast<ControllerType>(index));
    }
}
