import numpy as np
import random
from typing import Callable, Tuple, List


def f(vec: np.ndarray) -> float:
    """Function: Sphere + sine perturbation"""
    sum_squares = np.sum(vec ** 2)
    sum_sin = np.sum(np.sin(3 * vec))
    return sum_squares + 0.5 * sum_sin


def numerical_gradient(func: Callable[[np.ndarray], float], vec: np.ndarray, h: float = 1e-5) -> np.ndarray:
    """Numerical gradient (central difference)"""
    grad = np.zeros_like(vec)
    for i in range(len(vec)):
        vec_forward = vec.copy()
        vec_backward = vec.copy()
        vec_forward[i] += h
        vec_backward[i] -= h
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2 * h)
    return grad


def gradient_descent(func: Callable[[np.ndarray], float],
                     start_vec: np.ndarray,
                     init_lr: float,
                     max_iter: int,
                     tolerance: float,
                     log_interval: int) -> Tuple[np.ndarray, List[np.ndarray]]:
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
        if np.linalg.norm(new_vec - vec) < tolerance or np.linalg.norm(grad) < tolerance:
            print(f"Converged at iteration {iteration}")
            vec = new_vec
            break

        vec = new_vec

    return vec, history


def format_vector(vec: np.ndarray) -> str:
    """Format vector like NumPy with 6 decimal places"""
    formatted_values = [f" {x:.6f}" if x >= 0 else f"{x:.6f}" for x in vec]
    return "[" + ", ".join(formatted_values) + "]"


def main():
    """Main function"""
    random.seed(0)
    np.random.seed(0)

    # Generate random starting point
    start_point = np.array([-3 + 6 * random.random() for _ in range(5)])

    # Run gradient descent
    result_vec, path = gradient_descent(
        func=f,
        start_vec=start_point,
        init_lr=0.05,
        max_iter=1000,
        tolerance=1e-6,
        log_interval=10
    )

    print("\nFinal result:")
    print(f"Minimum found at: {format_vector(result_vec)}")
    print(f"Function value: {f(result_vec):.6f}")


if __name__ == "__main__":
    main()