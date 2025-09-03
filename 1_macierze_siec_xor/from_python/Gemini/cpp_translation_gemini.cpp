#include <iostream>
#include <vector>
#include <cmath>
#include <Eigen/Dense> // For matrix operations

// Activation functions
Eigen::MatrixXd relu(const Eigen::MatrixXd& x) {
    return x.cwiseMax(0.0);
}

Eigen::MatrixXd relu_derivative(const Eigen::MatrixXd& x) {
    return (x.array() > 0.0).cast<double>();
}

Eigen::MatrixXd sigmoid(const Eigen::MatrixXd& x) {
    return 1.0 / (1.0 + (-x.array()).exp()).matrix();
}

Eigen::MatrixXd sigmoid_derivative(const Eigen::MatrixXd& x) {
    Eigen::MatrixXd s = sigmoid(x);
    return s.array() * (1.0 - s.array());
}

int main() {
    // Data
    Eigen::MatrixXd X(4, 2);
    X << 0, 0,
         0, 1,
         1, 0,
         1, 1;

    Eigen::MatrixXd y(4, 1);
    y << 0,
         1,
         1,
         0;

    // Hyperparameters
    int input_size = 2;
    int hidden_size = 4;
    int output_size = 1;
    double learning_rate = 0.1;
    int epochs = 10000;

    // Random seed for reproducibility
    srand(42); // For C-style random, consider <random> for better C++ randomness

    // Initialize weights and biases
    // Eigen::Randn for normal distribution (requires C++11 or later)
    // For older C++, you might need a custom normal distribution generator
    Eigen::MatrixXd W1 = Eigen::MatrixXd::Random(input_size, hidden_size);
    Eigen::MatrixXd b1 = Eigen::MatrixXd::Zero(1, hidden_size);
    Eigen::MatrixXd W2 = Eigen::MatrixXd::Random(hidden_size, output_size);
    Eigen::MatrixXd b2 = Eigen::MatrixXd::Zero(1, output_size);

    for (int epoch = 0; epoch < epochs; ++epoch) {
        // FORWARD
        Eigen::MatrixXd z1 = X * W1 + b1.replicate(X.rows(), 1);
        Eigen::MatrixXd a1 = relu(z1);

        Eigen::MatrixXd z2 = a1 * W2 + b2.replicate(a1.rows(), 1);
        Eigen::MatrixXd a2 = sigmoid(z2);

        // Loss
        double loss = ((a2 - y).array().pow(2)).mean();

        // BACKWARD
        Eigen::MatrixXd dz2 = (a2 - y).array() * sigmoid_derivative(z2).array();
        Eigen::MatrixXd dW2 = a1.transpose() * dz2;
        Eigen::MatrixXd db2 = dz2.colwise().sum(); // Sum along columns

        Eigen::MatrixXd dz1 = (dz2 * W2.transpose()).array() * relu_derivative(z1).array();
        Eigen::MatrixXd dW1 = X.transpose() * dz1;
        Eigen::MatrixXd db1 = dz1.colwise().sum(); // Sum along columns

        // Update weights and biases
        W2 -= learning_rate * dW2;
        b2 -= learning_rate * db2;
        W1 -= learning_rate * dW1;
        b1 -= learning_rate * db1;

        if (epoch % 1000 == 0) {
            std::cout << "Epoch " << epoch << ", loss: " << std::fixed << std::setprecision(4) << loss << std::endl;
        }
    }

    std::cout << "\nPredictions:" << std::endl;
    Eigen::MatrixXd preds_z1 = X * W1 + b1.replicate(X.rows(), 1);
    Eigen::MatrixXd preds_a1 = relu(preds_z1);
    Eigen::MatrixXd preds_z2 = preds_a1 * W2 + b2.replicate(preds_a1.rows(), 1);
    Eigen::MatrixXd preds = sigmoid(preds_z2);

    // Round predictions to 2 decimal places
    Eigen::MatrixXd rounded_preds = (preds.array() * 100).round() / 100;
    std::cout << rounded_preds << std::endl;

    return 0;
}