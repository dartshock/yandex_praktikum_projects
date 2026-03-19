#pragma once

#include <cmath>
#include <optional>
#include <string>
#include "pow.h"
#include "rational.h"

using Error = std::string;
using namespace std::literals;

// Реализация шаблонного калькулятора.
template <typename Number>
class Calculator {
public:
    // Заменяет текущий результат на число n.
    void Set(Number n) { curr_num_ = n; }

    // Возвращает текущий результат вычислений калькулятора.
    auto GetNumber() const { return curr_num_; }

    // Возвращает наличие сохраненного в памяти числа
    bool GetHasMem() const { return mem_.has_value(); }

    // Сохраняет число в ячейку памяти
    void Save() { mem_ = curr_num_; }

    // Загружает число из ячейки памяти
    void Load() {
        if (mem_.has_value()) {
            curr_num_ = *mem_;
        }
    }

    // Прибавляет число n к текущему результату внутри калькулятора.
    std::optional<Error> Add(Number n) {
        curr_num_ += n;
        return std::nullopt;
    }

    // Вычитает число n из текущего результата.
    std::optional<Error> Sub(Number n) {
        curr_num_ -= n;
        return std::nullopt;
    }

    // Делит текущий результат на n.
    std::optional<Error> Div(Number n) {
        if constexpr (std::is_integral_v<Number> ||
                      std::is_same_v<Number, Rational>) {
            if (n == 0) {
                return "Division by zero"s;
            }
        }
        curr_num_ /= n;
        return std::nullopt;
    }

    // Умножает текущий результат на n.
    std::optional<Error> Mul(Number n) {
        curr_num_ *= n;
        return std::nullopt;
    }

    // Возводит текущий результат в степень n.
    std::optional<Error> Pow(Number n) {
        if constexpr (std::is_floating_point_v<Number>) {
            curr_num_ = std::pow(curr_num_, n);
        } else if constexpr (std::is_integral_v<Number>) {
            if (curr_num_ == 0 && n == 0) {
                return "Zero power to zero"s;
            } else if (n < 0) {
                return "Integer negative power"s;
            }
            curr_num_ = IntegerPow(curr_num_, n);
        } else if constexpr (std::is_same_v<Number, Rational>) {
            if (n.GetDenominator() != 1) {
                return "Fractional power is not supported"s;
            } else if (curr_num_ == 0 && n == 0) {
                return "Zero power to zero"s;
            }
            curr_num_ = ::Pow(curr_num_, n);
        }
        return std::nullopt;
    }

private:
    Number curr_num_ = 0;
    std::optional<Number> mem_;
};
