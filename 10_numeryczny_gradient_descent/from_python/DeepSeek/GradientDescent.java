// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescent.java) ---
package org.example;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.function.Function;

public class GradientDescent {

    // Example function (works for any number of dimensions)
    public static double f(List<Double> vec) {
        double sumSq = 0.0;
        double sumSin = 0.0;
        for (double v : vec) {
            sumSq += v * v;
            sumSin += Math.sin(3 * v);
        }
        return sumSq + 0.5 * sumSin;
    }

    // Numerical gradient using central difference
    public static List<Double> numericalGradient(Function<List<Double>, Double> func,
                                                 List<Double> vec, double h) {
        List<Double> grad = new ArrayList<>(vec.size());
        for (int i = 0; i < vec.size(); i++) {
            List<Double> vecForward = new ArrayList<>(vec);
            List<Double> vecBackward = new ArrayList<>(vec);
            vecForward.set(i, vecForward.get(i) + h);
            vecBackward.set(i, vecBackward.get(i) - h);
            grad.add((func.apply(vecForward) - func.apply(vecBackward)) / (2 * h));
        }
        return grad;
    }

    // Helper function to compute vector norm
    public static double vectorNorm(List<Double> vec) {
        double sum = 0.0;
        for (double v : vec) {
            sum += v * v;
        }
        return Math.sqrt(sum);
    }

    // Gradient Descent with adaptive learning rate
    public static class Result {
        public final List<Double> minimum;
        public final List<List<Double>> history;

        public Result(List<Double> minimum, List<List<Double>> history) {
            this.minimum = minimum;
            this.history = history;
        }
    }

    public static Result gradientDescent(Function<List<Double>, Double> func,
                                         List<Double> startVec,
                                         double initLr,
                                         int maxIter,
                                         double tolerance,
                                         int logInterval) {

        List<Double> vec = new ArrayList<>(startVec);
        double lr = initLr;
        List<List<Double>> history = new ArrayList<>();
        history.add(new ArrayList<>(vec));

        for (int iteration = 0; iteration < maxIter; iteration++) {
            List<Double> grad = numericalGradient(func, vec, 1e-5);
            List<Double> newVec = new ArrayList<>(vec.size());

            for (int i = 0; i < vec.size(); i++) {
                newVec.add(vec.get(i) - lr * grad.get(i));
            }

            // If the step increases the function value -> decrease learning rate
            if (func.apply(newVec) > func.apply(vec)) {
                lr *= 0.5;
                continue;
            }

            // If the step improves the function -> slightly increase learning rate
            lr *= 1.05;
            history.add(new ArrayList<>(newVec));

            // Log progress every `logInterval` iterations
            if (iteration % logInterval == 0) {
                System.out.printf("Iter %d: f(x) = %.6f, x = %s, lr = %.5f%n",
                        iteration, func.apply(vec), vec, lr);
            }

            // Stop if movement or gradient is small
            double vecDiffNorm = 0.0;
            double gradNorm = 0.0;
            for (int i = 0; i < vec.size(); i++) {
                double diff = newVec.get(i) - vec.get(i);
                vecDiffNorm += diff * diff;
                gradNorm += grad.get(i) * grad.get(i);
            }
            vecDiffNorm = Math.sqrt(vecDiffNorm);
            gradNorm = Math.sqrt(gradNorm);

            if (vecDiffNorm < tolerance || gradNorm < tolerance) {
                System.out.printf("Converged at iteration %d%n", iteration);
                break;
            }

            vec = new ArrayList<>(newVec);
        }
        return new Result(vec, history);
    }

    public static void main(String[] args) {
        // Start from a random 5D vector
        Random random = new Random(0);
        List<Double> startPoint = new ArrayList<>(5);
        for (int i = 0; i < 5; i++) {
            startPoint.add(-3 + 6 * random.nextDouble());
        }

        Result result = gradientDescent(GradientDescent::f, startPoint, 0.05, 1000, 1e-6, 10);

        System.out.println("\nFinal result:");
        System.out.println("Minimum found at: " + result.minimum);
        System.out.printf("Function value: %.6f%n", f(result.minimum));
    }
}