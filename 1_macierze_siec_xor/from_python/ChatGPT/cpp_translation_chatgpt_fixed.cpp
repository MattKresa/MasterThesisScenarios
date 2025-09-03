#include <iostream>
#include <Eigen/Dense>
#include <cmath>
#include <cstdlib>

using namespace Eigen;
using namespace std;

// Activation functions
MatrixXd relu(const MatrixXd& x) {
    return x.cwiseMax(0.0);
}

MatrixXd relu_derivative(const MatrixXd& x) {
    return (x.array() > 0).cast<double>();
}

MatrixXd sigmoid(const MatrixXd& x) {
    return 1.0 / (1.0 + (-x.array()).exp());
}

MatrixXd sigmoid_derivative(const MatrixXd& x) {
    MatrixXd s = sigmoid(x);
    return s.array() * (1 - s.array());
}

int main() {
    // Input (XOR problem)
    MatrixXd X(4, 2);
    X << 0, 0,
        0, 1,
        1, 0,
        1, 1;

    MatrixXd y(4, 1);
    y << 0,
        1,
        1,
        0;

    int input_size = 2;
    int hidden_size = 4;
    int output_size = 1;
    double learning_rate = 0.1;
    int epochs = 10000;

    srand(42); // Seed

    // Random initialization
    MatrixXd W1 = MatrixXd::Random(input_size, hidden_size);
    MatrixXd b1 = MatrixXd::Zero(1, hidden_size);
    MatrixXd W2 = MatrixXd::Random(hidden_size, output_size);
    MatrixXd b2 = MatrixXd::Zero(1, output_size);

    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Forward pass
        MatrixXd z1 = X * W1 + b1.replicate(X.rows(), 1);
        MatrixXd a1 = relu(z1);

        MatrixXd z2 = a1 * W2 + b2.replicate(a1.rows(), 1);
        MatrixXd a2 = sigmoid(z2);

        // Loss (MSE)
        double loss = ((a2 - y).array().square()).mean();

        // Backpropagation
        MatrixXd dz2 = (a2 - y).array() * sigmoid_derivative(z2).array();
        MatrixXd dW2 = a1.transpose() * dz2;
        MatrixXd db2 = dz2.colwise().sum();

        MatrixXd dz1 = (dz2 * W2.transpose()).array() * relu_derivative(z1).array();
        MatrixXd dW1 = X.transpose() * dz1;
        MatrixXd db1 = dz1.colwise().sum();

        // Update weights
        W2 -= learning_rate * dW2;
        b2 -= learning_rate * db2;
        W1 -= learning_rate * dW1;
        b1 -= learning_rate * db1;

        if (epoch % 1000 == 0) {
            cout << "Epoch " << epoch << ", loss: " << loss << endl;
        }
    }

    // Predictions
    cout << "\nPredictions:" << endl;
    MatrixXd preds = sigmoid(relu(X * W1 + b1.replicate(X.rows(), 1)) * W2 + b2.replicate(4, 1));
    cout << preds.unaryExpr([](double v) { return round(v * 100) / 100; }) << endl;

    return 0;
}
