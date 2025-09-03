// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/GradientDescent.java) ---
package org.example;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

public class GradientDescent {

    /**
     * The function to be minimized.
     * This is a "Sphere function with a sine perturbation."
     *
     * @param vec The input vector (array of doubles).
     * @return The function value at the given vector.
     */
    public static double f(double[] vec) {
        double sumSq = 0.0;
        double sumSin = 0.0;
        for (double val : vec) {
            sumSq += val * val;
            sumSin += Math.sin(3 * val);
        }
        return sumSq + 0.5 * sumSin;
    }

    /**
     * Calculates the numerical gradient of a function using the central difference method.
     *
     * @param func The function to calculate the gradient for.
     * @param vec The vector at which to calculate the gradient.
     * @param h The small step size for the central difference.
     * @return The gradient vector.
     */
    public static double[] numericalGradient(Function func, double[] vec, double h) {
        double[] grad = new double[vec.length];
        for (int i = 0; i < vec.length; i++) {
            double[] vecForward = Arrays.copyOf(vec, vec.length);
            double[] vecBackward = Arrays.copyOf(vec, vec.length);
            vecForward[i] += h;
            vecBackward[i] -= h;
            grad[i] = (func.evaluate(vecForward) - func.evaluate(vecBackward)) / (2 * h);
        }
        return grad;
    }

    /**
     * Calculates the L2 norm (magnitude) of a vector.
     *
     * @param vec The input vector.
     * @return The L2 norm.
     */
    public static double l2Norm(double[] vec) {
        double sumSq = 0.0;
        for (double val : vec) {
            sumSq += val * val;
        }
        return Math.sqrt(sumSq);
    }

    /**
     * Performs Gradient Descent with an adaptive learning rate.
     *
     * @param func The function to minimize.
     * @param startVec The starting vector for the search.
     * @param initLr The initial learning rate.
     * @param maxIter The maximum number of iterations.
     * @param tolerance The convergence tolerance.
     * @param logInterval The interval for logging progress.
     * @return A list containing the final minimum vector and a list of all vectors in the path.
     */
    public static List<Object> gradientDescent(Function func, double[] startVec, double initLr, int maxIter, double tolerance, int logInterval) {
        double[] vec = Arrays.copyOf(startVec, startVec.length);
        double lr = initLr;
        List<double[]> history = new ArrayList<>();
        history.add(Arrays.copyOf(vec, vec.length));

        for (int iteration = 0; iteration < maxIter; iteration++) {
            double[] grad = numericalGradient(func, vec, 1e-5);
            double[] newVec = new double[vec.length];
            for (int i = 0; i < vec.length; i++) {
                newVec[i] = vec[i] - lr * grad[i];
            }

            // Adaptive learning rate logic
            if (func.evaluate(newVec) > func.evaluate(vec)) {
                lr *= 0.5;
                continue;
            }
            lr *= 1.05;

            history.add(Arrays.copyOf(newVec, newVec.length));

            // Log progress
            if (iteration % logInterval == 0) {
                System.out.printf("Iter %d: f(x) = %.6f, x = %s, lr = %.5f\n",
                        iteration, func.evaluate(vec), Arrays.toString(vec), lr);
            }

            // Convergence check
            double[] diffVec = new double[vec.length];
            for (int i = 0; i < vec.length; i++) {
                diffVec[i] = newVec[i] - vec[i];
            }

            if (l2Norm(diffVec) < tolerance || l2Norm(grad) < tolerance) {
                System.out.printf("Converged at iteration %d\n", iteration);
                vec = newVec;
                break;
            }

            vec = newVec;
        }

        List<Object> result = new ArrayList<>();
        result.add(vec);
        result.add(history);
        return result;
    }

    /**
     * Interface to represent the function being minimized.
     */
    @FunctionalInterface
    interface Function {
        double evaluate(double[] vec);
    }

    public static void main(String[] args) {
        // Example usage
        Random rand = new Random(0); // Seed for reproducibility
        int dimensions = 5;
        double[] startPoint = new double[dimensions];
        for (int i = 0; i < dimensions; i++) {
            startPoint[i] = -3 + 6 * rand.nextDouble(); // Random value in [-3, 3]
        }

        System.out.println("Starting from: " + Arrays.toString(startPoint));

        // Create a lambda function to pass as the 'func' argument
        Function functionToMinimize = GradientDescent::f;

        List<Object> result = gradientDescent(functionToMinimize, startPoint, 0.05, 1000, 1e-6, 10);

        double[] minimumVec = (double[]) result.get(0);

        System.out.println("\nFinal result:");
        System.out.println("Minimum found at: " + Arrays.toString(minimumVec));
        System.out.printf("Function value: %.6f\n", f(minimumVec));
    }
}