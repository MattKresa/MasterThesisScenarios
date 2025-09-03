import numpy as np


# Example function (works for any number of dimensions)
def f(vec):
    # Sphere function with a sine perturbation
    return np.sum(vec ** 2) + 0.5 * np.sum(np.sin(3 * vec))


# Numerical gradient using central difference
def numerical_gradient(func, vec, h=1e-5):
    grad = np.zeros_like(vec, dtype=float)
    for i in range(len(vec)):
        vec_forward = np.copy(vec)
        vec_backward = np.copy(vec)
        vec_forward[i] += h
        vec_backward[i] -= h
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2 * h)
    return grad


# Gradient Descent with adaptive learning rate
def gradient_descent(func, start_vec, init_lr=0.1, max_iter=1000, tolerance=1e-6, log_interval=10):
    vec = np.array(start_vec, dtype=float)
    lr = init_lr
    history = [vec.copy()]

    for iteration in range(max_iter):
        grad = numerical_gradient(func, vec)
        new_vec = vec - lr * grad

        # If the step increases the function value -> decrease learning rate
        if func(new_vec) > func(vec):
            lr *= 0.5
            continue

        # If the step improves the function -> slightly increase learning rate
        lr *= 1.05

        history.append(new_vec.copy())

        # Log progress every `log_interval` iterations
        if iteration % log_interval == 0:
            print(f"Iter {iteration}: f(x) = {func(vec):.6f}, x = {vec}, lr = {lr:.5f}")

        # Stop if movement or gradient is small
        if np.linalg.norm(new_vec - vec) < tolerance or np.linalg.norm(grad) < tolerance:
            print(f"Converged at iteration {iteration}")
            break

        vec = new_vec
    return vec, history


# ===== Example usage =====
# Start from a random 5D vector
np.random.seed(0)
start_point = np.random.uniform(-3, 3, size=5)

minimum_vec, path = gradient_descent(f, start_vec=start_point, init_lr=0.05)

print("\nFinal result:")
print(f"Minimum found at: {minimum_vec}")
print(f"Function value: {f(minimum_vec):.6f}")
