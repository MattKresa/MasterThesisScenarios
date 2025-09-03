#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>
#include <functional>

// Example function (works for any number of dimensions)
double f(const std::vector<double>& vec) {
    // Sphere function with a sine perturbation
    double sum_squares = 0.0;
    double sum_sin = 0.0;

    for (double x : vec) {
        sum_squares += x * x;
        sum_sin += std::sin(3.0 * x);
    }

    return sum_squares + 0.5 * sum_sin;
}

// Numerical gradient using central difference
std::vector<double> numerical_gradient(std::function<double(const std::vector<double>&)> func,
    const std::vector<double>& vec,
    double h = 1e-5) {
    std::vector<double> grad(vec.size(), 0.0);

    for (size_t i = 0; i < vec.size(); ++i) {
        std::vector<double> vec_forward = vec;
        std::vector<double> vec_backward = vec;

        vec_forward[i] += h;
        vec_backward[i] -= h;

        grad[i] = (func(vec_forward) - func(vec_backward)) / (2.0 * h);
    }

    return grad;
}

// Calculate L2 norm of a vector
double norm(const std::vector<double>& vec) {
    double sum = 0.0;
    for (double x : vec) {
        sum += x * x;
    }
    return std::sqrt(sum);
}

// Vector subtraction
std::vector<double> subtract(const std::vector<double>& a, const std::vector<double>& b) {
    std::vector<double> result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        result[i] = a[i] - b[i];
    }
    return result;
}

// Vector scalar multiplication
std::vector<double> multiply(const std::vector<double>& vec, double scalar) {
    std::vector<double> result(vec.size());
    for (size_t i = 0; i < vec.size(); ++i) {
        result[i] = vec[i] * scalar;
    }
    return result;
}

// Print vector
void print_vector(const std::vector<double>& vec) {
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << std::fixed << std::setprecision(6) << vec[i];
        if (i < vec.size() - 1) std::cout << ", ";
    }
    std::cout << "]";
}

// Gradient Descent with adaptive learning rate
std::pair<std::vector<double>, std::vector<std::vector<double>>>
gradient_descent(std::function<double(const std::vector<double>&)> func,
    const std::vector<double>& start_vec,
    double init_lr = 0.1,
    int max_iter = 1000,
    double tolerance = 1e-6,
    int log_interval = 10) {

    std::vector<double> vec = start_vec;
    double lr = init_lr;
    std::vector<std::vector<double>> history;
    history.push_back(vec);

    for (int iteration = 0; iteration < max_iter; ++iteration) {
        std::vector<double> grad = numerical_gradient(func, vec);
        std::vector<double> new_vec = subtract(vec, multiply(grad, lr));

        // If the step increases the function value -> decrease learning rate
        if (func(new_vec) > func(vec)) {
            lr *= 0.5;
            continue;
        }

        // If the step improves the function -> slightly increase learning rate
        lr *= 1.05;

        history.push_back(new_vec);

        // Log progress every `log_interval` iterations
        if (iteration % log_interval == 0) {
            std::cout << "Iter " << iteration << ": f(x) = "
                << std::fixed << std::setprecision(6) << func(vec)
                << ", x = ";
            print_vector(vec);
            std::cout << ", lr = " << std::fixed << std::setprecision(5) << lr << std::endl;
        }

        // Stop if movement or gradient is small
        std::vector<double> movement = subtract(new_vec, vec);
        if (norm(movement) < tolerance || norm(grad) < tolerance) {
            std::cout << "Converged at iteration " << iteration << std::endl;
            break;
        }

        vec = new_vec;
    }

    return std::make_pair(vec, history);
}

// ===== Example usage =====
int main() {
    // Start from a random 5D vector
    std::random_device rd;
    std::mt19937 gen(0); // Using seed 0 for reproducibility (like np.random.seed(0))
    std::uniform_real_distribution<double> dist(-3.0, 3.0);

    std::vector<double> start_point(5);
    for (size_t i = 0; i < start_point.size(); ++i) {
        start_point[i] = dist(gen);
    }

    auto result = gradient_descent(f, start_point, 0.05);
    std::vector<double> minimum_vec = result.first;
    std::vector<std::vector<double>> path = result.second;

    std::cout << "\nFinal result:" << std::endl;
    std::cout << "Minimum found at: ";
    print_vector(minimum_vec);
    std::cout << std::endl;
    std::cout << "Function value: " << std::fixed << std::setprecision(6)
        << f(minimum_vec) << std::endl;

    return 0;
}