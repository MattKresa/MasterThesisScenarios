import re
import numpy as np
from dataclasses import dataclass
from typing import List, Dict, Tuple, Callable


@dataclass
class Equation:
    var: str
    expr: str


@dataclass
class SystemData:
    params: Dict[str, float]
    y0: List[float]
    t_start: float = 0.0
    t_end: float = 1.0
    dt: float = 0.1
    equations: List[Equation] = None

    def __post_init__(self):
        if self.equations is None:
            self.equations = []


def read_system(filename: str) -> SystemData:
    """Read system of differential equations from file."""
    data = SystemData(params={}, y0=[])
    deriv_pattern = re.compile(r'^([a-zA-Z_]\w*)\'?\s*=\s*(.+)$')

    try:
        with open(filename, 'r') as file:
            for line in file:
                # Trim whitespace
                line = line.strip()

                # Skip empty lines and comments
                if not line or line.startswith('#'):
                    continue

                # Parameters without derivatives
                if '=' in line and "'" not in line:
                    key, val = line.split('=', 1)
                    key = key.strip()
                    val = val.strip()

                    if key == "y0":
                        # Parse array-like string: [1, 2, 3] or 1,2,3
                        val = val.strip('[]')
                        data.y0 = [float(x.strip()) for x in val.split(',') if x.strip()]
                    elif key == "t_start":
                        data.t_start = float(val)
                    elif key == "t_end":
                        data.t_end = float(val)
                    elif key == "dt":
                        data.dt = float(val)
                    else:
                        # Evaluate mathematical expressions
                        try:
                            # Replace common mathematical functions for eval
                            safe_val = val.replace('^', '**')  # Handle exponentiation
                            # Add numpy functions to evaluation context
                            eval_context = {
                                'sin': np.sin, 'cos': np.cos, 'tan': np.tan,
                                'exp': np.exp, 'log': np.log, 'sqrt': np.sqrt,
                                'pi': np.pi, 'e': np.e,
                                'abs': abs, 'pow': pow
                            }
                            data.params[key] = eval(safe_val, {"__builtins__": {}}, eval_context)
                        except:
                            # If evaluation fails, try to convert directly to float
                            data.params[key] = float(val)

                # Derivatives (equations with prime notation)
                elif "'" in line and '=' in line:
                    match = deriv_pattern.match(line)
                    if match:
                        var = match.group(1)
                        expr = match.group(2)
                        data.equations.append(Equation(var=var, expr=expr))

    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
        raise
    except Exception as e:
        print(f"Error reading system file: {e}")
        raise

    return data


class DerivFunc:
    """Function object to compute derivatives for the ODE system."""

    def __init__(self, equations: List[Equation], params: Dict[str, float]):
        self.equations = equations
        self.vars = params.copy()

        # Initialize variables for each equation
        for eq in equations:
            if eq.var not in self.vars:
                self.vars[eq.var] = 0.0

        self.vars['t'] = 0.0

        # Prepare evaluation context with mathematical functions
        self.eval_context = {
            'sin': np.sin, 'cos': np.cos, 'tan': np.tan,
            'exp': np.exp, 'log': np.log, 'sqrt': np.sqrt,
            'pi': np.pi, 'e': np.e,
            'abs': abs, 'pow': pow,
            '__builtins__': {}
        }

        # Compile expressions (prepare them for evaluation)
        self.compiled_exprs = []
        for eq in equations:
            # Replace common mathematical notation
            expr = eq.expr.replace('^', '**')
            self.compiled_exprs.append(expr)

    def __call__(self, t: float, y: List[float]) -> List[float]:
        """Evaluate the system of ODEs at time t with state y."""
        # Update time and state variables
        self.vars['t'] = t
        for i, eq in enumerate(self.equations):
            self.vars[eq.var] = y[i]

        # Update evaluation context with current variable values
        eval_context = {**self.eval_context, **self.vars}

        # Compute derivatives
        dydt = []
        for expr in self.compiled_exprs:
            try:
                dydt.append(eval(expr, eval_context))
            except Exception as e:
                print(f"Error evaluating expression '{expr}': {e}")
                dydt.append(0.0)

        return dydt


def rk4(f: Callable, y0: List[float], t0: float, t_end: float, h: float) -> Tuple[np.ndarray, np.ndarray]:
    """
    Solve ODE using 4th order Runge-Kutta method.

    Args:
        f: Function that computes derivatives
        y0: Initial conditions
        t0: Start time
        t_end: End time
        h: Step size

    Returns:
        Tuple of (time_array, solution_array)
    """
    n = int((t_end - t0) / h) + 1
    t = np.linspace(t0, t_end, n)
    y = np.zeros((n, len(y0)))

    y[0] = y0

    for i in range(n - 1):
        # RK4 algorithm
        k1 = np.array(f(t[i], y[i]))
        k2 = np.array(f(t[i] + h / 2, y[i] + h * k1 / 2))
        k3 = np.array(f(t[i] + h / 2, y[i] + h * k2 / 2))
        k4 = np.array(f(t[i] + h, y[i] + h * k3))

        y[i + 1] = y[i] + (h / 6) * (k1 + 2 * k2 + 2 * k3 + k4)

    return t, y


def main():
    """Main function to solve the ODE system."""
    try:
        # Read system from file
        sys = read_system("system.txt")

        # Create derivative function
        f = DerivFunc(sys.equations, sys.params)

        # Solve using RK4
        t_rk4, y_rk4 = rk4(f, sys.y0, sys.t_start, sys.t_end, sys.dt)

        # Print results
        print(f"{'t':>12}", end="")
        for eq in sys.equations:
            print(f" | {eq.var + '_RK4':>12}", end="")
        print()

        for i in range(len(t_rk4)):
            print(f"{t_rk4[i]:>12.6f}", end="")
            for j in range(len(y_rk4[i])):
                print(f" | {y_rk4[i][j]:>12.6f}", end="")
            print()

    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()