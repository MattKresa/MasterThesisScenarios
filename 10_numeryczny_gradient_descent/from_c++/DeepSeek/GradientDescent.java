// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescent.java) ---
package org.example;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.function.Function;

public class GradientDescent {

    // ===== Objective function =====
    public static double f(List<Double> vec) {
        double sumSq = 0.0;
        double sumSin = 0.0;
        for (double x : vec) {
            sumSq += x * x;
            sumSin += Math.sin(3.0 * x);
        }
        return sumSq + 0.5 * sumSin;
    }

    // ===== Numerical gradient (central difference) =====
    public static List<Double> numericalGradient(
            Function<List<Double>, Double> func,
            List<Double> vec,
            double h) {
        List<Double> grad = new ArrayList<>(vec.size());
        for (int i = 0; i < vec.size(); i++) {
            grad.add(0.0);
        }

        for (int i = 0; i < vec.size(); i++) {
            List<Double> vecForward = new ArrayList<>(vec);
            List<Double> vecBackward = new ArrayList<>(vec);
            vecForward.set(i, vecForward.get(i) + h);
            vecBackward.set(i, vecBackward.get(i) - h);
            grad.set(i, (func.apply(vecForward) - func.apply(vecBackward)) / (2.0 * h));
        }
        return grad;
    }

    // Overloaded version with default h value
    public static List<Double> numericalGradient(
            Function<List<Double>, Double> func,
            List<Double> vec) {
        return numericalGradient(func, vec, 1e-5);
    }

    // ===== Gradient descent with adaptive learning rate =====
    public static class Result {
        public final List<Double> minimumVec;
        public final List<List<Double>> history;

        public Result(List<Double> minimumVec, List<List<Double>> history) {
            this.minimumVec = minimumVec;
            this.history = history;
        }
    }

    public static Result gradientDescent(
            Function<List<Double>, Double> func,
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
            List<Double> grad = numericalGradient(func, vec);
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
                System.out.printf("Iter %d: f(x) = %.6f, lr = %.5f, x = [",
                        iteration, func.apply(vec), lr);
                for (int i = 0; i < vec.size(); i++) {
                    System.out.printf("%.5f", vec.get(i));
                    if (i + 1 < vec.size()) {
                        System.out.print(", ");
                    }
                }
                System.out.println("]");
            }

            // Stop criteria
            double moveNorm = 0.0, gradNorm = 0.0;
            for (int i = 0; i < vec.size(); i++) {
                double diff = newVec.get(i) - vec.get(i);
                moveNorm += diff * diff;
                gradNorm += grad.get(i) * grad.get(i);
            }
            moveNorm = Math.sqrt(moveNorm);
            gradNorm = Math.sqrt(gradNorm);

            if (moveNorm < tolerance || gradNorm < tolerance) {
                System.out.printf("Converged at iteration %d\n", iteration);
                break;
            }

            vec = new ArrayList<>(newVec);
        }

        return new Result(vec, history);
    }

    // Overloaded version with default parameters
    public static Result gradientDescent(
            Function<List<Double>, Double> func,
            List<Double> startVec,
            double initLr) {
        return gradientDescent(func, startVec, initLr, 1000, 1e-6, 10);
    }

    public static void main(String[] args) {
        // Set random number generator (equivalent to np.random.seed(0))
        Random rng = new Random(0);

        // Start point in 5D
        List<Double> startPoint = new ArrayList<>(5);
        for (int i = 0; i < 5; i++) {
            startPoint.add(-3.0 + 6.0 * rng.nextDouble());
        }

        Result result = gradientDescent(GradientDescent::f, startPoint, 0.05);

        System.out.println("\nFinal result:");
        System.out.print("Minimum found at: [");
        for (int i = 0; i < result.minimumVec.size(); i++) {
            System.out.printf("%.6f", result.minimumVec.get(i));
            if (i + 1 < result.minimumVec.size()) {
                System.out.print(", ");
            }
        }
        System.out.println("]");
        System.out.printf("Function value: %.6f\n", f(result.minimumVec));
    }
}