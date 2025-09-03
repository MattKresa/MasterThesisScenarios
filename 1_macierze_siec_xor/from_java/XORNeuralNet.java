import java.util.*;

public class XORNeuralNet {

    static Random rand = new Random();

    static double relu(double x) {
        return Math.max(0, x);
    }

    static double dRelu(double x) {
        return x > 0 ? 1 : 0;
    }

    static double sigmoid(double x) {
        return 1.0 / (1.0 + Math.exp(-x));
    }

    static double dSigmoid(double x) {
        double s = sigmoid(x);
        return s * (1 - s);
    }

    public static void main(String[] args) {
        // Dane XOR
        double[][] inputs = {
            {0, 0},
            {0, 1},
            {1, 0},
            {1, 1}
        };
        double[] targets = {0, 1, 1, 0};

        // Network architecture
        int inputSize = 2;
        int hiddenSize = 4;
        int outputSize = 1;
        double learningRate = 0.1;

        // Weights and biases
        double[][] W1 = new double[hiddenSize][inputSize];
        double[] B1 = new double[hiddenSize];
        double[] W2 = new double[hiddenSize];
        double B2 = randWeight();

        // Initialize
        for (int i = 0; i < hiddenSize; i++) {
            B1[i] = randWeight();
            W2[i] = randWeight();
            for (int j = 0; j < inputSize; j++) {
                W1[i][j] = randWeight();
            }
        }

        // Training loop
        for (int epoch = 0; epoch < 10000; epoch++) {
            double totalLoss = 0;

            for (int i = 0; i < inputs.length; i++) {
                double[] x = inputs[i];
                double y = targets[i];

                // === Forward ===
                double[] Z1 = new double[hiddenSize];
                double[] A1 = new double[hiddenSize];

                for (int j = 0; j < hiddenSize; j++) {
                    Z1[j] = B1[j];
                    for (int k = 0; k < inputSize; k++) {
                        Z1[j] += W1[j][k] * x[k];
                    }
                    A1[j] = relu(Z1[j]);
                }

                double Z2 = B2;
                for (int j = 0; j < hiddenSize; j++) {
                    Z2 += W2[j] * A1[j];
                }
                double A2 = sigmoid(Z2);

                // === Loss (MSE) ===
                double loss = Math.pow(A2 - y, 2);
                totalLoss += loss;

                // === Backward ===
                double dA2 = 2 * (A2 - y);
                double dZ2 = dA2 * dSigmoid(Z2);

                double[] dW2 = new double[hiddenSize];
                for (int j = 0; j < hiddenSize; j++) {
                    dW2[j] = dZ2 * A1[j];
                }
                double dB2 = dZ2;

                double[] dA1 = new double[hiddenSize];
                double[] dZ1 = new double[hiddenSize];

                for (int j = 0; j < hiddenSize; j++) {
                    dA1[j] = dZ2 * W2[j];
                    dZ1[j] = dA1[j] * dRelu(Z1[j]);
                }

                double[][] dW1 = new double[hiddenSize][inputSize];
                double[] dB1 = new double[hiddenSize];

                for (int j = 0; j < hiddenSize; j++) {
                    for (int k = 0; k < inputSize; k++) {
                        dW1[j][k] = dZ1[j] * x[k];
                    }
                    dB1[j] = dZ1[j];
                }

                // === Update ===
                for (int j = 0; j < hiddenSize; j++) {
                    for (int k = 0; k < inputSize; k++) {
                        W1[j][k] -= learningRate * dW1[j][k];
                    }
                    B1[j] -= learningRate * dB1[j];
                    W2[j] -= learningRate * dW2[j];
                }
                B2 -= learningRate * dB2;
            }

            if (epoch % 1000 == 0) {
                System.out.printf("Epoch %d, Loss: %.4f\n", epoch, totalLoss / inputs.length);
            }
        }

        // === Test ===
        System.out.println("\nPredictions:");
        for (int i = 0; i < inputs.length; i++) {
            double[] x = inputs[i];

            double[] Z1 = new double[hiddenSize];
            double[] A1 = new double[hiddenSize];
            for (int j = 0; j < hiddenSize; j++) {
                Z1[j] = B1[j];
                for (int k = 0; k < inputSize; k++) {
                    Z1[j] += W1[j][k] * x[k];
                }
                A1[j] = relu(Z1[j]);
            }

            double Z2 = B2;
            for (int j = 0; j < hiddenSize; j++) {
                Z2 += W2[j] * A1[j];
            }
            double A2 = sigmoid(Z2);

            System.out.printf("%.0f XOR %.0f = %.4f\n",
                    x[0], x[1], A2);
        }
    }

    static double randWeight() {
        return rand.nextDouble() * 2 - 1;
    }
}
