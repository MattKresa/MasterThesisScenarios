import math
import random
from typing import List, Callable, Tuple


def f(vec: List[float]) -> float:
    """Function: Sphere + sine perturbation"""
    sum_squares = 0.0
    sum_sin = 0.0
    for v in vec:
        sum_squares += v * v
        sum_sin += math.sin(3 * v)
    return sum_squares + 0.5 * sum_sin


def numerical_gradient(func: Callable[[List[float]], float], vec: List[float], h: float) -> List[float]:
    """Numerical gradient (central difference)"""
    grad = [0.0] * len(vec)
    for i in range(len(vec)):
        vec_forward = vec.copy()
        vec_backward = vec.copy()
        vec_forward[i] += h
        vec_backward[i] -= h
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2 * h)
    return grad


def norm(vec: List[float]) -> float:
    """Euclidean norm (L2)"""
    return math.sqrt(sum(v * v for v in vec))


def gradient_descent(func: Callable[[List[float]], float],
                     start_vec: List[float],
                     init_lr: float,
                     max_iter: int,
                     tolerance: float,
                     log_interval: int) -> Tuple[List[float], List[List[float]]]:
    """
    Gradient Descent with adaptive learning rate

    Returns:
        Tuple containing:
        - minimum vector found
        - history of all visited vectors
    """
    vec = start_vec.copy()
    lr = init_lr
    history = [vec.copy()]

    for iteration in range(max_iter):
        grad = numerical_gradient(func, vec, 1e-5)
        new_vec = [vec[i] - lr * grad[i] for i in range(len(vec))]

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
        if norm(diff(new_vec, vec)) < tolerance or norm(grad) < tolerance:
            print(f"Converged at iteration {iteration}")
            vec = new_vec
            break

        vec = new_vec

    return vec, history


def diff(a: List[float], b: List[float]) -> List[float]:
    """Helper: difference of two vectors"""
    return [a[i] - b[i] for i in range(len(a))]


def format_vector(vec: List[float]) -> str:
    """Helper: format vector like NumPy with 6 decimal places"""
    elements = [f"{x: .6f}" for x in vec]  # space for alignment like NumPy
    return "[" + ", ".join(elements) + "]"


def main():
    random.seed(0)
    start_point = [-3 + 6 * random.random() for _ in range(5)]

    minimum_vec, path = gradient_descent(f, start_point, 0.05, 1000, 1e-6, 10)

    print("\nFinal result:")
    print(f"Minimum found at: {format_vector(minimum_vec)}")
    print(f"Function value: {f(minimum_vec):.6f}")


if __name__ == "__main__":
    main()