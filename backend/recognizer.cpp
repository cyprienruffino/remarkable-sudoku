#include "recognizer.h"
#include "digits.h"

#include <cmath>
#include <iostream>

struct DigitRecognizer::Network
{
    digits::Net net;
};

DigitRecognizer::DigitRecognizer() = default;
DigitRecognizer::~DigitRecognizer() = default;

bool DigitRecognizer::load(const std::string &netPath)
{
    try {
        auto network = std::make_unique<Network>();
        dlib::deserialize(netPath) >> network->net;
        m_net = std::move(network);
    } catch (const std::exception &e) {
        std::cerr << "Failed to load " << netPath << ": " << e.what() << std::endl;
        return false;
    }
    return true;
}

// Draws the strokes as antialiased ink, 1 being full ink
static std::vector<float> rasterize(const std::vector<Stroke> &strokes, int size, float penWidth)
{
    std::vector<float> ink(size * size, 0.0f);
    const float radius = penWidth / 2;

    auto stamp = [&](float cx, float cy) {
        const int x0 = std::max(0, int(std::floor(cx - radius - 1)));
        const int x1 = std::min(size - 1, int(std::ceil(cx + radius + 1)));
        const int y0 = std::max(0, int(std::floor(cy - radius - 1)));
        const int y1 = std::min(size - 1, int(std::ceil(cy + radius + 1)));
        for (int y = y0; y <= y1; y++) {
            for (int x = x0; x <= x1; x++) {
                const float distance = std::hypot(x + 0.5f - cx, y + 0.5f - cy);
                const float coverage = std::clamp(radius + 0.5f - distance, 0.0f, 1.0f);
                ink[y * size + x] = std::max(ink[y * size + x], coverage);
            }
        }
    };

    for (const Stroke &stroke : strokes) {
        for (size_t i = 0; i < stroke.size(); i++) {
            const Point &b = stroke[i];
            const Point &a = i > 0 ? stroke[i - 1] : b;
            const int steps = std::max(1, int(std::ceil(std::hypot(b.x - a.x, b.y - a.y) * 2)));
            for (int s = 0; s <= steps; s++) {
                const float t = float(s) / steps;
                stamp(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
            }
        }
    }
    return ink;
}

DigitRecognizer::Result DigitRecognizer::recognize(const std::vector<Stroke> &strokes, int cellSize, int *digit)
{
    if (!m_net || cellSize < 8 || strokes.empty()) {
        return Result::Empty;
    }

    // About as thick, relative to the digit, as MNIST's strokes
    const std::vector<float> ink = rasterize(strokes, cellSize, std::max(3.0f, cellSize * 0.07f));

    float covered = 0;
    for (const float value : ink) {
        covered += value;
    }
    // The user has scribbled over the cell to erase it
    if (covered > cellSize * cellSize / 4.0f) {
        return Result::Clear;
    }

    const digits::Image input = digits::normalize(ink, cellSize, cellSize);
    if (input.size() == 0) {
        return Result::Empty;
    }
    // A dot or a tap isn't a digit
    int inkedPixels = 0;
    for (const float value : ink) {
        inkedPixels += value > 0.5f;
    }
    if (inkedPixels < cellSize * cellSize / 200) {
        return Result::Empty;
    }

    *digit = int(m_net->net(input)) + 1;
    return Result::Digit;
}
