import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class NeuralNetworkXOR {
    
    private static double sigmoid(double x) {
        return 1.0 / (1.0 + Math.exp(-x));
    }
    
    private static double dSigmoid(double x) {
        double s = sigmoid(x);
        return s * (1 - s);
    }
    
    private static double relu(double x) {
        return x > 0 ? x : 0;
    }
    
    private static double dRelu(double x) {
        return x > 0 ? 1 : 0;
    }
    
    private static double randWeight(Random rand) {
        return rand.nextDouble() * 2 - 1;
    }
    
    static class Sample {
        List<Double> input;
        double output;
        
        public Sample(List<Double> input, double output) {
            this.input = input;
            this.output = output;
        }
    }
    
    public static void main(String[] args) {
        Random rand = new Random(System.currentTimeMillis());
        
        List<Sample> data = new ArrayList<>();
        data.add(new Sample(List.of(0.0, 0.0), 0));
        data.add(new Sample(List.of(0.0, 1.0), 1));
        data.add(new Sample(List.of(1.0, 0.0), 1));
        data.add(new Sample(List.of(1.0, 1.0), 0));
        
        // Network architecture
        final int inputSize = 2;
        final int hiddenSize = 4;
        final int outputSize = 1;
        double learningRate = 0.1;
        
        // Weights and biases
        double[][] W1 = new double[hiddenSize][inputSize];
        double[] B1 = new double[hiddenSize];
        
        double[] W2 = new double[hiddenSize];  // from hidden to output
        double B2 = randWeight(rand);
        
        // Initialize weights
        for (int i = 0; i < hiddenSize; ++i) {
            B1[i] = randWeight(rand);
            for (int j = 0; j < inputSize; ++j) {
                W1[i][j] = randWeight(rand);
            }
            W2[i] = randWeight(rand);
        }
        
        // Training loop
        for (int epoch = 0; epoch < 10000; ++epoch) {
            double totalLoss = 0;
            
            for (Sample sample : data) {
                // === Forward pass ===
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
                
                // === Loss MSE ===
                double y = sample.output;
                double loss = Math.pow(A2 - y, 2);
                totalLoss += loss;
                
                // === Backward pass ===
                double dA2 = A2 - y;
                double dZ2 = dA2 * dSigmoid(Z2);
                
                double[] dW2 = new double[hiddenSize];
                for (int i = 0; i < hiddenSize; ++i) {
                    dW2[i] = dZ2 * A1[i];
                }
                double dB2 = dZ2;
                
                double[] dA1 = new double[hiddenSize];
                double[] dZ1 = new double[hiddenSize];
                for (int i = 0; i < hiddenSize; ++i) {
                    dA1[i] = dZ2 * W2[i];
                    dZ1[i] = dA1[i] * dRelu(Z1[i]);
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
        for (Sample sample : data) {
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
            
            System.out.printf("%.0f XOR %.0f = %.2f%n", 
                sample.input.get(0), sample.input.get(1), A2);
        }
    }
}