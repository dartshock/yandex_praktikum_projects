#include "cell.h"

#include <cassert>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

#include "sheet.h"

class Cell::Impl {
public:
    virtual ~Impl() = default;

    virtual std::string GetText() const = 0;
    virtual Value GetValue(const SheetInterface& sheet) const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

class Cell::EmptyImpl : public Impl {
public:
    std::string GetText() const noexcept override {
        return std::string{};
    }

    Value GetValue(const SheetInterface& sheet) const noexcept override {
        return std::string{};
    }

    std::vector<Position> GetReferencedCells() const override {
        return std::vector<Position>{};
    }
};

class Cell::TextImpl : public Impl {
public:
    explicit TextImpl(std::string text)
        : text_(std::move(text)) {
    }

    std::string GetText() const override {
        return text_;
    }

    Value GetValue(const SheetInterface& sheet) const override {
        return (text_.front() == ESCAPE_SIGN) ? text_.substr(1) : text_;
    }

    std::vector<Position> GetReferencedCells() const override {
        return std::vector<Position>{};
    }

private:
    const std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
    explicit FormulaImpl(std::string expression)
        : formula_(ParseFormula(std::move(expression))) {
    }

    explicit FormulaImpl(std::unique_ptr<FormulaInterface> formula)
        : formula_(std::move(formula)) {
    }

    std::string GetText() const override {
        return '=' + formula_->GetExpression();
    }

    Value GetValue(const SheetInterface& sheet) const override {
        auto result = formula_->Evaluate(sheet);
        if (std::holds_alternative<double>(result)) {
            return std::get<double>(result);
        }
        return std::get<FormulaError>(result);
    }

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }

private:
    const std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(Sheet& sheet, Position pos)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet)
    , position_(pos) {
}

Cell::~Cell() = default;

void Cell::SetFormula(std::string&& formula) {
    auto parsed_formula = ParseFormula(std::move(formula));
    std::vector<Position> referenced_cells =
        parsed_formula->GetReferencedCells();
    if (HasCyclicDependency(referenced_cells)) {
        throw CircularDependencyException("Cyclic dependency");
    }
    impl_ = std::make_unique<FormulaImpl>(std::move(parsed_formula));

    cached_value_.reset();
    RemoveDependencies();
    if (IsReferenced()) {
        InvalidateCache();
    }

    SetNewDependencies(referenced_cells);
}

void Cell::Set(std::string text) {
    if (text == GetText()) {
        return;
    }

    if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        SetFormula(text.substr(1));
    } else {
        RemoveDependencies();
        cached_value_.reset();
        if (IsReferenced()) {
            InvalidateCache();
        }
        if (text.empty()) {
            impl_ = std::make_unique<EmptyImpl>();
        } else {
            impl_ = std::make_unique<TextImpl>(std::move(text));
        }
    }
}

void Cell::Clear() {
    Set(std::string{});
}

Cell::Value Cell::GetValue() const {
    if (!cached_value_.has_value()) {
        cached_value_ = impl_->GetValue(sheet_);
    }
    return *cached_value_;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return referenced_cells_;
}

bool Cell::IsReferenced() const {
    return !dependent_cells_.empty();
}

void Cell::RemoveDependent(Position pos) {
    auto iter = dependent_cells_.find(pos);
    if (iter != dependent_cells_.end()) {
        dependent_cells_.erase(iter);
    }
}

void Cell::RemoveDependencies() {
    for (const Position& ref_cell_pos : referenced_cells_) {
        Cell* ref_cell = sheet_.GetOrCreateCell(ref_cell_pos);
        if (ref_cell) {
            ref_cell->RemoveDependent(position_);
        }
    }
    referenced_cells_.clear();
}

void Cell::AddDependent(Position pos) {
    dependent_cells_.insert(pos);
}

void Cell::SetNewDependencies(
    const std::vector<Position>& new_referenced_cells) {
    for (const Position& ref_cell_pos : new_referenced_cells) {
        Cell* ref_cell = sheet_.GetOrCreateCell(ref_cell_pos);
        if (ref_cell) {
            ref_cell->AddDependent(position_);
        }
    }
    referenced_cells_ = new_referenced_cells;
}

void Cell::InvalidateCache() {
    std::unordered_set<Position, PositionHasher> invalidated_cells;
    std::deque<Position> cells_to_invalidate(dependent_cells_.begin(),
                                             dependent_cells_.end());

    // BFS обход
    while (!cells_to_invalidate.empty()) {
        // Извлекаем позицию ячейки из начала очереди
        Position current_cell_pos = cells_to_invalidate.front();
        cells_to_invalidate.pop_front();

        // Получаем указатель на текущую ячейку
        Cell* current_cell = sheet_.GetOrCreateCell(current_cell_pos);

        // Очищаем кэш ячейки
        current_cell->cached_value_.reset();

        // Добавляем позиции, зависимые от текущей ячейки в очередь
        for (const Position& ref_pos : current_cell->dependent_cells_) {
            // Если эта зависимость ещё не посещалась
            auto it = invalidated_cells.find(ref_pos);
            if (it == invalidated_cells.end()) {
                cells_to_invalidate.push_back(ref_pos);
                invalidated_cells.insert(ref_pos);
            }
        }
    }
}

bool Cell::BFSInitialization(
    const std::vector<Position>& ref_cells,
    std::unordered_set<Position, PositionHasher>& visited_cells,
    std::deque<Position>& cells_to_check) const {
    for (const Position& ref_cell_pos : ref_cells) {
        if (ref_cell_pos == position_) {
            return true;
        }
        auto iter = visited_cells.find(ref_cell_pos);
        if (iter == visited_cells.end()) {
            cells_to_check.push_back(ref_cell_pos);
            visited_cells.insert(ref_cell_pos);
        }
    }
    return false;
}

bool Cell::BFSTraversal(
    std::unordered_set<Position, PositionHasher>& visited_cells,
    std::deque<Position>& cells_to_check) const {
    while (!cells_to_check.empty()) {
        // Извлекаем позицию ячейки из начала очереди
        auto current_cell_pos = cells_to_check.front();
        cells_to_check.pop_front();

        if (current_cell_pos == position_) {
            return true;
        }

        // Получаем зависимости текущей ячейки
        const CellInterface* current_cell = sheet_.GetCell(current_cell_pos);
        if (!current_cell) {
            continue;
        }  // Если ячейки нет, пропускаем

        auto current_cell_ref_cells = current_cell->GetReferencedCells();

        // Добавляем зависимости текущей ячейки в очередь
        for (const Position& ref_pos : current_cell_ref_cells) {
            auto iter = visited_cells.find(ref_pos);
            if (iter == visited_cells.end()) {
                cells_to_check.push_back(ref_pos);
                visited_cells.insert(ref_pos);
            }
        }
    }
    return false;
}

bool Cell::HasCyclicDependency(const std::vector<Position>& ref_cells) const {
    std::unordered_set<Position, PositionHasher> visited_cells;
    std::deque<Position> cells_to_check;

    if (BFSInitialization(ref_cells, visited_cells, cells_to_check)) {
        return true;
    }
    return BFSTraversal(visited_cells, cells_to_check);
}