#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>

// Example function (works for any number of dimensions)
double f(const std::vector<double>& vec) {
    double sum_sq = 0.0;
    double sum_sin = 0.0;
    for (double v : vec) {
        sum_sq += v * v;
        sum_sin += sin(3 * v);
    }
    return sum_sq + 0.5 * sum_sin;
}

// Numerical gradient using central difference
std::vector<double> numerical_gradient(double (*func)(const std::vector<double>&),
    const std::vector<double>& vec,
    double h = 1e-5) {
    std::vector<double> grad(vec.size(), 0.0);
    for (size_t i = 0; i < vec.size(); ++i) {
        std::vector<double> vec_forward = vec;
        std::vector<double> vec_backward = vec;
        vec_forward[i] += h;
        vec_backward[i] -= h;
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2 * h);
    }
    return grad;
}

// Helper function to compute vector norm
double vector_norm(const std::vector<double>& vec) {
    double sum = 0.0;
    for (double v : vec) {
        sum += v * v;
    }
    return sqrt(sum);
}

// Gradient Descent with adaptive learning rate
std::pair<std::vector<double>, std::vector<std::vector<double>>>
gradient_descent(double (*func)(const std::vector<double>&),
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
        std::vector<double> new_vec(vec.size());

        for (size_t i = 0; i < vec.size(); ++i) {
            new_vec[i] = vec[i] - lr * grad[i];
        }

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
            std::cout << "Iter " << iteration << ": f(x) = " << std::fixed << std::setprecision(6)
                << func(vec) << ", x = [";
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i != 0) std::cout << ", ";
                std::cout << vec[i];
            }
            std::cout << "], lr = " << std::setprecision(5) << lr << std::endl;
        }

        // Stop if movement or gradient is small
        double vec_diff_norm = 0.0;
        double grad_norm = 0.0;
        for (size_t i = 0; i < vec.size(); ++i) {
            vec_diff_norm += (new_vec[i] - vec[i]) * (new_vec[i] - vec[i]);
            grad_norm += grad[i] * grad[i];
        }
        vec_diff_norm = sqrt(vec_diff_norm);
        grad_norm = sqrt(grad_norm);

        if (vec_diff_norm < tolerance || grad_norm < tolerance) {
            std::cout << "Converged at iteration " << iteration << std::endl;
            break;
        }

        vec = new_vec;
    }
    return { vec, history };
}

int main() {
    // Start from a random 5D vector
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(-3.0, 3.0);

    std::vector<double> start_point(5);
    for (double& v : start_point) {
        v = dist(gen);
    }

    auto [minimum_vec, path] = gradient_descent(f, start_point, 0.05);

    std::cout << "\nFinal result:" << std::endl;
    std::cout << "Minimum found at: [";
    for (size_t i = 0; i < minimum_vec.size(); ++i) {
        if (i != 0) std::cout << ", ";
        std::cout << minimum_vec[i];
    }
    std::cout << "]" << std::endl;
    std::cout << "Function value: " << std::fixed << std::setprecision(6)
        << f(minimum_vec) << std::endl;

    return 0;
}