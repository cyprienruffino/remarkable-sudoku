// Trains the digit recognizer on MNIST (digits 1 to 9).
//
// Download and gunzip the four files from http://yann.lecun.com/exdb/mnist/
// (or a mirror) into a directory, then:
//     build-pc/train_digits <mnist directory> data/digits.dat [epochs]
//
// It saves its progress next to the output every minute and resumes from it,
// so it can be stopped and restarted, with fewer epochs to finish early.

#include "digits.h"

#include <dlib/data_io/mnist.h>

#include <iostream>

static void prepare(const std::vector<dlib::matrix<unsigned char>> &images, const std::vector<unsigned long> &labels,
                    std::vector<digits::Image> *outImages, std::vector<unsigned long> *outLabels)
{
    for (size_t i = 0; i < images.size(); i++) {
        if (labels[i] == 0) {
            continue;
        }
        const auto &image = images[i];
        std::vector<float> ink(image.size());
        for (long y = 0; y < image.nr(); y++) {
            for (long x = 0; x < image.nc(); x++) {
                ink[y * image.nc() + x] = image(y, x) / 255.0f;
            }
        }
        digits::Image normalized = digits::normalize(ink, image.nc(), image.nr());
        if (normalized.size() == 0) {
            continue;
        }
        outImages->push_back(normalized);
        outLabels->push_back(labels[i] - 1);
    }
}

static double accuracy(digits::Net &net, const std::vector<digits::Image> &images, const std::vector<unsigned long> &labels)
{
    const std::vector<unsigned long> predicted = net(images);
    size_t correct = 0;
    for (size_t i = 0; i < images.size(); i++) {
        correct += predicted[i] == labels[i];
    }
    return double(correct) / images.size();
}

int main(int argc, char *argv[]) try
{
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <mnist directory> <output network> [epochs]" << std::endl;
        return 1;
    }

    std::vector<dlib::matrix<unsigned char>> mnistTrainImages, mnistTestImages;
    std::vector<unsigned long> mnistTrainLabels, mnistTestLabels;
    dlib::load_mnist_dataset(argv[1], mnistTrainImages, mnistTrainLabels, mnistTestImages, mnistTestLabels);

    std::vector<digits::Image> trainImages, testImages;
    std::vector<unsigned long> trainLabels, testLabels;
    prepare(mnistTrainImages, mnistTrainLabels, &trainImages, &trainLabels);
    prepare(mnistTestImages, mnistTestLabels, &testImages, &testLabels);
    std::cout << trainImages.size() << " training and " << testImages.size() << " test digits" << std::endl;

    digits::Net net;
    dlib::dnn_trainer<digits::Net> trainer(net);
    trainer.set_learning_rate(0.01);
    trainer.set_min_learning_rate(0.00001);
    trainer.set_mini_batch_size(128);
    trainer.be_verbose();
    // Lets an interrupted training resume
    trainer.set_synchronization_file(std::string(argv[2]) + ".sync", std::chrono::minutes(1));
    // After loading the saved progress, which includes the previous settings
    trainer.set_max_num_epochs(argc == 4 ? std::stoul(argv[3]) : 10);
    trainer.train(trainImages, trainLabels);

    net.clean();
    dlib::serialize(argv[2]) << net;

    std::cout << "Training accuracy: " << accuracy(net, trainImages, trainLabels) << std::endl;
    std::cout << "Test accuracy: " << accuracy(net, testImages, testLabels) << std::endl;
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
}
