import java.util.Arrays;
import java.util.Random;

public class NeuralNetwork {

    // --- Activation Functions ---

    public static double[][] relu(double[][] x) {
        int rows = x.length;
        int cols = x[0].length;
        double[][] result = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[i][j] = Math.max(0, x[i][j]);
            }
        }
        return result;
    }

    public static double[][] relu_derivative(double[][] x) {
        int rows = x.length;
        int cols = x[0].length;
        double[][] result = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[i][j] = (x[i][j] > 0) ? 1.0 : 0.0;
            }
        }
        return result;
    }

    public static double[][] sigmoid(double[][] x) {
        int rows = x.length;
        int cols = x[0].length;
        double[][] result = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[i][j] = 1.0 / (1.0 + Math.exp(-x[i][j]));
            }
        }
        return result;
    }

    public static double[][] sigmoid_derivative(double[][] x) {
        double[][] s = sigmoid(x);
        int rows = s.length;
        int cols = s[0].length;
        double[][] result = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[i][j] = s[i][j] * (1.0 - s[i][j]);
            }
        }
        return result;
    }

    // --- Basic Matrix Operations ---

    // Matrix multiplication (dot product)
    public static double[][] matMul(double[][] a, double[][] b) {
        int rA = a.length;
        int cA = a[0].length;
        int rB = b.length;
        int cB = b[0].length;

        if (cA != rB) {
            throw new IllegalArgumentException("Matrices cannot be multiplied: inner dimensions must match.");
        }

        double[][] result = new double[rA][cB];
        for (int i = 0; i < rA; i++) {
            for (int j = 0; j < cB; j++) {
                for (int k = 0; k < cA; k++) {
                    result[i][j] += a[i][k] * b[k][j];
                }
            }
        }
        return result;
    }

    // Element-wise addition
    public static double[][] matAdd(double[][] a, double[][] b) {
        int rows = a.length;
        int cols = a[0].length;
        double[][] result = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[i][j] = a[i][j] + b[i][j];
            }
        }
        return result;
    }

    // Element-wise subtraction
    public static double[][] matSub(double[][] a, double[][] b) {
        int rows = a.length;
        int cols = a[0].length;
        double[][] result = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[i][j] = a[i][j] - b[i][j];
            }
        }
        return result;
    }

    // Element-wise multiplication (Hadamard product)
    public static double[][] matMulElementWise(double[][] a, double[][] b) {
        int rows = a.length;
        int cols = a[0].length;
        double[][] result = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[i][j] = a[i][j] * b[i][j];
            }
        }
        return result;
    }

    // Scalar multiplication
    public static double[][] matScalarMul(double[][] a, double scalar) {
        int rows = a.length;
        int cols = a[0].length;
        double[][] result = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[i][j] = a[i][j] * scalar;
            }
        }
        return result;
    }

    // Transpose
    public static double[][] transpose(double[][] matrix) {
        int rows = matrix.length;
        int cols = matrix[0].length;
        double[][] transposed = new double[cols][rows];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                transposed[j][i] = matrix[i][j];
            }
        }
        return transposed;
    }

    // Sum along axis (for db1, db2)
    // axis = 0 means sum columns (result will have 1 row)
    public static double[][] sumAlongAxis(double[][] matrix, int axis) {
        if (axis == 0) { // Sum columns
            int rows = matrix.length;
            int cols = matrix[0].length;
            double[][] result = new double[1][cols];
            for (int j = 0; j < cols; j++) {
                for (int i = 0; i < rows; i++) {
                    result[0][j] += matrix[i][j];
                }
            }
            return result;
        } else {
            throw new IllegalArgumentException("Only axis=0 (column sum) is implemented for sumAlongAxis.");
        }
    }

    // Calculate mean of a matrix
    public static double mean(double[][] matrix) {
        double sum = 0;
        int count = 0;
        for (double[] row : matrix) {
            for (double val : row) {
                sum += val;
                count++;
            }
        }
        return sum / count;
    }
    
    // Broadcast a row vector to match the number of rows of another matrix
    public static double[][] broadcastRow(double[][] rowVector, int numRows) {
        if (rowVector.length != 1) {
            throw new IllegalArgumentException("Input must be a single row vector.");
        }
        int cols = rowVector[0].length;
        double[][] result = new double[numRows][cols];
        for (int i = 0; i < numRows; i++) {
            System.arraycopy(rowVector[0], 0, result[i], 0, cols);
        }
        return result;
    }

    // Initialize matrix with random values (Gaussian-like)
    public static double[][] randn(int rows, int cols, Random rand) {
        double[][] matrix = new double[rows][cols];
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                // Using nextGaussian for a normal distribution
                matrix[i][j] = rand.nextGaussian(); 
            }
        }
        return matrix;
    }

    // Initialize matrix with zeros
    public static double[][] zeros(int rows, int cols) {
        return new double[rows][cols];
    }

    // Print matrix (for debugging)
    public static void printMatrix(double[][] matrix) {
        for (double[] row : matrix) {
            System.out.println(Arrays.toString(row));
        }
    }
    
    // Round predictions to N decimal places
    public static double[][] round(double[][] matrix, int decimalPlaces) {
        double[][] rounded = new double[matrix.length][matrix[0].length];
        double scale = Math.pow(10, decimalPlaces);
        for (int i = 0; i < matrix.length; i++) {
            for (int j = 0; j < matrix[0].length; j++) {
                rounded[i][j] = Math.round(matrix[i][j] * scale) / scale;
            }
        }
        return rounded;
    }


    public static void main(String[] args) {
        // Data
        double[][] X = {
            {0, 0},
            {0, 1},
            {1, 0},
            {1, 1}
        };

        double[][] y = {
            {0},
            {1},
            {1},
            {0}
        };

        // Hyperparameters
        int input_size = 2;
        int hidden_size = 4;
        int output_size = 1;
        double learning_rate = 0.1;
        int epochs = 10000;

        // Random seed
        Random rand = new Random(42);

        // Initialize weights and biases
        double[][] W1 = randn(input_size, hidden_size, rand);
        double[][] b1 = zeros(1, hidden_size);
        double[][] W2 = randn(hidden_size, output_size, rand);
        double[][] b2 = zeros(1, output_size);

        for (int epoch = 0; epoch < epochs; ++epoch) {
            // FORWARD
            double[][] z1_unbiased = matMul(X, W1);
            double[][] b1_broadcasted = broadcastRow(b1, z1_unbiased.length);
            double[][] z1 = matAdd(z1_unbiased, b1_broadcasted);
            double[][] a1 = relu(z1);

            double[][] z2_unbiased = matMul(a1, W2);
            double[][] b2_broadcasted = broadcastRow(b2, z2_unbiased.length);
            double[][] z2 = matAdd(z2_unbiased, b2_broadcasted);
            double[][] a2 = sigmoid(z2);

            // Loss
            double[][] loss_diff = matSub(a2, y);
            double[][] loss_squared = matMulElementWise(loss_diff, loss_diff);
            double loss = mean(loss_squared);

            // BACKWARD
            double[][] dz2_sigmoid_deriv = sigmoid_derivative(z2);
            double[][] dz2 = matMulElementWise(loss_diff, dz2_sigmoid_deriv);
            
            double[][] dW2 = matMul(transpose(a1), dz2);
            double[][] db2 = sumAlongAxis(dz2, 0); // Sum columns

            double[][] W2_T = transpose(W2);
            double[][] dz1_part1 = matMul(dz2, W2_T);
            double[][] dz1_relu_deriv = relu_derivative(z1);
            double[][] dz1 = matMulElementWise(dz1_part1, dz1_relu_deriv);
            
            double[][] dW1 = matMul(transpose(X), dz1);
            double[][] db1 = sumAlongAxis(dz1, 0); // Sum columns

            // Update weights and biases
            W2 = matSub(W2, matScalarMul(dW2, learning_rate));
            b2 = matSub(b2, matScalarMul(db2, learning_rate));
            W1 = matSub(W1, matScalarMul(dW1, learning_rate));
            b1 = matSub(b1, matScalarMul(db1, learning_rate));

            if (epoch % 1000 == 0) {
                System.out.printf("Epoch %d, loss: %.4f%n", epoch, loss);
            }
        }

        System.out.println("\nPredictions:");
        double[][] preds_z1_unbiased = matMul(X, W1);
        double[][] b1_broadcasted_preds = broadcastRow(b1, preds_z1_unbiased.length);
        double[][] preds_z1 = matAdd(preds_z1_unbiased, b1_broadcasted_preds);
        double[][] preds_a1 = relu(preds_z1);

        double[][] preds_z2_unbiased = matMul(preds_a1, W2);
        double[][] b2_broadcasted_preds = broadcastRow(b2, preds_z2_unbiased.length);
        double[][] preds_z2 = matAdd(preds_z2_unbiased, b2_broadcasted_preds);
        double[][] preds = sigmoid(preds_z2);
        
        printMatrix(round(preds, 2));
    }
}