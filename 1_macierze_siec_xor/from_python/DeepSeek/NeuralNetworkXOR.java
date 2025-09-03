import java.util.Arrays;
import java.util.Random;

public class NeuralNetworkXOR {

    // Activation functions
    public static double relu(double x) {
        return Math.max(0, x);
    }

    public static double reluDerivative(double x) {
        return x > 0 ? 1 : 0;
    }

    public static double sigmoid(double x) {
        return 1 / (1 + Math.exp(-x));
    }

    public static double sigmoidDerivative(double x) {
        double s = sigmoid(x);
        return s * (1 - s);
    }

    public static void main(String[] args) {
        // Input data (XOR problem)
        double[][] X = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
        double[][] y = {{0}, {1}, {1}, {0}};

        // Network parameters
        int inputSize = 2;
        int hiddenSize = 4;
        int outputSize = 1;
        double learningRate = 0.1;
        int epochs = 10000;

        // Initialize weights with fixed seed for reproducibility
        Random random = new Random(42);
        
        // Initialize weights and biases
        double[][] W1 = new double[inputSize][hiddenSize];
        double[][] b1 = new double[1][hiddenSize];
        double[][] W2 = new double[hiddenSize][outputSize];
        double[][] b2 = new double[1][outputSize];

        // Random initialization
        for (int i = 0; i < inputSize; i++) {
            for (int j = 0; j < hiddenSize; j++) {
                W1[i][j] = random.nextGaussian();
            }
        }

        for (int i = 0; i < hiddenSize; i++) {
            for (int j = 0; j < outputSize; j++) {
                W2[i][j] = random.nextGaussian();
            }
        }

        // Training loop
        for (int epoch = 0; epoch < epochs; epoch++) {
            // Forward pass
            double[][] z1 = matrixAdd(matrixMultiply(X, W1), b1);
            double[][] a1 = applyFunction(z1, NeuralNetworkXOR::relu);

            double[][] z2 = matrixAdd(matrixMultiply(a1, W2), b2);
            double[][] a2 = applyFunction(z2, NeuralNetworkXOR::sigmoid);

            // Calculate loss
            double loss = meanSquaredError(a2, y);

            // Backward pass
            double[][] dz2 = elementWiseMultiply(matrixSubtract(a2, y), 
                                               applyFunction(z2, NeuralNetworkXOR::sigmoidDerivative));
            double[][] dW2 = matrixMultiply(transpose(a1), dz2);
            double[][] db2 = sumRows(dz2);

            double[][] dz1 = elementWiseMultiply(
                matrixMultiply(dz2, transpose(W2)),
                applyFunction(z1, NeuralNetworkXOR::reluDerivative)
            );
            double[][] dW1 = matrixMultiply(transpose(X), dz1);
            double[][] db1 = sumRows(dz1);

            // Update weights
            W2 = matrixSubtract(W2, matrixMultiplyScalar(dW2, learningRate));
            b2 = matrixSubtract(b2, matrixMultiplyScalar(db2, learningRate));
            W1 = matrixSubtract(W1, matrixMultiplyScalar(dW1, learningRate));
            b1 = matrixSubtract(b1, matrixMultiplyScalar(db1, learningRate));

            if (epoch % 1000 == 0) {
                System.out.printf("Epoch %d, loss: %.4f%n", epoch, loss);
            }
        }

        // Predictions
        System.out.println("\nPredictions:");
        double[][] hiddenOutput = applyFunction(matrixAdd(matrixMultiply(X, W1), b1), NeuralNetworkXOR::relu);
        double[][] predictions = applyFunction(matrixAdd(matrixMultiply(hiddenOutput, W2), b2), NeuralNetworkXOR::sigmoid);
        
        for (double[] row : predictions) {
            System.out.println(Arrays.toString(row));
        }
    }

    // Matrix operations
    public static double[][] matrixMultiply(double[][] a, double[][] b) {
        double[][] result = new double[a.length][b[0].length];
        for (int i = 0; i < a.length; i++) {
            for (int j = 0; j < b[0].length; j++) {
                for (int k = 0; k < a[0].length; k++) {
                    result[i][j] += a[i][k] * b[k][j];
                }
            }
        }
        return result;
    }

    public static double[][] matrixAdd(double[][] a, double[][] b) {
        double[][] result = new double[a.length][a[0].length];
        for (int i = 0; i < a.length; i++) {
            for (int j = 0; j < a[0].length; j++) {
                result[i][j] = a[i][j] + b[0][j]; // Broadcasting bias
            }
        }
        return result;
    }

    public static double[][] matrixSubtract(double[][] a, double[][] b) {
        double[][] result = new double[a.length][a[0].length];
        for (int i = 0; i < a.length; i++) {
            for (int j = 0; j < a[0].length; j++) {
                result[i][j] = a[i][j] - b[i][j];
            }
        }
        return result;
    }

    public static double[][] elementWiseMultiply(double[][] a, double[][] b) {
        double[][] result = new double[a.length][a[0].length];
        for (int i = 0; i < a.length; i++) {
            for (int j = 0; j < a[0].length; j++) {
                result[i][j] = a[i][j] * b[i][j];
            }
        }
        return result;
    }

    public static double[][] transpose(double[][] matrix) {
        double[][] result = new double[matrix[0].length][matrix.length];
        for (int i = 0; i < matrix.length; i++) {
            for (int j = 0; j < matrix[0].length; j++) {
                result[j][i] = matrix[i][j];
            }
        }
        return result;
    }

    public static double[][] sumRows(double[][] matrix) {
        double[][] result = new double[1][matrix[0].length];
        for (double[] row : matrix) {
            for (int j = 0; j < row.length; j++) {
                result[0][j] += row[j];
            }
        }
        return result;
    }

    public static double[][] applyFunction(double[][] matrix, ActivationFunction func) {
        double[][] result = new double[matrix.length][matrix[0].length];
        for (int i = 0; i < matrix.length; i++) {
            for (int j = 0; j < matrix[0].length; j++) {
                result[i][j] = func.apply(matrix[i][j]);
            }
        }
        return result;
    }

    public static double[][] matrixMultiplyScalar(double[][] matrix, double scalar) {
        double[][] result = new double[matrix.length][matrix[0].length];
        for (int i = 0; i < matrix.length; i++) {
            for (int j = 0; j < matrix[0].length; j++) {
                result[i][j] = matrix[i][j] * scalar;
            }
        }
        return result;
    }

    public static double meanSquaredError(double[][] predictions, double[][] targets) {
        double sum = 0;
        for (int i = 0; i < predictions.length; i++) {
            for (int j = 0; j < predictions[0].length; j++) {
                double diff = predictions[i][j] - targets[i][j];
                sum += diff * diff;
            }
        }
        return sum / (predictions.length * predictions[0].length);
    }

    interface ActivationFunction {
        double apply(double x);
    }
}