import math
import random


# ===== Objective function =====
def f(vec):
    """
    Calculates the objective function value.
    f(x) = sum(x_i^2) + 0.5 * sum(sin(3 * x_i))
    """
    sum_sq = sum(x ** 2 for x in vec)
    sum_sin = sum(math.sin(3.0 * x) for x in vec)
    return sum_sq + 0.5 * sum_sin


# ===== Numerical gradient (central difference) =====
def numerical_gradient(func, vec, h=1e-5):
    """
    Calculates the numerical gradient of a function using the central difference method.
    """
    grad = [0.0] * len(vec)
    for i in range(len(vec)):
        vec_forward = list(vec)
        vec_backward = list(vec)
        vec_forward[i] += h
        vec_backward[i] -= h
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2.0 * h)
    return grad


# ===== Gradient descent with adaptive learning rate =====
def gradient_descent(func, start_vec, init_lr=0.1, max_iter=1000, tolerance=1e-6, log_interval=10):
    """
    Performs gradient descent with an adaptive learning rate.
    """
    vec = list(start_vec)
    lr = init_lr
    history = [list(vec)]

    for iteration in range(max_iter):
        grad = numerical_gradient(func, vec)

        # Calculate the new vector
        new_vec = [v - lr * g for v, g in zip(vec, grad)]

        # Adaptive learning rate logic
        if func(tuple(new_vec)) > func(tuple(vec)):
            lr *= 0.5
            continue

        lr *= 1.05
        vec = new_vec
        history.append(list(vec))

        # Log progress
        if iteration % log_interval == 0:
            print(
                f"Iter {iteration}: f(x) = {func(vec):.6f}, lr = {lr:.5f}, x = [{', '.join(f'{x:.5f}' for x in vec)}]")

        # Stop criteria
        move_norm = math.sqrt(sum((n - v) ** 2 for n, v in zip(new_vec, history[-2])))
        grad_norm = math.sqrt(sum(g ** 2 for g in grad))

        if move_norm < tolerance or grad_norm < tolerance:
            print(f"Converged at iteration {iteration}")
            break

    return vec, history


def main():
    # Set random seed (equivalent to np.random.seed(0))
    random.seed(0)

    # Generate a random start point in 5D
    start_point = [random.uniform(-3.0, 3.0) for _ in range(5)]

    print("Initial point:", start_point)

    minimum_vec, path = gradient_descent(f, start_point, 0.05)

    print("\nFinal result:")
    print(f"Minimum found at: [{', '.join(f'{x:.6f}' for x in minimum_vec)}]")
    print(f"Function value: {f(minimum_vec):.6f}")


if __name__ == "__main__":
    main()