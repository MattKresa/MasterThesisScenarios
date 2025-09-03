// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescent.java) ---
package org.example;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

public class GradientDescent {

    // Function: Sphere + sine perturbation
    public static double f(double[] vec) {
        double sumSquares = 0.0;
        double sumSin = 0.0;
        for (double v : vec) {
            sumSquares += v * v;
            sumSin += Math.sin(3 * v);
        }
        return sumSquares + 0.5 * sumSin;
    }

    // Numerical gradient (central difference)
    public static double[] numericalGradient(Function func, double[] vec, double h) {
        double[] grad = new double[vec.length];
        for (int i = 0; i < vec.length; i++) {
            double[] vecForward = vec.clone();
            double[] vecBackward = vec.clone();
            vecForward[i] += h;
            vecBackward[i] -= h;
            grad[i] = (func.apply(vecForward) - func.apply(vecBackward)) / (2 * h);
        }
        return grad;
    }

    // Euclidean norm (L2)
    public static double norm(double[] vec) {
        double sum = 0.0;
        for (double v : vec) {
            sum += v * v;
        }
        return Math.sqrt(sum);
    }

    // Gradient Descent with adaptive learning rate
    public static Result gradientDescent(Function func, double[] startVec, double initLr,
                                         int maxIter, double tolerance, int logInterval) {

        double[] vec = startVec.clone();
        double lr = initLr;
        List<double[]> history = new ArrayList<>();
        history.add(vec.clone());

        for (int iteration = 0; iteration < maxIter; iteration++) {
            double[] grad = numericalGradient(func, vec, 1e-5);
            double[] newVec = new double[vec.length];
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

            // Log progress every `logInterval` iterations
            if (iteration % logInterval == 0) {
                System.out.printf("Iter %d: f(x) = %.6f, x = %s, lr = %.5f%n",
                        iteration, func.apply(vec), formatVector(vec), lr);
            }

            // Stop if movement or gradient is small
            if (norm(diff(newVec, vec)) < tolerance || norm(grad) < tolerance) {
                System.out.printf("Converged at iteration %d%n", iteration);
                vec = newVec;
                break;
            }

            vec = newVec;
        }

        return new Result(vec, history);
    }

    // Helper: difference of two vectors
    public static double[] diff(double[] a, double[] b) {
        double[] out = new double[a.length];
        for (int i = 0; i < a.length; i++) {
            out[i] = a[i] - b[i];
        }
        return out;
    }

    // Helper: format vector like NumPy with 6 decimal places
    public static String formatVector(double[] vec) {
        StringBuilder sb = new StringBuilder("[");
        for (int i = 0; i < vec.length; i++) {
            sb.append(String.format("% .6f", vec[i])); // space for alignment like NumPy
            if (i < vec.length - 1) sb.append(", ");
        }
        sb.append("]");
        return sb.toString();
    }

    // Functional interface for a function that takes a vector and returns a scalar
    @FunctionalInterface
    interface Function {
        double apply(double[] vec);
    }

    // Structure to store the result of gradient descent
    static class Result {
        double[] minimumVec;
        List<double[]> path;

        Result(double[] minimumVec, List<double[]> path) {
            this.minimumVec = minimumVec;
            this.path = path;
        }
    }

    // ====== MAIN ======
    public static void main(String[] args) {
        Random random = new Random(0);
        double[] startPoint = new double[5];
        for (int i = 0; i < startPoint.length; i++) {
            startPoint[i] = -3 + 6 * random.nextDouble();
        }

        Result result = gradientDescent(GradientDescent::f,
                startPoint, 0.05, 1000, 1e-6, 10);

        System.out.println("\nFinal result:");
        System.out.println("Minimum found at: " + formatVector(result.minimumVec));
        System.out.printf("Function value: %.6f%n", f(result.minimumVec));
    }
}
