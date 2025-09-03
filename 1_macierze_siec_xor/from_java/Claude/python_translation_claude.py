import random
import math

def relu(x):
    return max(0, x)

def d_relu(x):
    return 1 if x > 0 else 0

def sigmoid(x):
    return 1.0 / (1.0 + math.exp(-x))

def d_sigmoid(x):
    s = sigmoid(x)
    return s * (1 - s)

def rand_weight():
    return random.random() * 2 - 1

def main():
    # XOR data
    inputs = [
        [0, 0],
        [0, 1],
        [1, 0],
        [1, 1]
    ]
    targets = [0, 1, 1, 0]

    # Network architecture
    input_size = 2
    hidden_size = 4
    output_size = 1
    learning_rate = 0.1

    # Weights and biases
    W1 = [[rand_weight() for _ in range(input_size)] for _ in range(hidden_size)]
    B1 = [rand_weight() for _ in range(hidden_size)]
    W2 = [rand_weight() for _ in range(hidden_size)]
    B2 = rand_weight()

    # Training loop
    for epoch in range(10000):
        total_loss = 0

        for i in range(len(inputs)):
            x = inputs[i]
            y = targets[i]

            # === Forward ===
            Z1 = [0] * hidden_size
            A1 = [0] * hidden_size

            for j in range(hidden_size):
                Z1[j] = B1[j]
                for k in range(input_size):
                    Z1[j] += W1[j][k] * x[k]
                A1[j] = relu(Z1[j])

            Z2 = B2
            for j in range(hidden_size):
                Z2 += W2[j] * A1[j]
            A2 = sigmoid(Z2)

            # === Loss (MSE) ===
            loss = (A2 - y) ** 2
            total_loss += loss

            # === Backward ===
            dA2 = 2 * (A2 - y)
            dZ2 = dA2 * d_sigmoid(Z2)

            dW2 = [dZ2 * A1[j] for j in range(hidden_size)]
            dB2 = dZ2

            dA1 = [dZ2 * W2[j] for j in range(hidden_size)]
            dZ1 = [dA1[j] * d_relu(Z1[j]) for j in range(hidden_size)]

            dW1 = [[dZ1[j] * x[k] for k in range(input_size)] for j in range(hidden_size)]
            dB1 = dZ1[:]

            # === Update ===
            for j in range(hidden_size):
                for k in range(input_size):
                    W1[j][k] -= learning_rate * dW1[j][k]
                B1[j] -= learning_rate * dB1[j]
                W2[j] -= learning_rate * dW2[j]
            B2 -= learning_rate * dB2

        if epoch % 1000 == 0:
            print(f"Epoch {epoch}, Loss: {total_loss / len(inputs):.4f}")

    # === Test ===
    print("\nPredictions:")
    for i in range(len(inputs)):
        x = inputs[i]

        Z1 = [0] * hidden_size
        A1 = [0] * hidden_size
        for j in range(hidden_size):
            Z1[j] = B1[j]
            for k in range(input_size):
                Z1[j] += W1[j][k] * x[k]
            A1[j] = relu(Z1[j])

        Z2 = B2
        for j in range(hidden_size):
            Z2 += W2[j] * A1[j]
        A2 = sigmoid(Z2)

        print(f"{x[0]:.0f} XOR {x[1]:.0f} = {A2:.4f}")

if __name__ == "__main__":
    main()