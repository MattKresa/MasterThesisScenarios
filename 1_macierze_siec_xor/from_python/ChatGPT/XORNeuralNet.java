import org.ejml.simple.SimpleMatrix;
import java.util.Random;

public class XORNeuralNet {

    // Activation functions
    public static SimpleMatrix relu(SimpleMatrix x) {
        SimpleMatrix result = new SimpleMatrix(x.numRows(), x.numCols());
        for (int i = 0; i < x.numRows(); i++) {
            for (int j = 0; j < x.numCols(); j++) {
                result.set(i, j, Math.max(0, x.get(i, j)));
            }
        }
        return result;
    }

    public static SimpleMatrix reluDerivative(SimpleMatrix x) {
        SimpleMatrix result = new SimpleMatrix(x.numRows(), x.numCols());
        for (int i = 0; i < x.numRows(); i++) {
            for (int j = 0; j < x.numCols(); j++) {
                result.set(i, j, x.get(i, j) > 0 ? 1.0 : 0.0);
            }
        }
        return result;
    }

    public static SimpleMatrix sigmoid(SimpleMatrix x) {
        SimpleMatrix result = new SimpleMatrix(x.numRows(), x.numCols());
        for (int i = 0; i < x.numRows(); i++) {
            for (int j = 0; j < x.numCols(); j++) {
                result.set(i, j, 1.0 / (1.0 + Math.exp(-x.get(i, j))));
            }
        }
        return result;
    }

    public static SimpleMatrix sigmoidDerivative(SimpleMatrix x) {
        SimpleMatrix s = sigmoid(x);
        return s.elementMult(SimpleMatrix.identity(s.numRows()).scale(0).plus(1).minus(s));
    }

    public static void main(String[] args) {
        // Data
        double[][] Xdata = {
            {0, 0},
            {0, 1},
            {1, 0},
            {1, 1}
        };
        double[][] ydata = {
            {0},
            {1},
            {1},
            {0}
        };

        SimpleMatrix X = new SimpleMatrix(Xdata);
        SimpleMatrix y = new SimpleMatrix(ydata);

        int inputSize = 2;
        int hiddenSize = 4;
        int outputSize = 1;
        double learningRate = 0.1;
        int epochs = 10000;

        Random rand = new Random(42);

        // Initialize weights & biases
        SimpleMatrix W1 = SimpleMatrix.random_DDRM(inputSize, hiddenSize, -1, 1, rand);
        SimpleMatrix b1 = new SimpleMatrix(1, hiddenSize);
        SimpleMatrix W2 = SimpleMatrix.random_DDRM(hiddenSize, outputSize, -1, 1, rand);
        SimpleMatrix b2 = new SimpleMatrix(1, outputSize);

        for (int epoch = 0; epoch < epochs; epoch++) {
            // Forward
            SimpleMatrix z1 = X.mult(W1).plus(b1);
            SimpleMatrix a1 = relu(z1);

            SimpleMatrix z2 = a1.mult(W2).plus(b2);
            SimpleMatrix a2 = sigmoid(z2);

            double loss = 0.0;
            for (int i = 0; i < a2.numRows(); i++) {
                loss += Math.pow(a2.get(i, 0) - y.get(i, 0), 2);
            }
            loss /= a2.numRows();

            // Backpropagation
            SimpleMatrix dz2 = a2.minus(y).elementMult(sigmoidDerivative(z2));
            SimpleMatrix dW2 = a1.transpose().mult(dz2);
            SimpleMatrix db2 = new SimpleMatrix(1, outputSize);
            for (int j = 0; j < dz2.numCols(); j++) {
                double sum = 0.0;
                for (int i = 0; i < dz2.numRows(); i++) sum += dz2.get(i, j);
                db2.set(0, j, sum);
            }

            SimpleMatrix dz1 = dz2.mult(W2.transpose()).elementMult(reluDerivative(z1));
            SimpleMatrix dW1 = X.transpose().mult(dz1);
            SimpleMatrix db1 = new SimpleMatrix(1, hiddenSize);
            for (int j = 0; j < dz1.numCols(); j++) {
                double sum = 0.0;
                for (int i = 0; i < dz1.numRows(); i++) sum += dz1.get(i, j);
                db1.set(0, j, sum);
            }

            // Update parameters
            W2 = W2.minus(dW2.scale(learningRate));
            b2 = b2.minus(db2.scale(learningRate));
            W1 = W1.minus(dW1.scale(learningRate));
            b1 = b1.minus(db1.scale(learningRate));

            if (epoch % 1000 == 0) {
                System.out.printf("Epoch %d, loss: %.4f%n", epoch, loss);
            }
        }

        // Predictions
        System.out.println("\nPredictions:");
        SimpleMatrix preds = sigmoid(relu(X.mult(W1).plus(b1)).mult(W2).plus(b2));
        for (int i = 0; i < preds.numRows(); i++) {
            System.out.printf("%.2f%n", preds.get(i, 0));
        }
    }
}
