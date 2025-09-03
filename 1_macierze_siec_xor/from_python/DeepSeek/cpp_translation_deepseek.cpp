#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <iomanip>
#include <numeric>

using namespace std;

// Activation functions
double relu(double x) {
    return max(0.0, x);
}

vector<vector<double>> relu(const vector<vector<double>>& x) {
    vector<vector<double>> result(x.size(), vector<double>(x[0].size()));
    for (size_t i = 0; i < x.size(); ++i) {
        for (size_t j = 0; j < x[0].size(); ++j) {
            result[i][j] = relu(x[i][j]);
        }
    }
    return result;
}

vector<vector<double>> relu_derivative(const vector<vector<double>>& x) {
    vector<vector<double>> result(x.size(), vector<double>(x[0].size()));
    for (size_t i = 0; i < x.size(); ++i) {
        for (size_t j = 0; j < x[0].size(); ++j) {
            result[i][j] = x[i][j] > 0 ? 1.0 : 0.0;
        }
    }
    return result;
}

double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

vector<vector<double>> sigmoid(const vector<vector<double>>& x) {
    vector<vector<double>> result(x.size(), vector<double>(x[0].size()));
    for (size_t i = 0; i < x.size(); ++i) {
        for (size_t j = 0; j < x[0].size(); ++j) {
            result[i][j] = sigmoid(x[i][j]);
        }
    }
    return result;
}

vector<vector<double>> sigmoid_derivative(const vector<vector<double>>& x) {
    vector<vector<double>> s = sigmoid(x);
    vector<vector<double>> result(s.size(), vector<double>(s[0].size()));
    for (size_t i = 0; i < s.size(); ++i) {
        for (size_t j = 0; j < s[0].size(); ++j) {
            result[i][j] = s[i][j] * (1.0 - s[i][j]);
        }
    }
    return result;
}

// Matrix operations
vector<vector<double>> dot(const vector<vector<double>>& a, const vector<vector<double>>& b) {
    vector<vector<double>> result(a.size(), vector<double>(b[0].size(), 0.0));
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < b[0].size(); ++j) {
            for (size_t k = 0; k < a[0].size(); ++k) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return result;
}

vector<vector<double>> operator+(const vector<vector<double>>& a, const vector<vector<double>>& b) {
    vector<vector<double>> result(a.size(), vector<double>(a[0].size()));
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < a[0].size(); ++j) {
            result[i][j] = a[i][j] + b[i][j];
        }
    }
    return result;
}

vector<vector<double>> operator-(const vector<vector<double>>& a, const vector<vector<double>>& b) {
    vector<vector<double>> result(a.size(), vector<double>(a[0].size()));
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < a[0].size(); ++j) {
            result[i][j] = a[i][j] - b[i][j];
        }
    }
    return result;
}

vector<vector<double>> operator*(const vector<vector<double>>& a, const vector<vector<double>>& b) {
    vector<vector<double>> result(a.size(), vector<double>(a[0].size()));
    for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < a[0].size(); ++j) {
            result[i][j] = a[i][j] * b[i][j];
        }
    }
    return result;
}

vector<vector<double>> transpose(const vector<vector<double>>& matrix) {
    vector<vector<double>> result(matrix[0].size(), vector<double>(matrix.size()));
    for (size_t i = 0; i < matrix.size(); ++i) {
        for (size_t j = 0; j < matrix[0].size(); ++j) {
            result[j][i] = matrix[i][j];
        }
    }
    return result;
}

vector<vector<double>> sum_rows(const vector<vector<double>>& matrix) {
    vector<vector<double>> result(1, vector<double>(matrix[0].size(), 0.0));
    for (const auto& row : matrix) {
        for (size_t j = 0; j < row.size(); ++j) {
            result[0][j] += row[j];
        }
    }
    return result;
}

double mean_squared_error(const vector<vector<double>>& y_pred, const vector<vector<double>>& y_true) {
    double sum = 0.0;
    for (size_t i = 0; i < y_pred.size(); ++i) {
        for (size_t j = 0; j < y_pred[0].size(); ++j) {
            double diff = y_pred[i][j] - y_true[i][j];
            sum += diff * diff;
        }
    }
    return sum / (y_pred.size() * y_pred[0].size());
}

vector<vector<double>> randn(size_t rows, size_t cols, default_random_engine& generator) {
    normal_distribution<double> distribution(0.0, 1.0);
    vector<vector<double>> result(rows, vector<double>(cols));
    for (auto& row : result) {
        for (auto& val : row) {
            val = distribution(generator);
        }
    }
    return result;
}

vector<vector<double>> zeros(size_t rows, size_t cols) {
    return vector<vector<double>>(rows, vector<double>(cols, 0.0));
}

int main() {
    // XOR dataset
    vector<vector<double>> X = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    vector<vector<double>> y = {{0}, {1}, {1}, {0}};

    // Network parameters
    int input_size = 2;
    int hidden_size = 4;
    int output_size = 1;
    double learning_rate = 0.1;
    int epochs = 10000;

    // Initialize weights with fixed seed for reproducibility
    default_random_engine generator(42);
    
    // Initialize weights and biases
    vector<vector<double>> W1 = randn(input_size, hidden_size, generator);
    vector<vector<double>> b1 = zeros(1, hidden_size);
    vector<vector<double>> W2 = randn(hidden_size, output_size, generator);
    vector<vector<double>> b2 = zeros(1, output_size);

    // Training loop
    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Forward pass
        vector<vector<double>> z1 = dot(X, W1) + b1;
        vector<vector<double>> a1 = relu(z1);

        vector<vector<double>> z2 = dot(a1, W2) + b2;
        vector<vector<double>> a2 = sigmoid(z2);

        // Calculate loss
        double loss = mean_squared_error(a2, y);

        // Backward pass
        vector<vector<double>> dz2 = (a2 - y) * sigmoid_derivative(z2);
        vector<vector<double>> dW2 = dot(transpose(a1), dz2);
        vector<vector<double>> db2 = sum_rows(dz2);

        vector<vector<double>> dz1 = dot(dz2, transpose(W2)) * relu_derivative(z1);
        vector<vector<double>> dW1 = dot(transpose(X), dz1);
        vector<vector<double>> db1 = sum_rows(dz1);

        // Update weights
        W2 = W2 - dot(dW2, {{learning_rate}});
        b2 = b2 - dot(db2, {{learning_rate}});
        W1 = W1 - dot(dW1, {{learning_rate}});
        b1 = b1 - dot(db1, {{learning_rate}});

        if (epoch % 1000 == 0) {
            cout << "Epoch " << epoch << ", loss: " << fixed << setprecision(4) << loss << endl;
        }
    }

    // Predictions
    cout << "\nPredictions:" << endl;
    vector<vector<double>> hidden_output = relu(dot(X, W1) + b1);
    vector<vector<double>> predictions = sigmoid(dot(hidden_output, W2) + b2);
    
    for (const auto& row : predictions) {
        cout << "[";
        for (size_t i = 0; i < row.size(); ++i) {
            if (i > 0) cout << " ";
            cout << fixed << setprecision(2) << row[i];
        }
        cout << "]" << endl;
    }

    return 0;
}