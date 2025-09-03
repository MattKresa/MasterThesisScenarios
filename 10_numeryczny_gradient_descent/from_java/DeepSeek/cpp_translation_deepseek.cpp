#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <functional>
#include <string>
#include <sstream>
#include <iomanip>

// Function: Sphere + sine perturbation
double f(const std::vector<double>& vec) {
    double sumSquares = 0.0;
    double sumSin = 0.0;
    for (double v : vec) {
        sumSquares += v * v;
        sumSin += sin(3 * v);
    }
    return sumSquares + 0.5 * sumSin;
}

// Numerical gradient (central difference)
std::vector<double> numericalGradient(const std::function<double(const std::vector<double>&)>& func,
    const std::vector<double>& vec,
    double h) {
    std::vector<double> grad(vec.size());
    for (size_t i = 0; i < vec.size(); i++) {
        std::vector<double> vecForward = vec;
        std::vector<double> vecBackward = vec;
        vecForward[i] += h;
        vecBackward[i] -= h;
        grad[i] = (func(vecForward) - func(vecBackward)) / (2 * h);
    }
    return grad;
}

// Euclidean norm (L2)
double norm(const std::vector<double>& vec) {
    double sum = 0.0;
    for (double v : vec) {
        sum += v * v;
    }
    return sqrt(sum);
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
        oss << std::fixed << std::setprecision(6) << std::setw(9) << vec[i];
        if (i < vec.size() - 1) oss << ", ";
    }
    oss << "]";
    return oss.str();
}

// Structure to store the result of gradient descent
struct Result {
    std::vector<double> minimumVec;
    std::vector<std::vector<double>> path;

    Result(const std::vector<double>& minimumVec, const std::vector<std::vector<double>>& path)
        : minimumVec(minimumVec), path(path) {
    }
};

// Gradient Descent with adaptive learning rate
Result gradientDescent(const std::function<double(const std::vector<double>&)>& func,
    const std::vector<double>& startVec,
    double initLr,
    int maxIter,
    double tolerance,
    int logInterval) {

    std::vector<double> vec = startVec;
    double lr = initLr;
    std::vector<std::vector<double>> history;
    history.push_back(vec);

    for (int iteration = 0; iteration < maxIter; iteration++) {
        std::vector<double> grad = numericalGradient(func, vec, 1e-5);
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
            std::cout << "Iter " << iteration << ": f(x) = " << std::fixed << std::setprecision(6)
                << func(vec) << ", x = " << formatVector(vec)
                << ", lr = " << std::setprecision(5) << lr << std::endl;
        }

        // Stop if movement or gradient is small
        if (norm(diff(newVec, vec)) < tolerance || norm(grad) < tolerance) {
            std::cout << "Converged at iteration " << iteration << std::endl;
            vec = newVec;
            break;
        }

        vec = newVec;
    }

    return Result(vec, history);
}

int main() {
    std::mt19937 gen(0); // Standard mersenne_twister_engine seeded with 0
    std::uniform_real_distribution<> dis(-3.0, 3.0);

    std::vector<double> startPoint(5);
    for (size_t i = 0; i < startPoint.size(); i++) {
        startPoint[i] = dis(gen);
    }

    Result result = gradientDescent(f, startPoint, 0.05, 1000, 1e-6, 10);

    std::cout << "\nFinal result:" << std::endl;
    std::cout << "Minimum found at: " << formatVector(result.minimumVec) << std::endl;
    std::cout << "Function value: " << std::fixed << std::setprecision(6)
        << f(result.minimumVec) << std::endl;

    return 0;
}