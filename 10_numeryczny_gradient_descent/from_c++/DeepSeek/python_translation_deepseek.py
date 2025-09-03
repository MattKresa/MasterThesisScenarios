import math
import random
from typing import Callable, List, Tuple

# ===== Objective function =====
def f(vec: List[float]) -> float:
    sum_sq = 0.0
    sum_sin = 0.0
    for x in vec:
        sum_sq += x * x
        sum_sin += math.sin(3.0 * x)
    return sum_sq + 0.5 * sum_sin

# ===== Numerical gradient (central difference) =====
def numerical_gradient(
    func: Callable[[List[float]], float],
    vec: List[float],
    h: float = 1e-5
) -> List[float]:
    grad = [0.0] * len(vec)
    for i in range(len(vec)):
        vec_forward = vec.copy()
        vec_backward = vec.copy()
        vec_forward[i] += h
        vec_backward[i] -= h
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2.0 * h)
    return grad

# ===== Gradient descent with adaptive learning rate =====
def gradient_descent(
    func: Callable[[List[float]], float],
    start_vec: List[float],
    init_lr: float = 0.1,
    max_iter: int = 1000,
    tolerance: float = 1e-6,
    log_interval: int = 10
) -> Tuple[List[float], List[List[float]]]:
    vec = start_vec.copy()
    lr = init_lr
    history = [vec.copy()]

    for iteration in range(max_iter):
        grad = numerical_gradient(func, vec)
        new_vec = [0.0] * len(vec)
        for i in range(len(vec)):
            new_vec[i] = vec[i] - lr * grad[i]

        # If the step increases the function value -> decrease learning rate
        if func(new_vec) > func(vec):
            lr *= 0.5
            continue

        # If the step improves the function -> slightly increase learning rate
        lr *= 1.05
        history.append(new_vec.copy())

        # Log progress every `log_interval` iterations
        if iteration % log_interval == 0:
            print(f"Iter {iteration}: f(x) = {func(vec):.6f}, lr = {lr:.5f}, x = {vec}")

        # Stop criteria
        move_norm = math.sqrt(sum((new_vec[i] - vec[i]) ** 2 for i in range(len(vec))))
        grad_norm = math.sqrt(sum(grad[i] ** 2 for i in range(len(vec))))

        if move_norm < tolerance or grad_norm < tolerance:
            print(f"Converged at iteration {iteration}")
            break

        vec = new_vec.copy()

    return vec, history

def main():
    # Set random number generator (equivalent to np.random.seed(0))
    random.seed(0)

    # Start point in 5D
    start_point = [random.uniform(-3.0, 3.0) for _ in range(5)]

    minimum_vec, path = gradient_descent(f, start_point, 0.05)

    print("\nFinal result:")
    print("Minimum found at: [" + ", ".join(f"{x:.6f}" for x in minimum_vec) + "]")
    print(f"Function value: {f(minimum_vec):.6f}")

if __name__ == "__main__":
    main()