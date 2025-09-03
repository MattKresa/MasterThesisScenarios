import math
import random

# ===== Objective function =====
def f(vec):
    sum_sq = sum(x * x for x in vec)
    sum_sin = sum(math.sin(3.0 * x) for x in vec)
    return sum_sq + 0.5 * sum_sin

# ===== Numerical gradient (central difference) =====
def numerical_gradient(func, vec, h=1e-5):
    grad = [0.0] * len(vec)
    for i in range(len(vec)):
        vec_forward = vec.copy()
        vec_backward = vec.copy()
        vec_forward[i] += h
        vec_backward[i] -= h
        grad[i] = (func(vec_forward) - func(vec_backward)) / (2.0 * h)
    return grad

# ===== Gradient descent with adaptive learning rate =====
def gradient_descent(func, start_vec, init_lr=0.1, max_iter=1000, tolerance=1e-6, log_interval=10):
    vec = start_vec.copy()
    lr = init_lr
    history = [vec.copy()]

    for iteration in range(max_iter):
        grad = numerical_gradient(func, vec)
        new_vec = [vec[i] - lr * grad[i] for i in range(len(vec))]

        # If step increases function value -> decrease learning rate
        if func(new_vec) > func(vec):
            lr *= 0.5
            continue

        # If step improves function -> slightly increase learning rate
        lr *= 1.05
        history.append(new_vec.copy())

        # Log progress
        if iteration % log_interval == 0:
            print(f"Iter {iteration}: f(x) = {func(vec):.6f}, lr = {lr:.5f}, x = [{', '.join(f'{x:.5f}' for x in vec)}]")

        # Stop criteria
        move_norm = math.sqrt(sum((new_vec[i] - vec[i]) ** 2 for i in range(len(vec))))
        grad_norm = math.sqrt(sum(g ** 2 for g in grad))

        if move_norm < tolerance or grad_norm < tolerance:
            print(f"Converged at iteration {iteration}")
            break

        vec = new_vec

    return vec, history

# ===== Main =====
if __name__ == "__main__":
    random.seed(0)

    # Start point in 5D
    start_point = [random.uniform(-3.0, 3.0) for _ in range(5)]

    minimum_vec, path = gradient_descent(f, start_point, init_lr=0.05)

    print("\nFinal result:")
    print(f"Minimum found at: [{', '.join(f'{x:.6f}' for x in minimum_vec)}]")
    print(f"Function value: {f(minimum_vec):.6f}")
