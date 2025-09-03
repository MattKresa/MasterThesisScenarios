import math
import random
import numpy as np


# --- Helper functions ---

def f(vec):
    """Function: Sphere + sine perturbation"""
    sum_squares = np.sum(vec ** 2)
    sum_sin = np.sum(np.sin(3 * vec))
    return sum_squares + 0.5 * sum_sin


def numerical_gradient(func, vec, h=1e-5):
    """Numerical gradient (central difference)"""
    grad = np.zeros_like(vec, dtype=float)
    for i in range(len(vec)):
        vec_forward = vec.copy()
        vec_backward = vec.copy()
        vec_forward[i] += h
        vec_backward[i] -= h
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2 * h)
    return grad


def norm(vec):
    """Euclidean norm (L2)"""
    return np.linalg.norm(vec)


def format_vector(vec):
    """Format vector like NumPy with 6 decimal places"""
    return np.array2string(vec, formatter={'float_kind': lambda x: f'{x: .6f}'})


# --- Main gradient descent algorithm ---

def gradient_descent(func, start_vec, init_lr, max_iter, tolerance, log_interval):
    """Gradient Descent with adaptive learning rate"""
    vec = start_vec.copy()
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
            print(f"Iter {iteration}: f(x) = {func(vec):.6f}, x = {format_vector(vec)}, lr = {lr:.5f}")

        # Stop if movement or gradient is small
        if norm(new_vec - vec) < tolerance or norm(grad) < tolerance:
            print(f"Converged at iteration {iteration}")
            vec = new_vec
            break

        vec = new_vec

    return vec, history


# --- MAIN execution block ---

if __name__ == "__main__":
    random.seed(0)
    start_point = np.array([-3 + 6 * random.random() for _ in range(5)])

    minimum_vec, path = gradient_descent(
        f,
        start_point,
        init_lr=0.05,
        max_iter=1000,
        tolerance=1e-6,
        log_interval=10
    )

    print("\nFinal result:")
    print("Minimum found at:", format_vector(minimum_vec))
    print(f"Function value: {f(minimum_vec):.6f}")