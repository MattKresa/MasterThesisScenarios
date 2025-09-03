import numpy as np

# Activation functions
def relu(x):
    return np.maximum(0, x)

def dRelu(x):
    return (x > 0).astype(float)

def sigmoid(x):
    return 1.0 / (1.0 + np.exp(-x))

def dSigmoid(x):
    s = sigmoid(x)
    return s * (1 - s)

# Random weight initialization
def randWeight():
    return np.random.rand() * 2 - 1

def main():
    # XOR data
    inputs = np.array([
        [0, 0],
        [0, 1],
        [1, 0],
        [1, 1]
    ])
    targets = np.array([0, 1, 1, 0])

    # Network architecture
    input_size = 2
    hidden_size = 4
    output_size = 1  # Not explicitly used but good for clarity
    learning_rate = 0.1

    # Weights and biases
    W1 = np.array([[randWeight() for _ in range(input_size)] for _ in range(hidden_size)])
    B1 = np.array([randWeight() for _ in range(hidden_size)])
    W2 = np.array([randWeight() for _ in range(hidden_size)])
    B2 = randWeight()

    # Training loop
    epochs = 10000
    for epoch in range(epochs):
        total_loss = 0

        for i in range(len(inputs)):
            x = inputs[i]
            y = targets[i]

            # === Forward ===
            Z1 = np.dot(W1, x) + B1
            A1 = relu(Z1)

            Z2 = np.dot(W2, A1) + B2
            A2 = sigmoid(Z2)

            # === Loss (MSE) ===
            loss = (A2 - y)**2
            total_loss += loss

            # === Backward ===
            dA2 = 2 * (A2 - y)
            dZ2 = dA2 * dSigmoid(Z2)

            dW2 = dZ2 * A1
            dB2 = dZ2

            dA1 = dZ2 * W2
            dZ1 = dA1 * dRelu(Z1)

            dW1 = np.outer(dZ1, x)
            dB1 = dZ1

            # === Update ===
            W1 -= learning_rate * dW1
            B1 -= learning_rate * dB1
            W2 -= learning_rate * dW2
            B2 -= learning_rate * dB2

        if epoch % 1000 == 0:
            print(f"Epoch {epoch}, Loss: {total_loss / len(inputs):.4f}")

    # === Test ===
    print("\nPredictions:")
    for i in range(len(inputs)):
        x = inputs[i]

        Z1 = np.dot(W1, x) + B1
        A1 = relu(Z1)

        Z2 = np.dot(W2, A1) + B2
        A2 = sigmoid(Z2)

        print(f"{x[0]:.0f} XOR {x[1]:.0f} = {A2:.4f}")

if __name__ == "__main__":
    main()