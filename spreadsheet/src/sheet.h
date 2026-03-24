#pragma once

#include <iostream>
#include <memory>
#include <variant>
#include <vector>

#include "common.h"
#include "cell.h"


inline std::ostream& operator<<(std::ostream& output,
                                const Cell::Value& value) {
    std::visit([&output](auto val) { output << val; }, value);
    return output;
}

class Sheet : public SheetInterface {
public:
    using CellPtr = std::unique_ptr<Cell>;
    using Row = std::vector<CellPtr>;
    using CellsNum = std::vector<size_t>;

    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    CellInterface* GetCell(Position pos) override;
    const CellInterface* GetCell(Position pos) const override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    Cell* GetOrCreateCell(Position pos);

private:
    bool CellExists(Position pos) const;

    void UpdatePrintableSize(Size new_size);

    int GetPrintableCellsInRange(const CellsNum& range) const;
    int GetPrintableRows() const;
    int GetPrintableCols() const;

    template <typename T>
    void PrintImpl(std::ostream& output, T&& func) const {
        for (int row = 0; row < size_.rows; ++row) {
            for (int col = 0; col < size_.cols; ++col) {
                output << ((data_[row][col] != nullptr) ? func(data_[row][col])
                                                        : "");
                output << (col == (size_.cols - 1) ? '\n' : '\t');
            }
        }
    }

private:
    std::vector<Row> data_;
    Size size_;
    CellsNum cells_in_row_;
    CellsNum cells_in_col_;
};