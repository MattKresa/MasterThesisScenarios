#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>

using namespace std;

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double d_sigmoid(double x) {
    double s = sigmoid(x);
    return s * (1 - s);
}

double relu(double x) {
    return x > 0 ? x : 0;
}

double d_relu(double x) {
    return x > 0 ? 1 : 0;
}

double randWeight() {
    return ((double)rand() / RAND_MAX) * 2 - 1;
}

struct Sample {
    vector<double> input;
    double output;
};

int main() {
    srand((unsigned)time(0));

    vector<Sample> data = {
        {{0, 0}, 0},
        {{0, 1}, 1},
        {{1, 0}, 1},
        {{1, 1}, 0}
    };

    // Network architecture
    const int inputSize = 2;
    const int hiddenSize = 4;
    const int outputSize = 1;
    double learningRate = 0.1;

    // Weights and biases
    vector<vector<double>> W1(hiddenSize, vector<double>(inputSize));
    vector<double> B1(hiddenSize);

    vector<double> W2(hiddenSize);  // from hidden to output
    double B2 = randWeight();

    // Initialize weights
    for (int i = 0; i < hiddenSize; ++i) {
        B1[i] = randWeight();
        for (int j = 0; j < inputSize; ++j)
            W1[i][j] = randWeight();
        W2[i] = randWeight();
    }

    // Training loop
    for (int epoch = 0; epoch < 10000; ++epoch) {
        double totalLoss = 0;

        for (auto& sample : data) {
            // === Forward pass ===
            vector<double> Z1(hiddenSize), A1(hiddenSize);

            for (int i = 0; i < hiddenSize; ++i) {
                Z1[i] = B1[i];
                for (int j = 0; j < inputSize; ++j)
                    Z1[i] += W1[i][j] * sample.input[j];
                A1[i] = relu(Z1[i]);
            }

            double Z2 = B2;
            for (int i = 0; i < hiddenSize; ++i)
                Z2 += W2[i] * A1[i];
            double A2 = sigmoid(Z2);

            // === Loss MSE ===
            double y = sample.output;
            double loss = pow(A2 - y, 2);
            totalLoss += loss;

            // === Backward pass ===
            double dA2 = A2 - y;
            double dZ2 = dA2 * d_sigmoid(Z2);

            vector<double> dW2(hiddenSize);
            for (int i = 0; i < hiddenSize; ++i)
                dW2[i] = dZ2 * A1[i];
            double dB2 = dZ2;

            vector<double> dA1(hiddenSize), dZ1(hiddenSize);
            for (int i = 0; i < hiddenSize; ++i) {
                dA1[i] = dZ2 * W2[i];
                dZ1[i] = dA1[i] * d_relu(Z1[i]);
            }

            vector<vector<double>> dW1(hiddenSize, vector<double>(inputSize));
            vector<double> dB1(hiddenSize);

            for (int i = 0; i < hiddenSize; ++i) {
                for (int j = 0; j < inputSize; ++j)
                    dW1[i][j] = dZ1[i] * sample.input[j];
                dB1[i] = dZ1[i];
            }

            // === Update weights ===
            for (int i = 0; i < hiddenSize; ++i) {
                for (int j = 0; j < inputSize; ++j)
                    W1[i][j] -= learningRate * dW1[i][j];
                B1[i] -= learningRate * dB1[i];
                W2[i] -= learningRate * dW2[i];
            }
            B2 -= learningRate * dB2;
        }

        if (epoch % 1000 == 0)
            cout << "Epoch " << epoch << ", Loss: " << std::fixed << setprecision(4) << totalLoss << endl;
    }

    // === Test ===
    cout << "\nPredictions:\n";
    for (auto& sample : data) {
        vector<double> Z1(hiddenSize), A1(hiddenSize);
        for (int i = 0; i < hiddenSize; ++i) {
            Z1[i] = B1[i];
            for (int j = 0; j < inputSize; ++j)
                Z1[i] += W1[i][j] * sample.input[j];
            A1[i] = relu(Z1[i]);
        }

        double Z2 = B2;
        for (int i = 0; i < hiddenSize; ++i)
            Z2 += W2[i] * A1[i];
        double A2 = sigmoid(Z2);

        cout << std::fixed << setprecision(0) << sample.input[0] << " XOR " << sample.input[1]
            << " = " << std::fixed << setprecision(2) << A2 << endl;
    }

    return 0;
}