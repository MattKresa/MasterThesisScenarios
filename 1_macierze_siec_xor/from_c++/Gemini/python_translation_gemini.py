import math
import random

def sigmoid(x):
    """Sigmoid activation function."""
    return 1.0 / (1.0 + math.exp(-x))

def d_sigmoid(x):
    """Derivative of the sigmoid function."""
    s = sigmoid(x)
    return s * (1 - s)

def relu(x):
    """ReLU activation function."""
    return x if x > 0 else 0

def d_relu(x):
    """Derivative of the ReLU function."""
    return 1 if x > 0 else 0

def rand_weight():
    """Generates a random weight between -1 and 1."""
    return random.uniform(-1, 1)

class Sample:
    """Represents a single training sample with input and expected output."""
    def __init__(self, input_data, output):
        self.input = input_data
        self.output = output

def main():
    # Training data for XOR
    data = [
        Sample([0, 0], 0),
        Sample([0, 1], 1),
        Sample([1, 0], 1),
        Sample([1, 1], 0)
    ]

    # Network architecture
    input_size = 2
    hidden_size = 4
    output_size = 1  # Though not explicitly used as a variable in the C++ code, it's conceptually 1
    learning_rate = 0.1

    # Weights and biases
    # W1: Weights from input to hidden layer (hidden_size x input_size matrix)
    W1 = [[0.0] * input_size for _ in range(hidden_size)]
    B1 = [0.0] * hidden_size  # Biases for hidden layer

    # W2: Weights from hidden to output layer (1 x hidden_size vector)
    W2 = [0.0] * hidden_size
    B2 = rand_weight()  # Bias for output layer

    # Initialize weights and biases
    for i in range(hidden_size):
        B1[i] = rand_weight()
        for j in range(input_size):
            W1[i][j] = rand_weight()
        W2[i] = rand_weight()

    # Training loop
    epochs = 10000
    for epoch in range(epochs):
        total_loss = 0

        for sample in data:
            # === Forward pass ===
            Z1 = [0.0] * hidden_size  # Weighted sum for hidden layer
            A1 = [0.0] * hidden_size  # Activation for hidden layer

            for i in range(hidden_size):
                Z1[i] = B1[i]
                for j in range(input_size):
                    Z1[i] += W1[i][j] * sample.input[j]
                A1[i] = relu(Z1[i])

            Z2 = B2  # Weighted sum for output layer
            for i in range(hidden_size):
                Z2 += W2[i] * A1[i]
            A2 = sigmoid(Z2)  # Activation for output layer (prediction)

            # === Loss (Mean Squared Error) ===
            y = sample.output
            loss = (A2 - y) ** 2
            total_loss += loss

            # === Backward pass ===
            dA2 = A2 - y
            dZ2 = dA2 * d_sigmoid(Z2)

            dW2 = [0.0] * hidden_size
            for i in range(hidden_size):
                dW2[i] = dZ2 * A1[i]
            dB2 = dZ2

            dA1 = [0.0] * hidden_size
            dZ1 = [0.0] * hidden_size
            for i in range(hidden_size):
                dA1[i] = dZ2 * W2[i]
                dZ1[i] = dA1[i] * d_relu(Z1[i])

            dW1 = [[0.0] * input_size for _ in range(hidden_size)]
            dB1 = [0.0] * hidden_size

            for i in range(hidden_size):
                for j in range(input_size):
                    dW1[i][j] = dZ1[i] * sample.input[j]
                dB1[i] = dZ1[i]

            # === Update weights and biases ===
            for i in range(hidden_size):
                for j in range(input_size):
                    W1[i][j] -= learning_rate * dW1[i][j]
                B1[i] -= learning_rate * dB1[i]
                W2[i] -= learning_rate * dW2[i]
            B2 -= learning_rate * dB2

        if epoch % 1000 == 0:
            print(f"Epoch {epoch}, Loss: {total_loss:.4f}")

    # --- Test ---
    print("\nPredictions:")
    for sample in data:
        # Forward pass for testing
        Z1 = [0.0] * hidden_size
        A1 = [0.0] * hidden_size
        for i in range(hidden_size):
            Z1[i] = B1[i]
            for j in range(input_size):
                Z1[i] += W1[i][j] * sample.input[j]
            A1[i] = relu(Z1[i])

        Z2 = B2
        for i in range(hidden_size):
            Z2 += W2[i] * A1[i]
        A2 = sigmoid(Z2)

        print(f"{int(sample.input[0])} XOR {int(sample.input[1])} = {A2:.2f}")

if __name__ == "__main__":
    main()