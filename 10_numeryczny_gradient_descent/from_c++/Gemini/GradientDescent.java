// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescent.java) ---
package org.example;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.function.Function;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.function.Function;
import java.text.DecimalFormat;

public class GradientDescent {

    // ===== Objective function =====
    public static double f(final List<Double> vec) {
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
            final List<Double> vec,
            double h
    ) {
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

    // ===== Gradient descent with adaptive learning rate =====
    public static class Result {
        public List<Double> minimumVec;
        public List<List<Double>> history;

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
            int logInterval
    ) {
        List<Double> vec = new ArrayList<>(startVec);
        double lr = initLr;
        List<List<Double>> history = new ArrayList<>();
        history.add(new ArrayList<>(vec));

        DecimalFormat df6 = new DecimalFormat("#.######");
        DecimalFormat df5 = new DecimalFormat("#.#####");

        for (int iteration = 0; iteration < maxIter; iteration++) {
            List<Double> grad = numericalGradient(func, vec, 1e-5);
            List<Double> newVec = new ArrayList<>(vec.size());
            for (int i = 0; i < vec.size(); i++) {
                newVec.add(vec.get(i) - lr * grad.get(i));
            }

            if (func.apply(newVec) > func.apply(vec)) {
                lr *= 0.5;
                continue;
            }

            lr *= 1.05;
            vec = newVec;
            history.add(new ArrayList<>(vec));

            if (iteration % logInterval == 0) {
                System.out.print("Iter " + iteration + ": f(x) = " + df6.format(func.apply(vec)) + ", lr = " + df5.format(lr) + ", x = [");
                for (int i = 0; i < vec.size(); i++) {
                    System.out.print(df5.format(vec.get(i)));
                    if (i + 1 < vec.size()) {
                        System.out.print(", ");
                    }
                }
                System.out.println("]");
            }

            double moveNorm = 0.0;
            double gradNorm = 0.0;
            if (history.size() > 1) {
                List<Double> prevVec = history.get(history.size() - 2);
                for (int i = 0; i < vec.size(); i++) {
                    moveNorm += Math.pow(vec.get(i) - prevVec.get(i), 2);
                    gradNorm += Math.pow(grad.get(i), 2);
                }
                moveNorm = Math.sqrt(moveNorm);
                gradNorm = Math.sqrt(gradNorm);
            }


            if (moveNorm < tolerance || gradNorm < tolerance) {
                System.out.println("Converged at iteration " + iteration);
                break;
            }
        }

        return new Result(vec, history);
    }

    public static void main(String[] args) {
        Random rng = new Random(0); // Equivalent to C++'s std::mt19937 with seed 0

        List<Double> startPoint = new ArrayList<>();
        for (int i = 0; i < 5; i++) {
            startPoint.add(rng.nextDouble() * 6.0 - 3.0); // uniform_real_distribution from -3.0 to 3.0
        }

        System.out.println("Initial point: " + startPoint);

        Result result = gradientDescent(
                GradientDescent::f,
                startPoint,
                0.05,
                1000,
                1e-6,
                10
        );

        List<Double> minimumVec = result.minimumVec;

        System.out.println("\nFinal result:");
        System.out.print("Minimum found at: [");
        DecimalFormat df6 = new DecimalFormat("#.######");
        for (int i = 0; i < minimumVec.size(); i++) {
            System.out.print(df6.format(minimumVec.get(i)));
            if (i + 1 < minimumVec.size()) {
                System.out.print(", ");
            }
        }
        System.out.println("]");
        System.out.println("Function value: " + df6.format(f(minimumVec)));
    }
}