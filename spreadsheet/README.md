# Spreadsheet Lib

Библиотека для работы с электронными таблицами на C++17 с поддержкой формул и обнаружением циклических зависимостей.

## Возможности

- **Типы ячеек** — текст, формулы и пустые ячейки
- **Поддержка формул** — арифметические выражения с операциями +, -, *, /, скобками и ссылками на ячейки
- **Обработка ошибок** — возвращает #REF!, #VALUE! или #ARITHM! при некорректных операциях
- **Обнаружение циклов** — предотвращает циклические зависимости между формулами
- **Управление зависимостями** — автоматическая инвалидация кэша при изменении зависимых ячеек
- **Интеграция с ANTLR4** — грамматический разбор формул

## API

### SheetInterface
| Метод | Описание |
|-------|----------|
|`SetCell(Position, std::string)`| Задаёт содержимое ячейки (текст или формула) |
|`GetCell(Position)` | Возвращает указатель на ячейку или nullptr|
|`ClearCell(Position)` |	Очищает содержимое ячейки |
|`GetPrintableSize()` |	Возвращает размер области с непустыми ячейками |
|`PrintValues(std::ostream)` |	Выводит вычисленные значения |
|`PrintTexts(std::ostream)` |	Выводит исходный текст |

### FormulaInterface
| Метод | Описание |
|-------|----------|
|`Evaluate(const SheetInterface&)`|	Вычисляет значение формулы |
|`GetExpression()`|	Возвращает нормализованное выражение |
|`GetReferencedCells()`|	Возвращает отсортированный список зависимых ячеек |

### Обработка исключений
| Исключение | Описание |
|------------|----------|
|`#REF!`|	Некорректная ссылка на ячейку |
|`#VALUE!`|	Ячейка содержит текст, не являющийся числом |
|`#ARITHM!`|	Деление на ноль или переполнение |

## Тестирование

### Проект включает Unit-тесты:
* Преобразование позиций ячеек
* Базовые арифметические операции
* Ссылки на ячейки
* Обработка ошибочных ситуаций
* Обнаружение циклических зависимостей

## Сборка и использование

### Требования
- Компилятор с поддержкой C++17
- Conan для установки ANTRL4
- CMake 3.8+

### Сборка под Linux
```
# mkdir build && cd build
# conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
# cmake .. -DCMAKE_TOOLCHAIN_FILE="./generators/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
# cmake --build .
```

### Сборка под Windows
```
# mkdir build && cd build
# conan install .. --build=missing -s build_type=Release
# cmake .. -DCMAKE_TOOLCHAIN_FILE="./generators/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
# cmake --build .
```

### Пример использования

```
#include "sheet.h"
#include "formula.h"

auto sheet = CreateSheet();

// Заполнение ячеек
sheet->SetCell({0, 0}, "10");        // A1 = 10
sheet->SetCell({0, 1}, "20");        // B1 = 20  
sheet->SetCell({1, 0}, "=A1+B1");    // A2 = A1+B1

// Получение значений
auto cell = sheet->GetCell({1, 0});
auto value = cell->GetValue();       // 30.0
auto text = cell->GetText();         // "=A1+B1"

// Печать всей таблицы
sheet->PrintValues(std::cout);
```