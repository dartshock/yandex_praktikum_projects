#include "sheet.h"

#include <algorithm>
#include <memory>
#include <unordered_set>

#include "cell.h"
#include "common.h"

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    Cell* cell_ptr = GetOrCreateCell(pos);
    cell_ptr->Set(std::move(text));
}

CellInterface* Sheet::GetCell(Position pos) {
    if (CellExists(pos)) {
        return data_[pos.row][pos.col].get();
    }
    return nullptr;
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!CellExists(pos)) {
        return;
    }

    if (!data_[pos.row][pos.col]->IsReferenced()) {
        data_[pos.row][pos.col].reset();
    } else {
        data_[pos.row][pos.col]->Clear();
    }
    cells_in_row_[pos.row]--;
    cells_in_col_[pos.col]--;

    int new_cols = GetPrintableCols();
    int new_rows = GetPrintableRows();
    if (new_rows != size_.rows || new_cols != size_.cols) {
        UpdatePrintableSize({new_rows, new_cols});
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintImpl(output, [](const CellPtr& cell) { return cell->GetValue(); });
}
void Sheet::PrintTexts(std::ostream& output) const {
    PrintImpl(output, [](const CellPtr& cell) { return cell->GetText(); });
}

bool Sheet::CellExists(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }

    return pos.row < size_.rows && pos.col < size_.cols;
}

Cell* Sheet::GetOrCreateCell(Position pos) {
    auto cell_ptr = GetCell(pos);
    if (cell_ptr) {
        return static_cast<Cell*>(cell_ptr);
    }
    int new_rows = std::max(size_.rows, pos.row + 1);
    int new_cols = std::max(size_.cols, pos.col + 1);

    if (new_rows != size_.rows || new_cols != size_.cols) {
        UpdatePrintableSize({new_rows, new_cols});
    }

    data_[pos.row][pos.col] = std::make_unique<Cell>(*this, pos);

    cells_in_row_[pos.row]++;
    cells_in_col_[pos.col]++;

    return data_[pos.row][pos.col].get();
}

void Sheet::UpdatePrintableSize(Size new_size) {
    data_.resize(new_size.rows);
    for (auto& row : data_) {
        row.resize(new_size.cols);
    }

    cells_in_row_.resize(new_size.rows);
    cells_in_col_.resize(new_size.cols);

    size_ = new_size;
}

int Sheet::GetPrintableCellsInRange(const CellsNum& range) const {
    auto it = std::find_if(range.rbegin(), range.rend(),
                           [](size_t value) { return value != 0; });

    return std::distance(it, range.rend());
}

int Sheet::GetPrintableRows() const {
    return GetPrintableCellsInRange(cells_in_row_);
}

int Sheet::GetPrintableCols() const {
    return GetPrintableCellsInRange(cells_in_col_);
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}