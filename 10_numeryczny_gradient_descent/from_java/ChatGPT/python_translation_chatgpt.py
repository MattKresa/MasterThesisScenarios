import math
import random

# Function: Sphere + sine perturbation
def f(vec):
    sum_squares = sum(v * v for v in vec)
    sum_sin = sum(math.sin(3 * v) for v in vec)
    return sum_squares + 0.5 * sum_sin


# Numerical gradient (central difference)
def numerical_gradient(func, vec, h):
    grad = []
    for i in range(len(vec)):
        vec_forward = vec.copy()
        vec_backward = vec.copy()
        vec_forward[i] += h
        vec_backward[i] -= h
        grad_val = (func(vec_forward) - func(vec_backward)) / (2 * h)
        grad.append(grad_val)
    return grad


# Euclidean norm (L2)
def norm(vec):
    return math.sqrt(sum(v * v for v in vec))


# Helper: difference of two vectors
def diff(a, b):
    return [a[i] - b[i] for i in range(len(a))]


# Helper: format vector like NumPy with 6 decimal places
def format_vector(vec):
    return "[" + ", ".join(f"{v: .6f}" for v in vec) + "]"


# Gradient Descent with adaptive learning rate
def gradient_descent(func, start_vec, init_lr, max_iter, tolerance, log_interval):
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

    return {"minimum_vec": vec, "path": history}


# ====== MAIN ======
if __name__ == "__main__":
    random.seed(0)
    start_point = [-3 + 6 * random.random() for _ in range(5)]

    result = gradient_descent(f, start_point, 0.05, 1000, 1e-6, 10)

    print("\nFinal result:")
    print("Minimum found at:", format_vector(result["minimum_vec"]))
    print(f"Function value: {f(result['minimum_vec']):.6f}")
