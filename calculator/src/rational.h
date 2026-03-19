#pragma once

#include <cstdlib>
#include <iostream>
#include <numeric>

class Rational {
public:
    Rational() = default;
    Rational(int numerator)
        : numerator_(numerator)
        , denominator_(1) {}
    Rational(int numerator, int denominator)
        : numerator_(numerator)
        , denominator_(denominator) {
        if (denominator_ == 0) {
            std::abort();
        }
        Reduction();
    }
    Rational(const Rational& r)
        : numerator_(r.numerator_)
        , denominator_(r.denominator_) {}

    // Перегрузка унарного +
    Rational operator+() const { return *this; }

    // Перегрузка унарного -
    Rational operator-() const { return Rational{-numerator_, denominator_}; }

    // Перегрузка сложения
    Rational operator+(const Rational& r) const {
        Rational new_num = *this;
        return new_num+=r;
    }

    // Перегрузка вычитания
    Rational operator-(const Rational& r) const {
        Rational new_num = *this;
        return new_num -= r;
    }

    // Перегрузка умножения
    Rational operator*(const Rational& r) const {
        Rational new_num = *this;
        return new_num *= r;
    }

    // Перегрузка деления
    Rational operator/(const Rational& r) const {
        Rational new_num = *this;
        return new_num /= r;
    }

    // Перегрузка ввода
    friend std::istream& operator>>(std::istream& is, Rational& r);

    // Перегрузка вывода
    friend std::ostream& operator<<(std::ostream& os, const Rational& r);


    // Перегрузка присваивания через экземпляр класса
    Rational& operator=(const Rational& r) {
        numerator_ = r.numerator_;
        denominator_ = r.denominator_;
        return *this;
    }

    // Перегрузка сложения/присваивания
    Rational& operator+=(const Rational& r) {
        numerator_ = numerator_ * r.denominator_ + denominator_ * r.numerator_;
        denominator_ *= r.denominator_;
        Reduction();
        return *this;
    }

    // Перегрузка вычитания/присваивания
    Rational& operator-=(const Rational& r) { return *this += -r; }

    // Перегрузка умножения/присваивания
    Rational& operator*=(const Rational& r) {
        numerator_ *= r.numerator_;
        denominator_ *= r.denominator_;
        Reduction();
        return *this;
    }

    // Перегрузка деления/присваивания
    Rational& operator/=(const Rational& r) {
        numerator_ *= r.denominator_;
        denominator_ *= r.numerator_;
        Reduction();
        return *this;
    }

    // Метод возвращает числитель
    int GetNumerator() const { return numerator_; }

    // Метод возвращает знаменатель
    int GetDenominator() const { return denominator_; }

private:
    // Метод для приведения дроби к корректной форме
    void Reduction() {
        if (denominator_ < 0) {
            numerator_ = -numerator_;
            denominator_ = -denominator_;
        }
        const int divisor = std::gcd(numerator_, denominator_);
        numerator_ /= divisor;
        denominator_ /= divisor;
    }

private:
    int numerator_ = 0;
    int denominator_ = 1;
};

inline auto operator<=> (const Rational& lhs, const Rational& rhs) {
    return lhs.GetNumerator() * rhs.GetDenominator() <=>
           lhs.GetDenominator() * rhs.GetNumerator();
}

inline bool operator==(const Rational& lhs, const Rational& rhs) {
    return !(lhs < rhs) && !(rhs < lhs);
}

inline std::ostream& operator<<(std::ostream& os, const Rational& r) {
    using namespace std::literals;
    os << r.numerator_;
    if (r.denominator_ != 1) {
        os << " / "s << r.denominator_;
    }
    return os;
}

inline std::istream& operator>>(std::istream& is, Rational& r) {
    int n, d;
    char div;

    if (!(is >> n)) {
        return is;
    }
    if (!(is >> std::ws >> div)) {
        r = Rational(n, 1);
        is.clear();
        return is;
    }
    if (div != '/') {
        r = Rational(n, 1);
        is.unget();
        return is;
    }
    if (!(is >> d) || (d == 0)) {
        is.setstate(std::ios::failbit);
        return is;
    }
    r = Rational(n, d);

    return is;
}
