import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;
import java.text.DecimalFormat;

public class NeuralNetworkXOR {

    // Global Random object for weight initialization
    private static Random random = new Random();

    /**
     * Sigmoid activation function.
     * @param x Input value.
     * @return Sigmoid of x.
     */
    public static double sigmoid(double x) {
        return 1.0 / (1.0 + Math.exp(-x));
    }

    /**
     * Derivative of the sigmoid function.
     * @param x Input value to the sigmoid function.
     * @return Derivative of sigmoid at x.
     */
    public static double d_sigmoid(double x) {
        double s = sigmoid(x);
        return s * (1 - s);
    }

    /**
     * ReLU activation function.
     * @param x Input value.
     * @return ReLU of x.
     */
    public static double relu(double x) {
        return x > 0 ? x : 0;
    }

    /**
     * Derivative of the ReLU function.
     * @param x Input value to the ReLU function.
     * @return Derivative of ReLU at x.
     */
    public static double d_relu(double x) {
        return x > 0 ? 1 : 0;
    }

    /**
     * Generates a random weight between -1 and 1.
     * @return Random double value.
     */
    public static double randWeight() {
        return random.nextDouble() * 2 - 1;
    }

    /**
     * Represents a single training sample with input and expected output.
     */
    static class Sample {
        List<Double> input;
        double output;

        public Sample(List<Double> input, double output) {
            this.input = input;
            this.output = output;
        }
    }

    public static void main(String[] args) {
        // Initialize random seed (equivalent to srand((unsigned)time(0)) in C++)
        random.setSeed(System.currentTimeMillis());

        // Training data for XOR
        List<Sample> data = new ArrayList<>();
        data.add(new Sample(Arrays.asList(0.0, 0.0), 0.0));
        data.add(new Sample(Arrays.asList(0.0, 1.0), 1.0));
        data.add(new Sample(Arrays.asList(1.0, 0.0), 1.0));
        data.add(new Sample(Arrays.asList(1.0, 1.0), 0.0));

        // Network architecture
        final int inputSize = 2;
        final int hiddenSize = 4;
        // final int outputSize = 1; // Not explicitly used as a variable, but output layer has 1 neuron
        double learningRate = 0.1;

        // Weights and biases
        double[][] W1 = new double[hiddenSize][inputSize]; // Weights from input to hidden layer
        double[] B1 = new double[hiddenSize];              // Biases for hidden layer

        double[] W2 = new double[hiddenSize];             // Weights from hidden to output layer
        double B2 = randWeight();                           // Bias for output layer

        // Initialize weights and biases
        for (int i = 0; i < hiddenSize; ++i) {
            B1[i] = randWeight();
            for (int j = 0; j < inputSize; ++j) {
                W1[i][j] = randWeight();
            }
            W2[i] = randWeight();
        }

        // Training loop
        int epochs = 10000;
        for (int epoch = 0; epoch < epochs; ++epoch) {
            double totalLoss = 0;

            for (Sample sample : data) {
                // === Forward pass ===
                double[] Z1 = new double[hiddenSize]; // Weighted sum for hidden layer
                double[] A1 = new double[hiddenSize]; // Activation for hidden layer

                for (int i = 0; i < hiddenSize; ++i) {
                    Z1[i] = B1[i];
                    for (int j = 0; j < inputSize; ++j) {
                        Z1[i] += W1[i][j] * sample.input.get(j);
                    }
                    A1[i] = relu(Z1[i]);
                }

                double Z2 = B2; // Weighted sum for output layer
                for (int i = 0; i < hiddenSize; ++i) {
                    Z2 += W2[i] * A1[i];
                }
                double A2 = sigmoid(Z2); // Activation for output layer (prediction)

                // === Loss (Mean Squared Error) ===
                double y = sample.output;
                double loss = Math.pow(A2 - y, 2);
                totalLoss += loss;

                // === Backward pass ===
                double dA2 = A2 - y;
                double dZ2 = dA2 * d_sigmoid(Z2);

                double[] dW2 = new double[hiddenSize];
                for (int i = 0; i < hiddenSize; ++i) {
                    dW2[i] = dZ2 * A1[i];
                }
                double dB2 = dZ2;

                double[] dA1 = new double[hiddenSize];
                double[] dZ1 = new double[hiddenSize];
                for (int i = 0; i < hiddenSize; ++i) {
                    dA1[i] = dZ2 * W2[i];
                    dZ1[i] = dA1[i] * d_relu(Z1[i]);
                }

                double[][] dW1 = new double[hiddenSize][inputSize];
                double[] dB1 = new double[hiddenSize];

                for (int i = 0; i < hiddenSize; ++i) {
                    for (int j = 0; j < inputSize; ++j) {
                        dW1[i][j] = dZ1[i] * sample.input.get(j);
                    }
                    dB1[i] = dZ1[i];
                }

                // === Update weights ===
                for (int i = 0; i < hiddenSize; ++i) {
                    for (int j = 0; j < inputSize; ++j) {
                        W1[i][j] -= learningRate * dW1[i][j];
                    }
                    B1[i] -= learningRate * dB1[i];
                    W2[i] -= learningRate * dW2[i];
                }
                B2 -= learningRate * dB2;
            }

            if (epoch % 1000 == 0) {
                System.out.printf("Epoch %d, Loss: %.4f%n", epoch, totalLoss);
            }
        }

        // === Test ===
        System.out.println("\nPredictions:");
        DecimalFormat df0 = new DecimalFormat("0");
        DecimalFormat df2 = new DecimalFormat("0.00");

        for (Sample sample : data) {
            // Forward pass for testing
            double[] Z1 = new double[hiddenSize];
            double[] A1 = new double[hiddenSize];
            for (int i = 0; i < hiddenSize; ++i) {
                Z1[i] = B1[i];
                for (int j = 0; j < inputSize; ++j) {
                    Z1[i] += W1[i][j] * sample.input.get(j);
                }
                A1[i] = relu(Z1[i]);
            }

            double Z2 = B2;
            for (int i = 0; i < hiddenSize; ++i) {
                Z2 += W2[i] * A1[i];
            }
            double A2 = sigmoid(Z2);

            System.out.println(df0.format(sample.input.get(0)) + " XOR " +
                               df0.format(sample.input.get(1)) + " = " +
                               df2.format(A2));
        }
    }
}