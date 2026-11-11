// Checks generated puzzles against what each level promises: ctest --test-dir <build>
#include "sudoku.h"

#include <iostream>

using namespace sudoku;

static int failures = 0;

static void check(bool condition, const char *what, const Puzzle &puzzle)
{
    if (!condition) {
        std::cerr << "FAIL: " << what << "\n  " << toString(puzzle.givens) << std::endl;
        failures++;
    }
}

int main()
{
    std::mt19937 random(42);
    int hardWithSingles = 0;
    for (const Level level : { Level::Easy, Level::Normal, Level::Hard }) {
        for (int i = 0; i < 100; i++) {
            const Puzzle puzzle = generate(level, random);
            int givens = 0;
            bool matchesSolution = true;
            for (int cell = 0; cell < 81; cell++) {
                givens += puzzle.givens[cell] != 0;
                matchesSolution &= !puzzle.givens[cell] || puzzle.givens[cell] == puzzle.solution[cell];
            }
            check(matchesSolution, "givens match the solution", puzzle);
            check(countSolutions(puzzle.solution) == 1, "solution is valid", puzzle);
            check(countSolutions(puzzle.givens) == 1, "unique solution", puzzle);
            if (level == Level::Easy) {
                check(givens == 36, "easy has 36 givens", puzzle);
            }
            if (level != Level::Hard) {
                check(solvableWithSingles(puzzle.givens), "solvable with singles", puzzle);
            } else if (solvableWithSingles(puzzle.givens)) {
                hardWithSingles++;
            }
        }
    }
    // Hard falls back to a singles-solvable puzzle after 50 attempts, which should be rare
    if (hardWithSingles > 2) {
        std::cerr << "FAIL: " << hardWithSingles << "/100 hard puzzles are solvable with singles" << std::endl;
        failures++;
    }
    std::cout << (failures ? "FAILED" : "OK") << std::endl;
    return failures ? 1 : 0;
}
