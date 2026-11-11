#ifndef RECOGNIZER_H
#define RECOGNIZER_H

#include <memory>
#include <string>
#include <vector>

struct Point
{
    float x;
    float y;
};
using Stroke = std::vector<Point>;

class DigitRecognizer
{
public:
    enum class Result { Digit, Empty, Clear };

    DigitRecognizer();
    ~DigitRecognizer();

    bool load(const std::string &netPath);

    // Strokes are in cell-local coordinates, the cell being cellSize x cellSize
    Result recognize(const std::vector<Stroke> &strokes, int cellSize, int *digit);

private:
    struct Network;
    std::unique_ptr<Network> m_net;
};

#endif // RECOGNIZER_H
