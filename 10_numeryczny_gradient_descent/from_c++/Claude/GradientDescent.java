// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescent.java) ---
package org.example;

import java.util.*;
import java.util.function.Function;

public class GradientDescent {

    // ===== Objective function =====
    public static double f(double[] vec) {
        double sumSq = 0.0;
        double sumSin = 0.0;
        for (double x : vec) {
            sumSq += x * x;
            sumSin += Math.sin(3.0 * x);
        }
        return sumSq + 0.5 * sumSin;
    }

    // ===== Numerical gradient (central difference) =====
    public static double[] numericalGradient(Function<double[], Double> func,
                                             double[] vec, double h) {
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

    // Overloaded method with default h value
    public static double[] numericalGradient(Function<double[], Double> func, double[] vec) {
        return numericalGradient(func, vec, 1e-5);
    }

    // ===== Result class to hold optimization results =====
    public static class OptimizationResult {
        public final double[] minimumVec;
        public final List<double[]> history;

        public OptimizationResult(double[] minimumVec, List<double[]> history) {
            this.minimumVec = minimumVec;
            this.history = history;
        }
    }

    // ===== Gradient descent with adaptive learning rate =====
    public static OptimizationResult gradientDescent(Function<double[], Double> func,
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
            double[] newVec = new double[vec.length];

            // Calculate new vector: newVec = vec - lr * grad
            for (int i = 0; i < vec.length; i++) {
                newVec[i] = vec[i] - lr * grad[i];
            }

            // If the step increases the function value -> decrease learning rate
            if (func.apply(newVec) > func.apply(vec)) {
                lr *= 0.5;
                continue;
            }

            // If the step improves the function -> slightly increase learning rate
            lr *= 1.05;
            history.add(newVec.clone());

            // Log progress every logInterval iterations
            if (iteration % logInterval == 0) {
                System.out.printf("Iter %d: f(x) = %.6f, lr = %.5f, x = %s%n",
                        iteration, func.apply(vec), lr, Arrays.toString(vec));
            }

            // Stop criteria
            double moveNorm = 0.0;
            double gradNorm = 0.0;

            for (int i = 0; i < vec.length; i++) {
                double diff = newVec[i] - vec[i];
                moveNorm += diff * diff;
                gradNorm += grad[i] * grad[i];
            }

            moveNorm = Math.sqrt(moveNorm);
            gradNorm = Math.sqrt(gradNorm);

            if (moveNorm < tolerance || gradNorm < tolerance) {
                System.out.println("Converged at iteration " + iteration);
                break;
            }

            vec = newVec.clone();
        }

        return new OptimizationResult(vec, history);
    }

    // Overloaded method with default parameters
    public static OptimizationResult gradientDescent(Function<double[], Double> func,
                                                     double[] startVec,
                                                     double initLr) {
        return gradientDescent(func, startVec, initLr, 1000, 1e-6, 10);
    }

    public static void main(String[] args) {
        // Set random number generator (equivalent to C++ mt19937(0))
        Random rng = new Random(0);

        // Start point in 5D
        double[] startPoint = new double[5];
        for (int i = 0; i < startPoint.length; i++) {
            // Generate random values between -3.0 and 3.0
            startPoint[i] = -3.0 + rng.nextDouble() * 6.0;
        }

        // Create function reference
        Function<double[], Double> objectiveFunction = GradientDescent::f;

        OptimizationResult result = gradientDescent(objectiveFunction, startPoint, 0.05);

        System.out.println("\nFinal result:");
        System.out.print("Minimum found at: [");
        for (int i = 0; i < result.minimumVec.length; i++) {
            System.out.printf("%.6f", result.minimumVec[i]);
            if (i + 1 < result.minimumVec.length) {
                System.out.print(", ");
            }
        }
        System.out.println("]");
        System.out.printf("Function value: %.6f%n", f(result.minimumVec));
    }
}