import math
import random

# === Activation functions ===
def sigmoid(x):
    return 1.0 / (1.0 + math.exp(-x))

def d_sigmoid(x):
    s = sigmoid(x)
    return s * (1 - s)

def relu(x):
    return x if x > 0 else 0

def d_relu(x):
    return 1 if x > 0 else 0

def rand_weight():
    return random.uniform(-1, 1)

# === Training data ===
data = [
    {"input": [0, 0], "output": 0},
    {"input": [0, 1], "output": 1},
    {"input": [1, 0], "output": 1},
    {"input": [1, 1], "output": 0},
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
    total_loss = 0.0
    for sample in data:
        # === Forward pass ===
        Z1 = [B1[i] + sum(W1[i][j] * sample["input"][j] for j in range(input_size)) for i in range(hidden_size)]
        A1 = [relu(z) for z in Z1]

        Z2 = B2 + sum(W2[i] * A1[i] for i in range(hidden_size))
        A2 = sigmoid(Z2)

        # === Loss MSE ===
        y = sample["output"]
        loss = (A2 - y) ** 2
        total_loss += loss

        # === Backward pass ===
        dA2 = A2 - y
        dZ2 = dA2 * d_sigmoid(Z2)

        dW2 = [dZ2 * A1[i] for i in range(hidden_size)]
        dB2 = dZ2

        dA1 = [dZ2 * W2[i] for i in range(hidden_size)]
        dZ1 = [dA1[i] * d_relu(Z1[i]) for i in range(hidden_size)]

        dW1 = [[dZ1[i] * sample["input"][j] for j in range(input_size)] for i in range(hidden_size)]
        dB1 = dZ1[:]

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
    Z1 = [B1[i] + sum(W1[i][j] * sample["input"][j] for j in range(input_size)) for i in range(hidden_size)]
    A1 = [relu(z) for z in Z1]

    Z2 = B2 + sum(W2[i] * A1[i] for i in range(hidden_size))
    A2 = sigmoid(Z2)

    print(f"{sample['input'][0]} XOR {sample['input'][1]} = {A2:.2f}")
