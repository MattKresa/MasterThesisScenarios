#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>

// Function f(vec): Sphere function with a sine perturbation
double f(const std::vector<double>& vec) {
    double sum_sq = 0.0;
    double sum_sin = 0.0;
    for (double x : vec) {
        sum_sq += x * x;
        sum_sin += std::sin(3.0 * x);
    }
    return sum_sq + 0.5 * sum_sin;
}

// Numerical gradient using central difference
std::vector<double> numerical_gradient(
    double (*func)(const std::vector<double>&),
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

// L2 norm of a vector
double norm(const std::vector<double>& v) {
    double sum_sq = 0.0;
    for (double x : v) sum_sq += x * x;
    return std::sqrt(sum_sq);
}

std::vector<double> subtract(const std::vector<double>& a, const std::vector<double>& b) {
    std::vector<double> result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        result[i] = a[i] - b[i];
    }
    return result;
}

// Gradient Descent with adaptive learning rate
std::pair<std::vector<double>, std::vector<std::vector<double>>> gradient_descent(
    double (*func)(const std::vector<double>&),
    const std::vector<double>& start_vec,
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

        if (func(new_vec) > func(vec)) {
            lr *= 0.5;
            continue;
        }

        lr *= 1.05;
        history.push_back(new_vec);

        if (iteration % log_interval == 0) {
            std::cout << "Iter " << iteration
                << ": f(x) = " << std::fixed << std::setprecision(6) << func(vec)
                << ", lr = " << std::setprecision(5) << lr << "\n";
        }

        std::vector<double> movement = subtract(new_vec, vec);
        if (norm(movement) < tolerance || norm(grad) < tolerance) {
            std::cout << "Converged at iteration " << iteration << "\n";
            break;
        }

        vec = new_vec;
    }

    return { vec, history };
}

int main() {
    // Seed random generator
    std::mt19937 gen(0);
    std::uniform_real_distribution<double> dist(-3.0, 3.0);

    // Start from a random 5D vector
    std::vector<double> start_point(5);
    for (double& x : start_point) x = dist(gen);

    auto result = gradient_descent(f, start_point, 0.05);

    std::vector<double> minimum_vec = result.first;

    std::cout << "\nFinal result:\n";
    std::cout << "Minimum found at: [ ";
    for (double x : minimum_vec) std::cout << x << " ";
    std::cout << "]\n";
    std::cout << "Function value: " << std::fixed << std::setprecision(6)
        << f(minimum_vec) << "\n";

    return 0;
}
