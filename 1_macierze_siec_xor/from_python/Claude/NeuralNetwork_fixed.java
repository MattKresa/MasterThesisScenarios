import java.util.Random;

class Matrix {
    private double[][] data;
    private int rows;
    private int cols;
    
    public Matrix(int rows, int cols) {
        this.rows = rows;
        this.cols = cols;
        this.data = new double[rows][cols];
    }
    
    public Matrix(double[][] data) {
        this.data = data;
        this.rows = data.length;
        this.cols = data[0].length;
    }
    
    public int getRows() { return rows; }
    public int getCols() { return cols; }
    public double[][] getData() { return data; }
    
    public double get(int row, int col) {
        return data[row][col];
    }
    
    public void set(int row, int col, double value) {
        data[row][col] = value;
    }
    
    // Matrix multiplication
    public Matrix multiply(Matrix other) {
        if (this.cols != other.rows) {
            throw new IllegalArgumentException("Matrix dimensions don't match for multiplication");
        }
        
        Matrix result = new Matrix(this.rows, other.cols);
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < other.cols; j++) {
                for (int k = 0; k < this.cols; k++) {
                    result.data[i][j] += this.data[i][k] * other.data[k][j];
                }
            }
        }
        return result;
    }
    
    // Matrix addition
    public Matrix add(Matrix other) {
        Matrix result = new Matrix(this.rows, this.cols);
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < this.cols; j++) {
                result.data[i][j] = this.data[i][j] + other.data[0][j]; //ISSUE - i replaced by 0
            }
        }
        return result;
    }
    
    // Matrix subtraction
    public Matrix subtract(Matrix other) {
        Matrix result = new Matrix(this.rows, this.cols);
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < this.cols; j++) {
                result.data[i][j] = this.data[i][j] - other.data[i][j];
            }
        }
        return result;
    }
    
    // Scalar multiplication
    public Matrix multiply(double scalar) {
        Matrix result = new Matrix(this.rows, this.cols);
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < this.cols; j++) {
                result.data[i][j] = this.data[i][j] * scalar;
            }
        }
        return result;
    }
    
    // Element-wise multiplication (Hadamard product)
    public Matrix hadamard(Matrix other) {
        Matrix result = new Matrix(this.rows, this.cols);
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < this.cols; j++) {
                result.data[i][j] = this.data[i][j] * other.data[i][j];
            }
        }
        return result;
    }
    
    // Transpose
    public Matrix transpose() {
        Matrix result = new Matrix(this.cols, this.rows);
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < this.cols; j++) {
                result.data[j][i] = this.data[i][j];
            }
        }
        return result;
    }
    
    // Initialize with random values (normal distribution)
    public void randomize(Random random) {
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < this.cols; j++) {
                this.data[i][j] = random.nextGaussian();
            }
        }
    }
    
    // Sum along axis 0 (columns)
    public Matrix sumAxis0() {
        Matrix result = new Matrix(1, this.cols);
        for (int j = 0; j < this.cols; j++) {
            for (int i = 0; i < this.rows; i++) {
                result.data[0][j] += this.data[i][j];
            }
        }
        return result;
    }
    
    // Mean squared error
    public double mse(Matrix target) {
        double sum = 0.0;
        int count = 0;
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < this.cols; j++) {
                double diff = this.data[i][j] - target.data[i][j];
                sum += diff * diff;
                count++;
            }
        }
        return sum / count;
    }
    
    // Print matrix
    public void print() {
        for (int i = 0; i < this.rows; i++) {
            for (int j = 0; j < this.cols; j++) {
                System.out.printf("%.2f ", this.data[i][j]);
            }
            System.out.println();
        }
    }
}

public class NeuralNetwork_fixed {
    
