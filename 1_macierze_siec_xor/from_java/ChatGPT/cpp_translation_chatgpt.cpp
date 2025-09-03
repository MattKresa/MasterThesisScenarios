#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>

using namespace std;

// --- Activation functions ---
double relu(double x) {
    return max(0.0, x);
}

double dRelu(double x) {
    return x > 0 ? 1.0 : 0.0;
}

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double dSigmoid(double x) {
    double s = sigmoid(x);
    return s * (1 - s);
}

double randWeight() {
    return ((double)rand() / RAND_MAX) * 2 - 1;
}

int main() {
    srand((unsigned)time(nullptr));

    // XOR data
    double inputs[4][2] = {
        {0, 0},
        {0, 1},
        {1, 0},
        {1, 1}
    };
    double targets[4] = { 0, 1, 1, 0 };

    // Network architecture
    int inputSize = 2;
    int hiddenSize = 4;
    int outputSize = 1;
    double learningRate = 0.1;

    // Weights and biases
    double W1[4][2];
    double B1[4];
    double W2[4];
    double B2 = randWeight();

    // Initialize
    for (int i = 0; i < hiddenSize; i++) {
        B1[i] = randWeight();
        W2[i] = randWeight();
        for (int j = 0; j < inputSize; j++) {
            W1[i][j] = randWeight();
        }
    }

    // Training loop
    for (int epoch = 0; epoch < 10000; epoch++) {
        double totalLoss = 0.0;

        for (int i = 0; i < 4; i++) {
            double* x = inputs[i];
            double y = targets[i];

            // === Forward ===
            double Z1[4];
            double A1[4];

            for (int j = 0; j < hiddenSize; j++) {
                Z1[j] = B1[j];
                for (int k = 0; k < inputSize; k++) {
                    Z1[j] += W1[j][k] * x[k];
                }
                A1[j] = relu(Z1[j]);
            }

            double Z2 = B2;
            for (int j = 0; j < hiddenSize; j++) {
                Z2 += W2[j] * A1[j];
            }
            double A2 = sigmoid(Z2);

            // === Loss (MSE) ===
            double loss = pow(A2 - y, 2);
            totalLoss += loss;

            // === Backward ===
            double dA2 = 2 * (A2 - y);
            double dZ2 = dA2 * dSigmoid(Z2);

            double dW2[4];
            for (int j = 0; j < hiddenSize; j++) {
                dW2[j] = dZ2 * A1[j];
            }
            double dB2 = dZ2;

            double dA1[4];
            double dZ1[4];

            for (int j = 0; j < hiddenSize; j++) {
                dA1[j] = dZ2 * W2[j];
                dZ1[j] = dA1[j] * dRelu(Z1[j]);
            }

            double dW1[4][2];
            double dB1[4];

            for (int j = 0; j < hiddenSize; j++) {
                for (int k = 0; k < inputSize; k++) {
                    dW1[j][k] = dZ1[j] * x[k];
                }
                dB1[j] = dZ1[j];
            }

            // === Update ===
            for (int j = 0; j < hiddenSize; j++) {
                for (int k = 0; k < inputSize; k++) {
                    W1[j][k] -= learningRate * dW1[j][k];
                }
                B1[j] -= learningRate * dB1[j];
                W2[j] -= learningRate * dW2[j];
            }
            B2 -= learningRate * dB2;
        }

        if (epoch % 1000 == 0) {
            cout << "Epoch " << epoch
                << ", Loss: " << fixed << setprecision(4)
                << totalLoss / 4 << endl;
        }
    }

    // === Test ===
    cout << "\nPredictions:\n";
    for (int i = 0; i < 4; i++) {
        double* x = inputs[i];

        double Z1[4];
        double A1[4];
        for (int j = 0; j < hiddenSize; j++) {
            Z1[j] = B1[j];
            for (int k = 0; k < inputSize; k++) {
                Z1[j] += W1[j][k] * x[k];
            }
            A1[j] = relu(Z1[j]);
        }

        double Z2 = B2;
        for (int j = 0; j < hiddenSize; j++) {
            Z2 += W2[j] * A1[j];
        }
        double A2 = sigmoid(Z2);

        cout << fixed << setprecision(4)
            << (int)x[0] << " XOR " << (int)x[1] << " = " << A2 << endl;
    }

    return 0;
}
