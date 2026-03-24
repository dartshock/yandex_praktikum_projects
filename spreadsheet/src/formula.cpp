#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <string>
#include <variant>

#include "FormulaAST.h"

namespace {
class Formula : public FormulaInterface {
public:
    // Реализуйте следующие методы:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(std::move(expression))) {
    } catch (const std::exception& exc) {
        std::throw_with_nested(FormulaException(exc.what()));
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            auto get_cell_value = [&sheet](Position pos) {
                if (!pos.IsValid()) {
                    throw FormulaError(FormulaError::Category::Ref);
                }
                auto cell = sheet.GetCell(pos);
                if (cell == nullptr) {
                    return 0.0;
                }  // пустая ячейка = 0

                auto value = cell->GetValue();

                if (std::holds_alternative<double>(value)) {
                    return std::get<double>(value);
                }

                if (std::holds_alternative<std::string>(value)) {
                    std::string str = std::get<std::string>(value);
                    if (str.empty()) {
                        return 0.0;
                    }
                    try {
                        size_t pos = 0;
                        double value = std::stod(str, &pos);
                        if (pos != str.length()) {
                            throw FormulaError(FormulaError::Category::Value);
                        }
                        return value;
                    } catch (...) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }

                throw FormulaError(std::get<FormulaError>(value));
            };

            return ast_.Execute(get_cell_value);
        } catch (const FormulaError& exc) {
            return exc;
        }
    }
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        const auto& cells = ast_.GetCells();
        std::vector<Position> result(cells.begin(), cells.end());

        std::sort(result.begin(), result.end());

        auto last = std::unique(result.begin(), result.end());
        result.erase(last, result.end());

        return result;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}