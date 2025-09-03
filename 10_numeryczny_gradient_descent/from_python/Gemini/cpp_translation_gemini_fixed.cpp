#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <random>

// Use a typedef for the vector type to make the code cleaner.
using Vector = std::vector<double>;

// The function to be minimized.
double f(const Vector& vec) {
    double sum_sq = 0.0;
    double sum_sin = 0.0;
    for (double val : vec) {
        sum_sq += val * val;
        sum_sin += std::sin(3 * val);
    }
    return sum_sq + 0.5 * sum_sin;
}

// Calculates the numerical gradient using the central difference method.
Vector numerical_gradient(double (*func)(const Vector&), const Vector& vec, double h = 1e-5) {
    Vector grad(vec.size(), 0.0);
    for (size_t i = 0; i < vec.size(); ++i) {
        Vector vec_forward = vec;
        Vector vec_backward = vec;
        vec_forward[i] += h;
        vec_backward[i] -= h;
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2 * h);
    }
    return grad;
}

// Calculates the L2 norm (magnitude) of a vector.
double l2_norm(const Vector& vec) {
    double sum_sq = 0.0;
    for (double val : vec) {
        sum_sq += val * val;
    }
    return std::sqrt(sum_sq);
}

// Prints a vector to the console.
void print_vector(const Vector& vec) {
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i];
        if (i < vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]";
}

// Gradient Descent optimization algorithm with an adaptive learning rate.
Vector gradient_descent(double (*func)(const Vector&), Vector start_vec, double init_lr = 0.1, int max_iter = 1000, double tolerance = 1e-6, int log_interval = 10) {
    Vector vec = start_vec;
    double lr = init_lr;
    std::vector<Vector> history;
    history.push_back(vec);

    std::cout << std::fixed << std::setprecision(6);

    for (int iteration = 0; iteration < max_iter; ++iteration) {
        Vector grad = numerical_gradient(func, vec);
        Vector new_vec(vec.size());
        for (size_t i = 0; i < vec.size(); ++i) {
            new_vec[i] = vec[i] - lr * grad[i];
        }

        // Adaptive learning rate logic.
        if (func(new_vec) > func(vec)) {
            lr *= 0.5;
            continue;
        }

        lr *= 1.05;

        history.push_back(new_vec);

        // Log progress.
        if (iteration % log_interval == 0) {
            std::cout << "Iter " << iteration << ": f(x) = " << func(vec) << ", x = ";
            print_vector(vec);
            std::cout << ", lr = " << std::setprecision(5) << lr << std::setprecision(6) << std::endl;
        }

        // Convergence check.
        Vector diff_vec(vec.size());
        for (size_t i = 0; i < vec.size(); ++i) {
            diff_vec[i] = new_vec[i] - vec[i];
        }

        if (l2_norm(diff_vec) < tolerance || l2_norm(grad) < tolerance) {
            std::cout << "Converged at iteration " << iteration << std::endl;
            vec = new_vec;
            break;
        }

        vec = new_vec;
    }
    return vec;
}

// Main function to run the example.
int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(-3.0, 3.0);

    Vector start_point(5);
    for (double& v : start_point) {
        v = dist(gen);
    }

    Vector minimum_vec = gradient_descent(f, start_point, 0.05);

    std::cout << "\nFinal result:\n";
    std::cout << "Minimum found at: ";
    print_vector(minimum_vec);
    std::cout << "\nFunction value: " << f(minimum_vec) << std::endl;

    return 0;
}