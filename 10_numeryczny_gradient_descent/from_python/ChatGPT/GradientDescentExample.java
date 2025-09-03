// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescentExample.java) ---
package org.example;

import java.util.*;
import java.util.function.Function;

public class GradientDescentExample {

    // Function f(vec): Sphere function with sine perturbation
    public static double f(double[] vec) {
        double sumSq = 0.0;
        double sumSin = 0.0;
        for (double x : vec) {
            sumSq += x * x;
            sumSin += Math.sin(3.0 * x);
        }
        return sumSq + 0.5 * sumSin;
    }

    // Numerical gradient using central difference
    public static double[] numericalGradient(Function<double[], Double> func, double[] vec, double h) {
        double[] grad = new double[vec.length];
        for (int i = 0; i < vec.length; i++) {
            double[] vecForward = Arrays.copyOf(vec, vec.length);
            double[] vecBackward = Arrays.copyOf(vec, vec.length);

            vecForward[i] += h;
            vecBackward[i] -= h;

            grad[i] = (func.apply(vecForward) - func.apply(vecBackward)) / (2.0 * h);
        }
        return grad;
    }

    // Vector L2 norm
    public static double norm(double[] v) {
        double sumSq = 0.0;
        for (double x : v) sumSq += x * x;
        return Math.sqrt(sumSq);
    }

    // Subtract scaled vector: result = a - scale * b
    public static double[] subtractScaled(double[] a, double[] b, double scale) {
        double[] result = new double[a.length];
        for (int i = 0; i < a.length; i++) {
            result[i] = a[i] - scale * b[i];
        }
        return result;
    }

    // Gradient Descent with adaptive learning rate
    public static Pair<double[], List<double[]>> gradientDescent(
            Function<double[], Double> func,
            double[] startVec,
            double initLr,
            int maxIter,
            double tolerance,
            int logInterval
    ) {
        double[] vec = Arrays.copyOf(startVec, startVec.length);
        double lr = initLr;
        List<double[]> history = new ArrayList<>();
        history.add(Arrays.copyOf(vec, vec.length));

        for (int iteration = 0; iteration < maxIter; iteration++) {
            double[] grad = numericalGradient(func, vec, 1e-5);
            double[] newVec = subtractScaled(vec, grad, lr);

            if (func.apply(newVec) > func.apply(vec)) {
                lr *= 0.5;
                continue;
            }

            lr *= 1.05;
            history.add(Arrays.copyOf(newVec, newVec.length));

            if (iteration % logInterval == 0) {
                System.out.printf("Iter %d: f(x) = %.6f, lr = %.5f%n",
                        iteration, func.apply(vec), lr);
            }

            if (norm(diff(newVec, vec)) < tolerance || norm(grad) < tolerance) {
                System.out.println("Converged at iteration " + iteration);
                break;
            }

            vec = newVec;
        }
        return new Pair<>(vec, history);
    }

    // Vector difference
    public static double[] diff(double[] a, double[] b) {
        double[] result = new double[a.length];
        for (int i = 0; i < a.length; i++) {
            result[i] = a[i] - b[i];
        }
        return result;
    }

    // Simple Pair class
    public static class Pair<U, V> {
        public final U first;
        public final V second;
        public Pair(U first, V second) {
            this.first = first;
            this.second = second;
        }
    }

    public static void main(String[] args) {
        Random rand = new Random(0); // Seed for reproducibility
        double[] startPoint = new double[5];
        for (int i = 0; i < startPoint.length; i++) {
            startPoint[i] = -3 + 6 * rand.nextDouble();
        }

        Pair<double[], List<double[]>> result =
                gradientDescent(GradientDescentExample::f, startPoint, 0.05, 1000, 1e-6, 10);

        double[] minimumVec = result.first;

        System.out.println("\nFinal result:");
        System.out.print("Minimum found at: [ ");
        for (double x : minimumVec) {
            System.out.print(x + " ");
        }
        System.out.println("]");
        System.out.printf("Function value: %.6f%n", f(minimumVec));
    }
}
