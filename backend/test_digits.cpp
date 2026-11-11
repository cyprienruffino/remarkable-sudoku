// Checks the digit recognizer on synthetic pen strokes, drawn with random size,
// slant and position: test_digits data/digits.dat
#include "recognizer.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <random>

namespace {

using Shape = std::vector<std::vector<Point>>; // strokes in a unit box, y down

std::vector<Point> arc(float cx, float cy, float rx, float ry, float from, float to)
{
    std::vector<Point> points;
    const int steps = 24;
    for (int i = 0; i <= steps; i++) {
        const float angle = (from + (to - from) * i / steps) * float(M_PI) / 180;
        points.push_back({ cx + rx * std::cos(angle), cy + ry * std::sin(angle) });
    }
    return points;
}

std::vector<Point> join(std::vector<Point> a, const std::vector<Point> &b)
{
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

// How people commonly write each digit; angles are clockwise from 3 o'clock
const Shape shapes[10] = {
    {},
    { { { 0.35f, 0.2f }, { 0.55f, 0.0f }, { 0.55f, 1.0f } } },
    { join(arc(0.5f, 0.28f, 0.32f, 0.28f, 200, 380), { { 0.15f, 1.0f }, { 0.9f, 1.0f } }) },
    { join(arc(0.48f, 0.25f, 0.32f, 0.25f, 200, 450), arc(0.48f, 0.73f, 0.36f, 0.27f, 270, 510)) },
    { { { 0.6f, 0.0f }, { 0.1f, 0.65f }, { 0.9f, 0.65f } }, { { 0.65f, 0.3f }, { 0.65f, 1.0f } } },
    { { { 0.85f, 0.0f }, { 0.25f, 0.0f }, { 0.2f, 0.45f } }, join(arc(0.5f, 0.68f, 0.35f, 0.3f, 200, 450), {}) },
    { join(arc(0.55f, 0.5f, 0.4f, 0.5f, 290, 180), arc(0.5f, 0.7f, 0.35f, 0.3f, 180, 540)) },
    { { { 0.1f, 0.0f }, { 0.9f, 0.0f }, { 0.4f, 1.0f } } },
    { join(arc(0.5f, 0.25f, 0.28f, 0.25f, 90, -270), arc(0.5f, 0.74f, 0.34f, 0.26f, -90, 270)) },
    { join(arc(0.5f, 0.3f, 0.32f, 0.3f, 0, -360), { { 0.82f, 0.3f }, { 0.75f, 1.0f } }) },
};

} // namespace

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <network>" << std::endl;
        return 1;
    }
    DigitRecognizer recognizer;
    if (!recognizer.load(argv[1])) {
        return 1;
    }

    std::mt19937 random(7);
    std::uniform_real_distribution<float> size(0.45f, 0.8f), slant(-0.15f, 0.25f), aspect(0.6f, 1.0f), jitter(-1.0f, 1.0f);
    const int cellSize = 102; // the Move's cell size
    const int variants = 20;
    int correct = 0;
    for (int digit = 1; digit <= 9; digit++) {
        int digitCorrect = 0;
        std::string mistakes;
        for (int v = 0; v < variants; v++) {
            const float height = size(random) * cellSize;
            const float width = height * aspect(random) * 0.7f;
            const float s = slant(random);
            const float left = (cellSize - width) / 2 + jitter(random) * 8;
            const float top = (cellSize - height) / 2 + jitter(random) * 8;
            std::vector<Stroke> strokes;
            for (const auto &shapeStroke : shapes[digit]) {
                Stroke stroke;
                for (const Point &p : shapeStroke) {
                    stroke.push_back({ left + (p.x + s * (1 - p.y)) * width, top + p.y * height });
                }
                strokes.push_back(stroke);
            }
            int recognized = 0;
            recognizer.recognize(strokes, cellSize, &recognized);
            if (recognized == digit) {
                digitCorrect++;
            } else {
                mistakes += ' ' + std::to_string(recognized);
            }
        }
        std::cout << digit << ": " << digitCorrect << "/" << variants << (mistakes.empty() ? "" : "  read as" + mistakes) << std::endl;
        correct += digitCorrect;
    }
    const double accuracy = double(correct) / (9 * variants);
    std::cout << "Accuracy: " << accuracy << std::endl;
    return accuracy >= 0.9 ? 0 : 1;
}
