// AppLoad backend for Sudoku: generates puzzles and keeps the game in
// progress saved, so it survives closing the app.

#include "appload.h"
#include "sudoku.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

// Must match ui/main.qml
constexpr int MSG_NEW_GAME = 1; // contents: easy, normal or hard
constexpr int MSG_SAVE = 2;     // contents: the 81 entered digits, 0 for empty
constexpr int MSG_READY = 100;  // no game in progress
constexpr int MSG_GAME = 101;   // contents: {"level", "givens", "solution", "entries"}

namespace {

struct Game
{
    std::string level;
    std::string givens;
    std::string solution;
    std::string entries;

    std::string toJson() const
    {
        return "{\"level\":\"" + level + "\",\"givens\":\"" + givens + "\",\"solution\":\"" + solution
            + "\",\"entries\":\"" + entries + "\"}";
    }
};

std::filesystem::path savePath()
{
    const char *home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / ".local/share/sudoku/game";
}

bool isGrid(const std::string &text)
{
    return text.size() == 81 && text.find_first_not_of("0123456789") == std::string::npos;
}

std::optional<Game> load()
{
    std::ifstream file(savePath());
    Game game;
    if (!std::getline(file, game.level) || !std::getline(file, game.givens)
        || !std::getline(file, game.solution) || !std::getline(file, game.entries)) {
        return std::nullopt;
    }
    if (!isGrid(game.givens) || !isGrid(game.solution) || !isGrid(game.entries)) {
        return std::nullopt;
    }
    return game;
}

void save(const Game &game)
{
    std::error_code error;
    std::filesystem::create_directories(savePath().parent_path(), error);
    std::ofstream file(savePath());
    file << game.level << '\n' << game.givens << '\n' << game.solution << '\n' << game.entries << '\n';
    if (!file) {
        std::cerr << "Failed to save the game to " << savePath() << std::endl;
    }
}

std::optional<sudoku::Level> parseLevel(const std::string &level)
{
    if (level == "easy") {
        return sudoku::Level::Easy;
    }
    if (level == "normal") {
        return sudoku::Level::Normal;
    }
    if (level == "hard") {
        return sudoku::Level::Hard;
    }
    return std::nullopt;
}

Game newGame(const std::string &levelName, sudoku::Level level, std::mt19937 &random)
{
    const auto start = std::chrono::steady_clock::now();
    const sudoku::Puzzle puzzle = sudoku::generate(level, random);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    std::cerr << "Generated a " << levelName << " puzzle in " << elapsed.count() << "ms" << std::endl;
    return { levelName, sudoku::toString(puzzle.givens), sudoku::toString(puzzle.solution), std::string(81, '0') };
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <appload socket path> | --generate <level>" << std::endl;
        return 1;
    }

    std::mt19937 random(std::random_device{}());

    // For testing the generator without a frontend
    if (std::string(argv[1]) == "--generate" && argc > 2) {
        const auto level = parseLevel(argv[2]);
        if (!level) {
            return 1;
        }
        const Game game = newGame(argv[2], *level, random);
        std::cout << game.toJson() << std::endl;
        return 0;
    }

    appload::Connection appload;
    if (!appload.connect(argv[1])) {
        perror("connect");
        return 1;
    }

    std::optional<Game> game = load();
    appload::Message message;
    while (appload.receive(&message)) {
        switch (message.type) {
        case appload::MSG_TERMINATE:
            return 0;
        case appload::MSG_NEW_FRONTEND:
            if (game) {
                appload.send(MSG_GAME, game->toJson());
            } else {
                appload.send(MSG_READY, "ready");
            }
            break;
        case MSG_NEW_GAME: {
            const auto level = parseLevel(message.contents);
            if (!level) {
                std::cerr << "Invalid level " << message.contents << std::endl;
                break;
            }
            game = newGame(message.contents, *level, random);
            save(*game);
            appload.send(MSG_GAME, game->toJson());
            break;
        }
        case MSG_SAVE:
            if (game && isGrid(message.contents)) {
                game->entries = message.contents;
                save(*game);
            }
            break;
        default:
            std::cerr << "Unknown message type " << message.type << std::endl;
        }
    }
    return 0;
}
