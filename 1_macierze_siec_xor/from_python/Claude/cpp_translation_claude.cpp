#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>

class Matrix {
public:
    std::vector<std::vector<double>> data;
    int rows, cols;
    
    Matrix(int r, int c) : rows(r), cols(c) {
        data.resize(r, std::vector<double>(c, 0.0));
    }
    
    Matrix(std::vector<std::vector<double>> values) {
        data = values;
        rows = data.size();
        cols = data[0].size();
    }
    
    // Matrix multiplication
    Matrix operator*(const Matrix& other) const {
        Matrix result(rows, other.cols);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < other.cols; j++) {
                for (int k = 0; k < cols; k++) {
                    result.data[i][j] += data[i][k] * other.data[k][j];
                }
            }
        }
        return result;
    }
    
    // Matrix addition
    Matrix operator+(const Matrix& other) const {
        Matrix result(rows, cols);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result.data[i][j] = data[i][j] + other.data[i][j];
            }
        }
        return result;
    }
    
    // Matrix subtraction
    Matrix operator-(const Matrix& other) const {
        Matrix result(rows, cols);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result.data[i][j] = data[i][j] - other.data[i][j];
            }
        }
        return result;
    }
    
    // Scalar multiplication
    Matrix operator*(double scalar) const {
        Matrix result(rows, cols);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result.data[i][j] = data[i][j] * scalar;
            }
        }
        return result;
    }
    
    // Transpose
    Matrix transpose() const {
        Matrix result(cols, rows);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result.data[j][i] = data[i][j];
            }
        }
        return result;
    }
    
    // Element-wise multiplication
    Matrix hadamard(const Matrix& other) const {
        Matrix result(rows, cols);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result.data[i][j] = data[i][j] * other.data[i][j];
            }
        }
        return result;
    }
    
    // Initialize with random values
    void randomize(std::mt19937& gen) {
        std::normal_distribution<double> dist(0.0, 1.0);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                data[i][j] = dist(gen);
            }
        }
    }
    
    // Sum along axis (0 for columns)
    Matrix sum_axis0() const {
        Matrix result(1, cols);
        for (int j = 0; j < cols; j++) {
            for (int i = 0; i < rows; i++) {
                result.data[0][j] += data[i][j];
            }
        }
        return result;
    }
    
    // Mean squared error
    double mse(const Matrix& target) const {
        double sum = 0.0;
        int count = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                double diff = data[i][j] - target.data[i][j];
                sum += diff * diff;
                count++;
            }
        }
        return sum / count;
    }
    
    // Print matrix
    void print() const {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                std::cout << std::fixed << std::setprecision(2) << data[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }
};

// Activation functions
Matrix relu(const Matrix& x) {
    Matrix result(x.rows, x.cols);
    for (int i = 0; i < x.rows; i++) {
        for (int j = 0; j < x.cols; j++) {
            result.data[i][j] = std::max(0.0, x.data[i][j]);
        }
    }
    return result;
}

Matrix relu_derivative(const Matrix& x) {
    Matrix result(x.rows, x.cols);
    for (int i = 0; i < x.rows; i++) {
        for (int j = 0; j < x.cols; j++) {
            result.data[i][j] = (x.data[i][j] > 0) ? 1.0 : 0.0;
        }
    }
    return result;
}

Matrix sigmoid(const Matrix& x) {
    Matrix result(x.rows, x.cols);
    for (int i = 0; i < x.rows; i++) {
        for (int j = 0; j < x.cols; j++) {
            result.data[i][j] = 1.0 / (1.0 + std::exp(-x.data[i][j]));
        }
    }
    return result;
}

Matrix sigmoid_derivative(const Matrix& x) {
    Matrix s = sigmoid(x);
    Matrix ones(s.rows, s.cols);
    for (int i = 0; i < s.rows; i++) {
        for (int j = 0; j < s.cols; j++) {
            ones.data[i][j] = 1.0;
        }
    }
    return s.hadamard(ones - s);
}

int main() {
    // Training data (XOR problem)
    Matrix X({{0, 0},
              {0, 1},
              {1, 0},
              {1, 1}});
    
    Matrix y({{0},
              {1},
              {1},
              {0}});
    
    // Network parameters
    int input_size = 2;
    int hidden_size = 4;
    int output_size = 1;
    double learning_rate = 0.1;
    int epochs = 10000;
    
    // Initialize random number generator with seed 42
    std::mt19937 gen(42);
    
    // Initialize weights and biases
    Matrix W1(input_size, hidden_size);
    Matrix b1(1, hidden_size);
    Matrix W2(hidden_size, output_size);
    Matrix b2(1, output_size);
    
    W1.randomize(gen);
    W2.randomize(gen);
    
    // Training loop
    for (int epoch = 0; epoch < epochs; epoch++) {
        // Forward pass
        Matrix z1 = X * W1 + b1;
        Matrix a1 = relu(z1);
        
        Matrix z2 = a1 * W2 + b2;
        Matrix a2 = sigmoid(z2);
        
        // Calculate loss
        double loss = a2.mse(y);
        
        // Backward pass
        Matrix dz2 = (a2 - y).hadamard(sigmoid_derivative(z2));
        Matrix dW2 = a1.transpose() * dz2;
        Matrix db2 = dz2.sum_axis0();
        
        Matrix dz1 = (dz2 * W2.transpose()).hadamard(relu_derivative(z1));
        Matrix dW1 = X.transpose() * dz1;
        Matrix db1 = dz1.sum_axis0();
        
        // Update weights and biases
        W2 = W2 - dW2 * learning_rate;
        b2 = b2 - db2 * learning_rate;
        W1 = W1 - dW1 * learning_rate;
        b1 = b1 - db1 * learning_rate;
        
        // Print progress
        if (epoch % 1000 == 0) {
            std::cout << "Epoch " << epoch << ", loss: " 
                      << std::fixed << std::setprecision(4) << loss << std::endl;
        }
    }
    
    // Final predictions
    std::cout << "\nPredictions:" << std::endl;
    Matrix z1_final = X * W1 + b1;
    Matrix a1_final = relu(z1_final);
    Matrix z2_final = a1_final * W2 + b2;
    Matrix predictions = sigmoid(z2_final);
    predictions.print();
    
    return 0;
}