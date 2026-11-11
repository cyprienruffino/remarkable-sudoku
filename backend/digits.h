// Handwritten digit recognition, shared by the backend and tools/train_digits.cpp
// so that training and recognition see images prepared the same way.
#ifndef DIGITS_H
#define DIGITS_H

#include <dlib/dnn.h>

#include <vector>

namespace digits {

// Same network as reCrossable's letter recognizer, with one output per digit
// from 1 to 9: sudoku never needs 0, and leaving it out avoids 0/6 mix-ups.
template <typename SUBNET> using block_a1 = dlib::relu<dlib::con<10,1,1,1,1,SUBNET>>;
template <typename SUBNET> using block_a2 = dlib::relu<dlib::con<10,3,3,1,1,dlib::relu<dlib::con<16,1,1,1,1,SUBNET>>>>;
template <typename SUBNET> using block_a3 = dlib::relu<dlib::con<10,5,5,1,1,dlib::relu<dlib::con<16,1,1,1,1,SUBNET>>>>;
template <typename SUBNET> using block_a4 = dlib::relu<dlib::con<10,1,1,1,1,dlib::max_pool<3,3,1,1,SUBNET>>>;
template <typename SUBNET> using incept_a = dlib::inception4<block_a1,block_a2,block_a3,block_a4,SUBNET>;
template <typename SUBNET> using block_b1 = dlib::relu<dlib::con<4,1,1,1,1,SUBNET>>;
template <typename SUBNET> using block_b2 = dlib::relu<dlib::con<4,3,3,1,1,SUBNET>>;
template <typename SUBNET> using block_b3 = dlib::relu<dlib::con<4,1,1,1,1,dlib::max_pool<3,3,1,1,SUBNET>>>;
template <typename SUBNET> using incept_b = dlib::inception3<block_b1,block_b2,block_b3,SUBNET>;

using Net = dlib::loss_multiclass_log<
    dlib::fc<9,
    dlib::relu<dlib::fc<32,
    dlib::max_pool<2,2,2,2,incept_b<
    dlib::max_pool<2,2,2,2,incept_a<
    dlib::input<dlib::matrix<unsigned char>>
    >>>>>>>>;

using Image = dlib::matrix<unsigned char>;

// Turns a grayscale image with bright ink on black into the network's input,
// like MNIST: the ink's bounding box scaled to fit 20x20, centered in 28x28.
// Returns an empty image when there's no ink.
inline Image normalize(const std::vector<float> &ink, int width, int height)
{
    int left = width, top = height, right = -1, bottom = -1;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (ink[y * width + x] > 0.25f) {
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        }
    }
    if (right < 0) {
        return {};
    }

    // Keep the aspect ratio, so a 1 stays thin
    const float boxWidth = right - left + 1;
    const float boxHeight = bottom - top + 1;
    const float scale = 20.0f / std::max(boxWidth, boxHeight);
    const float offsetX = 14 - boxWidth * scale / 2;
    const float offsetY = 14 - boxHeight * scale / 2;

    auto sample = [&](float x, float y) {
        const int ix = int(std::floor(x));
        const int iy = int(std::floor(y));
        if (ix < 0 || iy < 0 || ix >= width || iy >= height) {
            return 0.0f;
        }
        return ink[iy * width + ix];
    };

    // Average 4x4 samples per output pixel, which works for both down- and upscaling
    Image image(28, 28);
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 28; x++) {
            float sum = 0;
            for (int sy = 0; sy < 4; sy++) {
                for (int sx = 0; sx < 4; sx++) {
                    const float ox = x + (sx + 0.5f) / 4;
                    const float oy = y + (sy + 0.5f) / 4;
                    sum += sample(left + (ox - offsetX) / scale, top + (oy - offsetY) / scale);
                }
            }
            image(y, x) = (unsigned char)std::clamp(sum / 16 * 255.0f, 0.0f, 255.0f);
        }
    }
    return image;
}

} // namespace digits

#endif // DIGITS_H
