#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>
#include <functional>

// ===== Objective function =====
double f(const std::vector<double>& vec) {
    double sum_sq = 0.0;
    double sum_sin = 0.0;
    for (double x : vec) {
        sum_sq += x * x;
        sum_sin += std::sin(3.0 * x);
    }
    return sum_sq + 0.5 * sum_sin;
}

// ===== Numerical gradient (central difference) =====
std::vector<double> numerical_gradient(
    const std::function<double(const std::vector<double>&)>& func,
    const std::vector<double>& vec,
    double h = 1e-5
) {
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

// ===== Gradient descent with adaptive learning rate =====
std::pair<std::vector<double>, std::vector<std::vector<double>>> gradient_descent(
    const std::function<double(const std::vector<double>&)>& func,
    std::vector<double> start_vec,
    double init_lr = 0.1,
    int max_iter = 1000,
    double tolerance = 1e-6,
    int log_interval = 10
) {
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
            std::cout << "Iter " << iteration
                << ": f(x) = " << std::fixed << std::setprecision(6) << func(vec)
                << ", lr = " << std::setprecision(5) << lr
                << ", x = [";
            for (size_t i = 0; i < vec.size(); ++i) {
                std::cout << std::setprecision(5) << vec[i];
                if (i + 1 < vec.size()) std::cout << ", ";
            }
            std::cout << "]\n";
        }

        // Stop criteria
        double move_norm = 0.0, grad_norm = 0.0;
        for (size_t i = 0; i < vec.size(); ++i) {
            move_norm += (new_vec[i] - vec[i]) * (new_vec[i] - vec[i]);
            grad_norm += grad[i] * grad[i];
        }
        move_norm = std::sqrt(move_norm);
        grad_norm = std::sqrt(grad_norm);

        if (move_norm < tolerance || grad_norm < tolerance) {
            std::cout << "Converged at iteration " << iteration << "\n";
            break;
        }

        vec = new_vec;
    }

    return { vec, history };
}

int main() {
    // Set random number generator (equivalent to np.random.seed(0))
    std::mt19937 rng(0);
    std::uniform_real_distribution<double> dist(-3.0, 3.0);

    // Start point in 5D
    std::vector<double> start_point(5);
    for (double& x : start_point) {
        x = dist(rng);
    }

    auto [minimum_vec, path] = gradient_descent(f, start_point, 0.05);

    std::cout << "\nFinal result:\n";
    std::cout << "Minimum found at: [";
    for (size_t i = 0; i < minimum_vec.size(); ++i) {
        std::cout << std::setprecision(6) << minimum_vec[i];
        if (i + 1 < minimum_vec.size()) std::cout << ", ";
    }
    std::cout << "]\n";
    std::cout << "Function value: " << std::setprecision(6) << f(minimum_vec) << "\n";

    return 0;
}
