#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm> // For std::max

// Static random number generator
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<> dis(-1.0, 1.0); // For weights between -1 and 1

// Activation Functions
double relu(double x) {
    return std::max(0.0, x);
}

double dRelu(double x) {
    return x > 0 ? 1.0 : 0.0;
}

double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

double dSigmoid(double x) {
    double s = sigmoid(x);
    return s * (1.0 - s);
}

// Function to generate a random weight
double randWeight() {
    return dis(gen);
}

int main() {
    // XOR Data
    std::vector<std::vector<double>> inputs = {
        {0, 0},
        {0, 1},
        {1, 0},
        {1, 1}
    };
    std::vector<double> targets = {0, 1, 1, 0};

    // Network Architecture
    int inputSize = 2;
    int hiddenSize = 4;
    // int outputSize = 1; // Not explicitly used but good for clarity
    double learningRate = 0.1;

    // Weights and biases
    std::vector<std::vector<double>> W1(hiddenSize, std::vector<double>(inputSize));
    std::vector<double> B1(hiddenSize);
    std::vector<double> W2(hiddenSize);
    double B2 = randWeight();

    // Initialize weights and biases
    for (int i = 0; i < hiddenSize; ++i) {
        B1[i] = randWeight();
        W2[i] = randWeight();
        for (int j = 0; j < inputSize; ++j) {
            W1[i][j] = randWeight();
        }
    }

    // Training loop
    for (int epoch = 0; epoch < 10000; ++epoch) {
        double totalLoss = 0;

        for (int i = 0; i < inputs.size(); ++i) {
            const std::vector<double>& x = inputs[i];
            double y = targets[i];

            // === Forward Pass ===
            std::vector<double> Z1(hiddenSize);
            std::vector<double> A1(hiddenSize);

            for (int j = 0; j < hiddenSize; ++j) {
                Z1[j] = B1[j];
                for (int k = 0; k < inputSize; ++k) {
                    Z1[j] += W1[j][k] * x[k];
                }
                A1[j] = relu(Z1[j]);
            }

            double Z2 = B2;
            for (int j = 0; j < hiddenSize; ++j) {
                Z2 += W2[j] * A1[j];
            }
            double A2 = sigmoid(Z2);

            // === Loss (Mean Squared Error) ===
            double loss = std::pow(A2 - y, 2);
            totalLoss += loss;

            // === Backward Pass ===
            double dA2 = 2 * (A2 - y);
            double dZ2 = dA2 * dSigmoid(Z2);

            std::vector<double> dW2(hiddenSize);
            for (int j = 0; j < hiddenSize; ++j) {
                dW2[j] = dZ2 * A1[j];
            }
            double dB2 = dZ2;

            std::vector<double> dA1(hiddenSize);
            std::vector<double> dZ1(hiddenSize);

            for (int j = 0; j < hiddenSize; ++j) {
                dA1[j] = dZ2 * W2[j];
                dZ1[j] = dA1[j] * dRelu(Z1[j]);
            }

            std::vector<std::vector<double>> dW1(hiddenSize, std::vector<double>(inputSize));
            std::vector<double> dB1(hiddenSize);

            for (int j = 0; j < hiddenSize; ++j) {
                for (int k = 0; k < inputSize; ++k) {
                    dW1[j][k] = dZ1[j] * x[k];
                }
                dB1[j] = dZ1[j];
            }

            // === Update Weights and Biases ===
            for (int j = 0; j < hiddenSize; ++j) {
                for (int k = 0; k < inputSize; ++k) {
                    W1[j][k] -= learningRate * dW1[j][k];
                }
                B1[j] -= learningRate * dB1[j];
                W2[j] -= learningRate * dW2[j];
            }
            B2 -= learningRate * dB2;
        }

        if (epoch % 1000 == 0) {
            printf("Epoch %d, Loss: %.4f\n", epoch, totalLoss / inputs.size());
        }
    }

    // ---
    ## Predictions

    std::cout << "\nPredictions:\n";
    for (int i = 0; i < inputs.size(); ++i) {
        const std::vector<double>& x = inputs[i];

        // Forward pass for testing
        std::vector<double> Z1(hiddenSize);
        std::vector<double> A1(hiddenSize);
        for (int j = 0; j < hiddenSize; ++j) {
            Z1[j] = B1[j];
            for (int k = 0; k < inputSize; ++k) {
                Z1[j] += W1[j][k] * x[k];
            }
            A1[j] = relu(Z1[j]);
        }

        double Z2 = B2;
        for (int j = 0; j < hiddenSize; ++j) {
            Z2 += W2[j] * A1[j];
        }
        double A2 = sigmoid(Z2);

        printf("%.0f XOR %.0f = %.4f\n", x[0], x[1], A2);
    }

    return 0;
}