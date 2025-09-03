// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescent.java) ---
package org.example;

import java.util.*;
import java.util.function.Function;

public class GradientDescent {

    // Functional interface for objective functions
    @FunctionalInterface
    public interface ObjectiveFunction {
        double apply(double[] vec);
    }

    // Result class to hold the optimization results
    public static class OptimizationResult {
        public final double[] minimumVec;
        public final List<double[]> history;

        public OptimizationResult(double[] minimumVec, List<double[]> history) {
            this.minimumVec = minimumVec;
            this.history = history;
        }
    }

    // Example function (works for any number of dimensions)
    public static double f(double[] vec) {
        // Sphere function with a sine perturbation
        double sumSquares = 0.0;
        double sumSin = 0.0;

        for (double x : vec) {
            sumSquares += x * x;
            sumSin += Math.sin(3.0 * x);
        }

        return sumSquares + 0.5 * sumSin;
    }

    // Numerical gradient using central difference
    public static double[] numericalGradient(ObjectiveFunction func, double[] vec, double h) {
        double[] grad = new double[vec.length];

        for (int i = 0; i < vec.length; i++) {
            double[] vecForward = vec.clone();
            double[] vecBackward = vec.clone();

            vecForward[i] += h;
            vecBackward[i] -= h;

            grad[i] = (func.apply(vecForward) - func.apply(vecBackward)) / (2.0 * h);
        }

        return grad;
    }

    // Overloaded version with default h value
    public static double[] numericalGradient(ObjectiveFunction func, double[] vec) {
        return numericalGradient(func, vec, 1e-5);
    }

    // Calculate L2 norm of a vector
    public static double norm(double[] vec) {
        double sum = 0.0;
        for (double x : vec) {
            sum += x * x;
        }
        return Math.sqrt(sum);
    }

    // Vector subtraction
    public static double[] subtract(double[] a, double[] b) {
        double[] result = new double[a.length];
        for (int i = 0; i < a.length; i++) {
            result[i] = a[i] - b[i];
        }
        return result;
    }

    // Vector scalar multiplication
    public static double[] multiply(double[] vec, double scalar) {
        double[] result = new double[vec.length];
        for (int i = 0; i < vec.length; i++) {
            result[i] = vec[i] * scalar;
        }
        return result;
    }

    // Print vector as formatted string
    public static String vectorToString(double[] vec) {
        StringBuilder sb = new StringBuilder("[");
        for (int i = 0; i < vec.length; i++) {
            sb.append(String.format("%.6f", vec[i]));
            if (i < vec.length - 1) {
                sb.append(", ");
            }
        }
        sb.append("]");
        return sb.toString();
    }

    // Gradient Descent with adaptive learning rate
    public static OptimizationResult gradientDescent(ObjectiveFunction func,
                                                     double[] startVec,
                                                     double initLr,
                                                     int maxIter,
                                                     double tolerance,
                                                     int logInterval) {

        double[] vec = startVec.clone();
        double lr = initLr;
        List<double[]> history = new ArrayList<>();
        history.add(vec.clone());

        for (int iteration = 0; iteration < maxIter; iteration++) {
            double[] grad = numericalGradient(func, vec);
            double[] newVec = subtract(vec, multiply(grad, lr));

            // If the step increases the function value -> decrease learning rate
            if (func.apply(newVec) > func.apply(vec)) {
                lr *= 0.5;
                continue;
            }

            // If the step improves the function -> slightly increase learning rate
            lr *= 1.05;

            history.add(newVec.clone());

            // Log progress every `logInterval` iterations
            if (iteration % logInterval == 0) {
                System.out.printf("Iter %d: f(x) = %.6f, x = %s, lr = %.5f%n",
                        iteration, func.apply(vec), vectorToString(vec), lr);
            }

            // Stop if movement or gradient is small
            double[] movement = subtract(newVec, vec);
            if (norm(movement) < tolerance || norm(grad) < tolerance) {
                System.out.printf("Converged at iteration %d%n", iteration);
                break;
            }

            vec = newVec;
        }

        return new OptimizationResult(vec, history);
    }

    // Overloaded version with default parameters
    public static OptimizationResult gradientDescent(ObjectiveFunction func, double[] startVec) {
        return gradientDescent(func, startVec, 0.1, 1000, 1e-6, 10);
    }

    // Overloaded version with custom initial learning rate
    public static OptimizationResult gradientDescent(ObjectiveFunction func, double[] startVec, double initLr) {
        return gradientDescent(func, startVec, initLr, 1000, 1e-6, 10);
    }

    // ===== Example usage =====
    public static void main(String[] args) {
        // Start from a random 5D vector
        Random random = new Random(0); // Using seed 0 for reproducibility
        double[] startPoint = new double[5];

        for (int i = 0; i < startPoint.length; i++) {
            startPoint[i] = -3.0 + random.nextDouble() * 6.0; // uniform(-3, 3)
        }

        // Create objective function using method reference
        ObjectiveFunction objective = GradientDescent::f;

        OptimizationResult result = gradientDescent(objective, startPoint, 0.05);

        System.out.println("\nFinal result:");
        System.out.println("Minimum found at: " + vectorToString(result.minimumVec));
        System.out.printf("Function value: %.6f%n", f(result.minimumVec));
    }
}