// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescentAdaptive.java) ---
package org.example;

import java.util.*;
import java.util.function.Function;

public class GradientDescentAdaptive {

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
    public static List<Double> numericalGradient(Function<List<Double>, Double> func, List<Double> vec, double h) {
        List<Double> grad = new ArrayList<>(Collections.nCopies(vec.size(), 0.0));
        for (int i = 0; i < vec.size(); i++) {
            List<Double> vecForward = new ArrayList<>(vec);
            List<Double> vecBackward = new ArrayList<>(vec);
            vecForward.set(i, vecForward.get(i) + h);
            vecBackward.set(i, vecBackward.get(i) - h);
            double diff = (func.apply(vecForward) - func.apply(vecBackward)) / (2.0 * h);
            grad.set(i, diff);
        }
        return grad;
    }

    public static List<Double> numericalGradient(Function<List<Double>, Double> func, List<Double> vec) {
        return numericalGradient(func, vec, 1e-5);
    }

    // ===== Gradient descent with adaptive learning rate =====
    public static Pair<List<Double>, List<List<Double>>> gradientDescent(
            Function<List<Double>, Double> func,
            List<Double> startVec,
            double initLr,
            int maxIter,
            double tolerance,
            int logInterval
    ) {
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

            // If step increases the function value -> decrease learning rate
            if (func.apply(newVec) > func.apply(vec)) {
                lr *= 0.5;
                continue;
            }

            // If step improves the function -> slightly increase learning rate
            lr *= 1.05;
            history.add(new ArrayList<>(newVec));

            // Log progress
            if (iteration % logInterval == 0) {
                System.out.printf("Iter %d: f(x) = %.6f, lr = %.5f, x = [%s]%n",
                        iteration,
                        func.apply(vec),
                        lr,
                        formatVector(vec, 5)
                );
            }

            // Stop criteria
            double moveNorm = 0.0;
            double gradNorm = 0.0;
            for (int i = 0; i < vec.size(); i++) {
                double moveDiff = newVec.get(i) - vec.get(i);
                moveNorm += moveDiff * moveDiff;
                gradNorm += grad.get(i) * grad.get(i);
            }
            moveNorm = Math.sqrt(moveNorm);
            gradNorm = Math.sqrt(gradNorm);

            if (moveNorm < tolerance || gradNorm < tolerance) {
                System.out.println("Converged at iteration " + iteration);
                break;
            }

            vec = newVec;
        }

        return new Pair<>(vec, history);
    }

    private static String formatVector(List<Double> vec, int precision) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < vec.size(); i++) {
            sb.append(String.format("%." + precision + "f", vec.get(i)));
            if (i < vec.size() - 1) sb.append(", ");
        }
        return sb.toString();
    }

    public static void main(String[] args) {
        Random rng = new Random(0);

        // Start point in 5D
        List<Double> startPoint = new ArrayList<>();
        for (int i = 0; i < 5; i++) {
            startPoint.add(-3.0 + 6.0 * rng.nextDouble());
        }

        Pair<List<Double>, List<List<Double>>> result =
                gradientDescent(GradientDescentAdaptive::f, startPoint, 0.05, 1000, 1e-6, 10);

        List<Double> minimumVec = result.first;

        System.out.println("\nFinal result:");
        System.out.println("Minimum found at: [" + formatVector(minimumVec, 6) + "]");
        System.out.printf("Function value: %.6f%n", f(minimumVec));
    }

    // Simple Pair class for returning two values
    public static class Pair<F, S> {
        public final F first;
        public final S second;
        public Pair(F first, S second) {
            this.first = first;
            this.second = second;
        }
    }
}
