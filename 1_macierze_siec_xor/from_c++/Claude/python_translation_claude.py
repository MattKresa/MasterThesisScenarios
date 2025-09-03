import numpy as np
import random

def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))

def d_sigmoid(x):
    s = sigmoid(x)
    return s * (1 - s)

def relu(x):
    return max(0, x)

def d_relu(x):
    return 1 if x > 0 else 0

def rand_weight():
    return random.uniform(-1, 1)

class Sample:
    def __init__(self, input_data, output):
        self.input = input_data
        self.output = output

def main():
    # Set random seed for reproducibility
    random.seed()
    np.random.seed()

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
    output_size = 1
    learning_rate = 0.1

    # Initialize weights and biases
    W1 = [[rand_weight() for _ in range(input_size)] for _ in range(hidden_size)]
    B1 = [rand_weight() for _ in range(hidden_size)]

    W2 = [rand_weight() for _ in range(hidden_size)]
    B2 = rand_weight()

    # Training loop
    for epoch in range(10000):
        total_loss = 0

        for sample in data:
            # === Forward pass ===
            Z1 = [0] * hidden_size
            A1 = [0] * hidden_size

            for i in range(hidden_size):
                Z1[i] = B1[i]
                for j in range(input_size):
                    Z1[i] += W1[i][j] * sample.input[j]
                A1[i] = relu(Z1[i])

            Z2 = B2
            for i in range(hidden_size):
                Z2 += W2[i] * A1[i]
            A2 = sigmoid(Z2)

            # === Loss (MSE) ===
            y = sample.output
            loss = (A2 - y) ** 2
            total_loss += loss

            # === Backward pass ===
            dA2 = A2 - y
            dZ2 = dA2 * d_sigmoid(Z2)

            dW2 = [dZ2 * A1[i] for i in range(hidden_size)]
            dB2 = dZ2

            dA1 = [dZ2 * W2[i] for i in range(hidden_size)]
            dZ1 = [dA1[i] * d_relu(Z1[i]) for i in range(hidden_size)]

            dW1 = [[dZ1[i] * sample.input[j] for j in range(input_size)] for i in range(hidden_size)]
            dB1 = dZ1.copy()

            # === Update weights ===
            for i in range(hidden_size):
                for j in range(input_size):
                    W1[i][j] -= learning_rate * dW1[i][j]
                B1[i] -= learning_rate * dB1[i]
                W2[i] -= learning_rate * dW2[i]
            B2 -= learning_rate * dB2

        if epoch % 1000 == 0:
            print(f"Epoch {epoch}, Loss: {total_loss:.4f}")

    # === Test ===
    print("\nPredictions:")
    for sample in data:
        # Forward pass for testing
        Z1 = [0] * hidden_size
        A1 = [0] * hidden_size

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