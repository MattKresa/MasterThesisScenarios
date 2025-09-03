#include <iostream>
#include <vector>
#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>
#include <random>

struct Result {
    std::vector<double> minimumVec;
    std::vector<std::vector<double>> path;
};

// Function: Sphere + sine perturbation
double f(const std::vector<double>& vec) {
    double sumSquares = 0.0;
    double sumSin = 0.0;
    for (double v : vec) {
        sumSquares += v * v;
        sumSin += std::sin(3 * v);
    }
    return sumSquares + 0.5 * sumSin;
}

// Numerical gradient (central difference)
std::vector<double> numericalGradient(std::function<double(const std::vector<double>&)> func,
    const std::vector<double>& vec, double h) {
    std::vector<double> grad(vec.size());
    for (size_t i = 0; i < vec.size(); i++) {
        auto vecForward = vec;
        auto vecBackward = vec;
        vecForward[i] += h;
        vecBackward[i] -= h;
        grad[i] = (func(vecForward) - func(vecBackward)) / (2 * h);
    }
    return grad;
}

// Euclidean norm (L2)
double norm(const std::vector<double>& vec) {
    double sum = 0.0;
    for (double v : vec) sum += v * v;
    return std::sqrt(sum);
}

// Helper: difference of two vectors
std::vector<double> diff(const std::vector<double>& a, const std::vector<double>& b) {
    std::vector<double> out(a.size());
    for (size_t i = 0; i < a.size(); i++) {
        out[i] = a[i] - b[i];
    }
    return out;
}

// Helper: format vector like NumPy with 6 decimal places
std::string formatVector(const std::vector<double>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); i++) {
        oss << std::fixed << std::setprecision(6) << std::showpos << vec[i];
        if (i < vec.size() - 1) oss << ", ";
    }
    oss << "]";
    return oss.str();
}

// Gradient Descent with adaptive learning rate
Result gradientDescent(std::function<double(const std::vector<double>&)> func,
    const std::vector<double>& startVec, double initLr,
    int maxIter, double tolerance, int logInterval) {
    auto vec = startVec;
    double lr = initLr;
    std::vector<std::vector<double>> history;
    history.push_back(vec);

    for (int iteration = 0; iteration < maxIter; iteration++) {
        auto grad = numericalGradient(func, vec, 1e-5);
        std::vector<double> newVec(vec.size());
        for (size_t i = 0; i < vec.size(); i++) {
            newVec[i] = vec[i] - lr * grad[i];
        }

        // If the step increases the function value -> decrease learning rate
        if (func(newVec) > func(vec)) {
            lr *= 0.5;
            continue;
        }

        // If the step improves the function -> slightly increase learning rate
        lr *= 1.05;

        history.push_back(newVec);

        // Log progress every `logInterval` iterations
        if (iteration % logInterval == 0) {
            std::cout << "Iter " << iteration << ": f(x) = "
                << std::fixed << std::setprecision(6) << func(vec)
                << ", x = " << formatVector(vec)
                << ", lr = " << std::fixed << std::setprecision(5) << lr
                << "\n";
        }

        // Stop if movement or gradient is small
        if (norm(diff(newVec, vec)) < tolerance || norm(grad) < tolerance) {
            std::cout << "Converged at iteration " << iteration << "\n";
            vec = newVec;
            break;
        }

        vec = newVec;
    }

    return { vec, history };
}

// ====== MAIN ======
int main() {
    std::mt19937 rng(0);
    std::uniform_real_distribution<double> dist(-3.0, 3.0);

    std::vector<double> startPoint(5);
    for (auto& v : startPoint) v = dist(rng);

    auto result = gradientDescent(f, startPoint, 0.05, 1000, 1e-6, 10);

    std::cout << "\nFinal result:\n";
    std::cout << "Minimum found at: " << formatVector(result.minimumVec) << "\n";
    std::cout << "Function value: " << std::fixed << std::setprecision(6)
        << f(result.minimumVec) << "\n";
}
