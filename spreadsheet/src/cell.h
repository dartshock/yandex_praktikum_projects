#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "common.h"
#include "formula.h"

class Sheet;

class PositionHasher {
public:
    size_t operator()(const Position& pos) const {
        return static_cast<size_t>(pos.col + pos.row * 37);
    }
};

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, Position pos);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    // Добавляет формульную ячейку с проверкой на цикличность 
    void SetFormula(std::string&& formula);

    // Проходит по referenced_cells_ и удаляет себя из dependent_cells_ для
    // каждой ячейки
    void RemoveDependencies();

    // Удаляет позицию pos из dependent_cells_
    void RemoveDependent(Position pos);

    // Добавляет новые зависимости
    void SetNewDependencies(const std::vector<Position>& new_referenced_cells);

    // Добавляет позицию pos в dependent_cells_
    void AddDependent(Position pos);

    // Инвалидирует кэш у ячеек на которые влияет
    void InvalidateCache();

    // Проверяет циклические зависимости в формуле
    bool HasCyclicDependency(const std::vector<Position>& ref_cells) const;

    // Инициализация очереди: добавляем все прямые зависимости
    // стартовой ячейки. Возвращает true если при инициализации обнаружена
    // циклическая зависимость
    bool BFSInitialization(
        const std::vector<Position>& ref_cells,
        std::unordered_set<Position, PositionHasher>& visited_cells,
        std::deque<Position>& cells_to_check) const;

    // Обход очереди. Возвращает true если обнаружена циклическая зависимость
    bool BFSTraversal(
        std::unordered_set<Position, PositionHasher>& visited_cells,
        std::deque<Position>& cells_to_check) const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;

    Sheet& sheet_;
    const Position position_;

    // Ячейки, от которых зависит эта. Берутся из формулы.
    std::vector<Position> referenced_cells_;

    // Ячейки, на которые влияет эта. Инвалидируются при изменении этой ячейки.
    std::unordered_set<Position, PositionHasher> dependent_cells_;

    mutable std::optional<Value> cached_value_;
};