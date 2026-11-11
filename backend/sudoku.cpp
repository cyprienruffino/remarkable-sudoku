#include "sudoku.h"

#include <algorithm>
#include <bit>
#include <numeric>

namespace sudoku {

namespace {

constexpr int allDigits = 0x3fe; // bits 1 to 9

int row(int cell) { return cell / 9; }
int column(int cell) { return cell % 9; }
int box(int cell) { return (cell / 27) * 3 + (cell % 9) / 3; }

// Digits already used in each row, column and box
struct Constraints
{
    std::array<int, 9> rows = {};
    std::array<int, 9> columns = {};
    std::array<int, 9> boxes = {};

    explicit Constraints(const Grid &grid)
    {
        for (int cell = 0; cell < 81; cell++) {
            if (grid[cell]) {
                set(cell, grid[cell]);
            }
        }
    }

    int candidates(int cell) const
    {
        return allDigits & ~(rows[row(cell)] | columns[column(cell)] | boxes[box(cell)]);
    }

    void set(int cell, int digit)
    {
        rows[row(cell)] |= 1 << digit;
        columns[column(cell)] |= 1 << digit;
        boxes[box(cell)] |= 1 << digit;
    }

    void unset(int cell, int digit)
    {
        rows[row(cell)] &= ~(1 << digit);
        columns[column(cell)] &= ~(1 << digit);
        boxes[box(cell)] &= ~(1 << digit);
    }
};

// Backtracking on the cell with the fewest candidates. With a random number
// generator, fills the grid with a random solution and stops at the first one.
int search(Grid &grid, Constraints &constraints, int limit, std::mt19937 *random)
{
    int best = -1;
    int bestCandidates = 0;
    int bestCount = 10;
    for (int cell = 0; cell < 81; cell++) {
        if (grid[cell]) {
            continue;
        }
        const int candidates = constraints.candidates(cell);
        const int count = std::popcount(unsigned(candidates));
        if (count == 0) {
            return 0;
        }
        if (count < bestCount) {
            best = cell;
            bestCandidates = candidates;
            bestCount = count;
        }
    }
    if (best == -1) {
        return 1;
    }

    std::array<int, 9> digits;
    int n = 0;
    for (int digit = 1; digit <= 9; digit++) {
        if (bestCandidates & (1 << digit)) {
            digits[n++] = digit;
        }
    }
    if (random) {
        std::shuffle(digits.begin(), digits.begin() + n, *random);
    }

    int found = 0;
    for (int i = 0; i < n && found < limit; i++) {
        grid[best] = digits[i];
        constraints.set(best, digits[i]);
        found += search(grid, constraints, limit - found, random);
        if (random && found) {
            return found;
        }
        constraints.unset(best, digits[i]);
        grid[best] = 0;
    }
    return found;
}

Grid randomSolution(std::mt19937 &random)
{
    Grid grid = {};
    Constraints constraints(grid);
    search(grid, constraints, 1, &random);
    return grid;
}

int givens(const Grid &grid)
{
    return std::count_if(grid.begin(), grid.end(), [](int digit) { return digit != 0; });
}

// Removes givens in random order as long as the puzzle stays unique and,
// if requested, solvable with singles. Stops at minGivens.
Grid dig(const Grid &solution, bool keepSingles, int minGivens, std::mt19937 &random)
{
    Grid grid = solution;
    std::array<int, 81> order;
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), random);

    int remaining = 81;
    for (const int cell : order) {
        if (remaining <= minGivens) {
            break;
        }
        const int digit = grid[cell];
        grid[cell] = 0;
        if (countSolutions(grid) != 1 || (keepSingles && !solvableWithSingles(grid))) {
            grid[cell] = digit;
        } else {
            remaining--;
        }
    }
    return grid;
}

} // namespace

int countSolutions(Grid grid, int limit)
{
    Constraints constraints(grid);
    // A given that clashes with another one makes the grid unsolvable
    for (int cell = 0; cell < 81; cell++) {
        if (grid[cell]) {
            const int digit = grid[cell];
            constraints.unset(cell, digit);
            if (!(constraints.candidates(cell) & (1 << digit))) {
                return 0;
            }
            constraints.set(cell, digit);
        }
    }
    return search(grid, constraints, limit, nullptr);
}

bool solvableWithSingles(Grid grid)
{
    Constraints constraints(grid);
    bool progress = true;
    while (progress) {
        progress = false;

        // Naked singles: cells with a single candidate
        for (int cell = 0; cell < 81; cell++) {
            if (grid[cell]) {
                continue;
            }
            const int candidates = constraints.candidates(cell);
            if (candidates == 0) {
                return false;
            }
            if (std::popcount(unsigned(candidates)) == 1) {
                const int digit = std::countr_zero(unsigned(candidates));
                grid[cell] = digit;
                constraints.set(cell, digit);
                progress = true;
            }
        }

        // Hidden singles: digits with a single possible cell in a row, column or box
        for (int unit = 0; unit < 27; unit++) {
            for (int digit = 1; digit <= 9; digit++) {
                int place = -1;
                int places = 0;
                for (int i = 0; i < 9; i++) {
                    const int cell = unit < 9 ? unit * 9 + i
                        : unit < 18 ? i * 9 + (unit - 9)
                        : ((unit - 18) / 3) * 27 + ((unit - 18) % 3) * 3 + (i / 3) * 9 + i % 3;
                    if (grid[cell] == digit) {
                        places = -1;
                        break;
                    }
                    if (!grid[cell] && (constraints.candidates(cell) & (1 << digit))) {
                        place = cell;
                        places++;
                    }
                }
                if (places == 1) {
                    grid[place] = digit;
                    constraints.set(place, digit);
                    progress = true;
                }
            }
        }
    }
    return givens(grid) == 81;
}

Puzzle generate(Level level, std::mt19937 &random)
{
    switch (level) {
    case Level::Easy: {
        const Grid solution = randomSolution(random);
        return { dig(solution, true, 36, random), solution };
    }
    case Level::Normal: {
        const Grid solution = randomSolution(random);
        return { dig(solution, true, 0, random), solution };
    }
    case Level::Hard:
        // Most minimal puzzles can still be solved with singles; keep trying
        // until one can't, which usually takes a few attempts
        for (int attempt = 0;; attempt++) {
            const Grid solution = randomSolution(random);
            const Grid puzzle = dig(solution, false, 0, random);
            if (!solvableWithSingles(puzzle) || attempt == 50) {
                return { puzzle, solution };
            }
        }
    }
    return {};
}

std::string toString(const Grid &grid)
{
    std::string text;
    for (const int digit : grid) {
        text += char('0' + digit);
    }
    return text;
}

} // namespace sudoku