    // Activation functions
    public static Matrix relu(Matrix x) {
        Matrix result = new Matrix(x.getRows(), x.getCols());
        for (int i = 0; i < x.getRows(); i++) {
            for (int j = 0; j < x.getCols(); j++) {
                result.set(i, j, Math.max(0.0, x.get(i, j)));
            }
        }
        return result;
    }
    
    public static Matrix reluDerivative(Matrix x) {
        Matrix result = new Matrix(x.getRows(), x.getCols());
        for (int i = 0; i < x.getRows(); i++) {
            for (int j = 0; j < x.getCols(); j++) {
                result.set(i, j, x.get(i, j) > 0 ? 1.0 : 0.0);
            }
        }
        return result;
    }
    
    public static Matrix sigmoid(Matrix x) {
        Matrix result = new Matrix(x.getRows(), x.getCols());
        for (int i = 0; i < x.getRows(); i++) {
            for (int j = 0; j < x.getCols(); j++) {
                result.set(i, j, 1.0 / (1.0 + Math.exp(-x.get(i, j))));
            }
        }
        return result;
    }
    
    public static Matrix sigmoidDerivative(Matrix x) {
        Matrix s = sigmoid(x);
        Matrix ones = new Matrix(s.getRows(), s.getCols());
        for (int i = 0; i < s.getRows(); i++) {
            for (int j = 0; j < s.getCols(); j++) {
                ones.set(i, j, 1.0);
            }
        }
        return s.hadamard(ones.subtract(s));
    }
    
    public static void main(String[] args) {
        // Training data (XOR problem)
        Matrix X = new Matrix(new double[][]{
            {0, 0},
            {0, 1},
            {1, 0},
            {1, 1}
        });
        
        Matrix y = new Matrix(new double[][]{
            {0},
            {1},
            {1},
            {0}
        });
        
        // Network parameters
        int inputSize = 2;
        int hiddenSize = 4;
        int outputSize = 1;
        double learningRate = 0.1;
        int epochs = 10000;
        
        // Initialize random number generator with seed 42
        Random random = new Random(42);
        
        // Initialize weights and biases
        Matrix W1 = new Matrix(inputSize, hiddenSize);
        Matrix b1 = new Matrix(1, hiddenSize);
        Matrix W2 = new Matrix(hiddenSize, outputSize);
        Matrix b2 = new Matrix(1, outputSize);
        
        W1.randomize(random);
        W2.randomize(random);
        
        // Training loop
        for (int epoch = 0; epoch < epochs; epoch++) {
            // Forward pass
            Matrix z1 = X.multiply(W1).add(b1);
            Matrix a1 = relu(z1);
            
            Matrix z2 = a1.multiply(W2).add(b2);
            Matrix a2 = sigmoid(z2);
            
            // Calculate loss
            double loss = a2.mse(y);
            
            // Backward pass
            Matrix dz2 = a2.subtract(y).hadamard(sigmoidDerivative(z2));
            Matrix dW2 = a1.transpose().multiply(dz2);
            Matrix db2 = dz2.sumAxis0();
            
            Matrix dz1 = dz2.multiply(W2.transpose()).hadamard(reluDerivative(z1));
            Matrix dW1 = X.transpose().multiply(dz1);
            Matrix db1 = dz1.sumAxis0();
            
            // Update weights and biases
            W2 = W2.subtract(dW2.multiply(learningRate));
            b2 = b2.subtract(db2.multiply(learningRate));
            W1 = W1.subtract(dW1.multiply(learningRate));
            b1 = b1.subtract(db1.multiply(learningRate));
            
            // Print progress
            if (epoch % 1000 == 0) {
                System.out.printf("Epoch %d, loss: %.4f%n", epoch, loss);
            }
        }
        
        // Final predictions
        System.out.println("\nPredictions:");
        Matrix z1Final = X.multiply(W1).add(b1);
        Matrix a1Final = relu(z1Final);
        Matrix z2Final = a1Final.multiply(W2).add(b2);
        Matrix predictions = sigmoid(z2Final);
        predictions.print();
    }
}