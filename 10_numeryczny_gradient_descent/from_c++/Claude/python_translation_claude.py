import numpy as np
import math


def f(vec):
    """Objective function to minimize."""
    sum_sq = np.sum(vec ** 2)
    sum_sin = np.sum(np.sin(3.0 * vec))
    return sum_sq + 0.5 * sum_sin


def numerical_gradient(func, vec, h=1e-5):
    """Calculate numerical gradient using central difference."""
    grad = np.zeros_like(vec)
    for i in range(len(vec)):
        vec_forward = vec.copy()
        vec_backward = vec.copy()
        vec_forward[i] += h
        vec_backward[i] -= h
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2.0 * h)
    return grad


def gradient_descent(func, start_vec, init_lr=0.1, max_iter=1000,
                     tolerance=1e-6, log_interval=10):
    """
    Gradient descent with adaptive learning rate.

    Returns:
        tuple: (final_vector, optimization_history)
    """
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

        # Log progress every log_interval iterations
        if iteration % log_interval == 0:
            print(f"Iter {iteration}: f(x) = {func(vec):.6f}, "
                  f"lr = {lr:.5f}, x = {vec}")

        # Stop criteria
        move_norm = np.linalg.norm(new_vec - vec)
        grad_norm = np.linalg.norm(grad)

        if move_norm < tolerance or grad_norm < tolerance:
            print(f"Converged at iteration {iteration}")
            break

        vec = new_vec

    return vec, history


def main():
    # Set random seed (equivalent to the C++ mt19937(0))
    np.random.seed(0)

    # Start point in 5D (random values between -3 and 3)
    start_point = np.random.uniform(-3.0, 3.0, size=5)

    minimum_vec, path = gradient_descent(f, start_point, init_lr=0.05)

    print("\nFinal result:")
    print(f"Minimum found at: {minimum_vec}")
    print(f"Function value: {f(minimum_vec):.6f}")


if __name__ == "__main__":
    main()