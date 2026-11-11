#ifndef SUDOKU_H
#define SUDOKU_H

#include <array>
#include <random>
#include <string>

namespace sudoku {

// 81 cells, row by row, 0 for empty
using Grid = std::array<int, 81>;

enum class Level { Easy, Normal, Hard };

struct Puzzle
{
    Grid givens;
    Grid solution;
};

// Counts solutions, stopping at `limit`
int countSolutions(Grid grid, int limit = 2);

// Whether the grid can be solved with naked and hidden singles only, which is
// what makes a puzzle easy: there's always a cell with a single possible digit
bool solvableWithSingles(Grid grid);

// Puzzles always have a unique solution.
//   Easy: 36 givens, solvable with singles
//   Normal: as few givens as possible, still solvable with singles
//   Hard: as few givens as possible, needing more than singles
Puzzle generate(Level level, std::mt19937 &random);

std::string toString(const Grid &grid);

} // namespace sudoku

#endif // SUDOKU_H
