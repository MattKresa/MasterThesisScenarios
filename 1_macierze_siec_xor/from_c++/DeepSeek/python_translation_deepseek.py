import math
import random
import time

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
    return random.random() * 2 - 1

class Sample:
    def __init__(self, input, output):
        self.input = input
        self.output = output

def main():
    random.seed(time.time())
    
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
    
    W2 = [rand_weight() for _ in range(hidden_size)]  # from hidden to output
    B2 = rand_weight()
    
    # Training loop
    for epoch in range(10000):
        total_loss = 0
        
        for sample in data:
            # === Forward pass ===
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
            
            # === Loss MSE ===
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
            
            dW1 = [[0.0 for _ in range(input_size)] for _ in range(hidden_size)]
            dB1 = [0.0] * hidden_size
            
            for i in range(hidden_size):
                for j in range(input_size):
                    dW1[i][j] = dZ1[i] * sample.input[j]
                dB1[i] = dZ1[i]
            
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
        
        print(f"{sample.input[0]:.0f} XOR {sample.input[1]:.0f} = {A2:.2f}")

if __name__ == "__main__":
    main()