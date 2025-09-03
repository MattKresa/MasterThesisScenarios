import re
import numpy as np
from typing import List, Dict, Any, Tuple


class Equation:
    def __init__(self, var: str, expr: str):
        self.var = var
        self.expr = expr


class SystemData:
    def __init__(self):
        self.params: Dict[str, Any] = {}
        self.y0: List[float] = []
        self.t_start: float = 0
        self.t_end: float = 1
        self.dt: float = 0.1
        self.equations: List[Equation] = []


def eval_expression(expr: str, variables: Dict[str, Any]) -> float:
    """
    Safely evaluate mathematical expressions with given variables.
    """
    # Create a safe namespace with math functions
    safe_dict = {
        '__builtins__': {},
        'np': np,
        'sin': np.sin,
        'cos': np.cos,
        'tan': np.tan,
        'exp': np.exp,
        'log': np.log,
        'sqrt': np.sqrt,
        'pi': np.pi,
        'e': np.e,
        'abs': abs,
        'pow': pow,
        'min': min,
        'max': max,
    }

    # Add variables to the namespace
    safe_dict.update(variables)

    try:
        return eval(expr, safe_dict)
    except Exception as e:
        raise ValueError(f"Error evaluating expression '{expr}': {e}")


def eval_python_like(expr: str, variables: Dict[str, Any]) -> float:
    """
    Evaluate expressions that might use numpy-like syntax.
    """
    # Replace "np." with direct numpy function calls if needed
    # The expression is already using np. syntax, so we can evaluate directly
    return eval_expression(expr, variables)


def read_system(filename: str) -> SystemData:
    """
    Read system parameters and differential equations from a file.
    """
    data = SystemData()
    deriv_pattern = re.compile(r"^([a-zA-Z_]\w*)\'\s*=\s*(.+)$")

    try:
        with open(filename, 'r', encoding='utf-8') as file:
            lines = file.readlines()
    except FileNotFoundError:
        raise FileNotFoundError(f"System file '{filename}' not found")

    for raw_line in lines:
        line = raw_line.strip()
        if not line or line.startswith('#'):
            continue

        # Parameters without derivatives
        if '=' in line and "'" not in line:
            parts = line.split('=', 1)
            if len(parts) != 2:
                continue

            key = parts[0].strip()
            val = parts[1].strip()

            if key.lower() == 'y0':
                # Parse array: [1, 2, 3] or 1, 2, 3
                arr_str = val.replace('[', '').replace(']', '')
                data.y0 = [float(s.strip()) for s in arr_str.split(',') if s.strip()]
            elif key.lower() == 't_start':
                data.t_start = float(val)
            elif key.lower() == 't_end':
                data.t_end = float(val)
            elif key.lower() == 'dt':
                data.dt = float(val)
            else:
                try:
                    result = eval_python_like(val, {})
                    data.params[key] = result
                except Exception:
                    data.params[key] = val

        # Derivatives
        if "'" in line and '=' in line:
            match = deriv_pattern.match(line)
            if match:
                var_name = match.group(1).strip()
                expression = match.group(2).strip()
                data.equations.append(Equation(var_name, expression))

    return data


def deriv(t: float, y: np.ndarray, equations: List[Equation], params: Dict[str, Any]) -> np.ndarray:
    """
    Calculate derivatives at time t for state vector y.
    """
    variables = params.copy()

    # Add current state variables
    for i, eq in enumerate(equations):
        variables[eq.var] = y[i]
    variables['t'] = t

    dydt = np.zeros(len(equations))
    for i, eq in enumerate(equations):
        dydt[i] = eval_python_like(eq.expr, variables)

    return dydt


def rk4(equations: List[Equation], params: Dict[str, Any], y0: np.ndarray,
        t0: float, t_end: float, h: float) -> None:
    """
    Solve system of ODEs using 4th order Runge-Kutta method.
    """
    n = int((t_end - t0) / h) + 1
    t = np.zeros(n)
    y = np.zeros((n, len(y0)))

    t[0] = t0
    y[0] = y0.copy()

    for i in range(n - 1):
        # Calculate k1, k2, k3, k4
        k1 = deriv(t[i], y[i], equations, params)
        k2 = deriv(t[i] + h / 2, y[i] + h * k1 / 2, equations, params)
        k3 = deriv(t[i] + h / 2, y[i] + h * k2 / 2, equations, params)
        k4 = deriv(t[i] + h, y[i] + h * k3, equations, params)

        # Update y and t
        y[i + 1] = y[i] + (h / 6) * (k1 + 2 * k2 + 2 * k3 + k4)
        t[i + 1] = t[i] + h

    # Print results
    print(f"{'t':>12}", end="")
    for eq in equations:
        print(f" | {eq.var + '_RK4':>12}", end="")
    print()

    for i in range(n):
        print(f"{t[i]:12.6f}", end="")
        for j in range(len(y0)):
            print(f" | {y[i][j]:12.6f}", end="")
        print()


def main():
    """
    Main function to read system and solve ODEs.
    """
    try:
        system_data = read_system("system.txt")

        if not system_data.equations:
            print("No differential equations found in system.txt")
            return

        if not system_data.y0:
            print("No initial conditions (y0) found in system.txt")
            return

        y0_array = np.array(system_data.y0)

        if len(y0_array) != len(system_data.equations):
            print(f"Mismatch: {len(system_data.equations)} equations but {len(y0_array)} initial conditions")
            return

        rk4(system_data.equations, system_data.params, y0_array,
            system_data.t_start, system_data.t_end, system_data.dt)

    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()