# Sudoku

Sudoku for the reMarkable Paper Pro family, as an [AppLoad](https://github.com/asivery/rm-appload) app.
Tap a square, then a number, with the pen or a finger.

- Three levels, all with a unique solution:
  - Easy: 36 givens, solvable by filling squares that have a single possible number
  - Normal: as few givens as possible, still solvable that way
  - Hard: as few givens as possible, needing more than that
- Duplicates in a row, column or box are circled; Check strikes out wrong numbers.
- The game in progress is saved in `~/.local/share/sudoku/game` and resumed
  when the app opens again.

The backend (`backend/`) generates puzzles and saves the game. The frontend
(`ui/main.qml`) is loaded by AppLoad inside xochitl.

## Building

In `nix develop` (from [nix-remarkable-sdk](https://github.com/cyprienruffino/nix-remarkable-sdk)):

```sh
# On the PC: tests, and the app with its backend
cmake -B build-pc -G Ninja && cmake --build build-pc && ctest --test-dir build-pc
cmake --install build-pc --prefix dist-pc/sudoku
appload-run dist-pc/sudoku
build-pc/entry --generate hard

# On the tablet
cmake -B build-rm -G Ninja -DCMAKE_TOOLCHAIN_FILE=$RM_TOOLCHAIN_FILE && cmake --build build-rm
cmake --install build-rm --prefix dist/sudoku
rm-deploy dist/sudoku
```
