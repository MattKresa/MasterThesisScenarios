import re
import math
from typing import List, Dict, Tuple, Optional


class Equation:
    def __init__(self, var: str, expr: str):
        self.var = var
        self.expr = expr


class SystemData:
    def __init__(self):
        self.params: Dict[str, float] = {}
        self.y0: List[float] = []
        self.t_start: float = 0.0
        self.t_end: float = 1.0
        self.dt: float = 0.1
        self.equations: List[Equation] = []


def eval_expression(expr: str, vars: Dict[str, float]) -> float:
    # Create a dictionary with all variables and math functions
    context = {**vars, **{name: getattr(math, name) for name in dir(math) if callable(getattr(math, name))}}

    # Replace numpy-style functions with math ones
    expr = expr.replace('np.', 'math.')

    # Evaluate the expression safely
    try:
        return float(eval(expr, {"__builtins__": None}, context))
    except Exception as e:
        raise ValueError(f"Error evaluating expression '{expr}': {str(e)}")


def read_system(filename: str) -> SystemData:
    data = SystemData()
    deriv_pattern = re.compile(r"^([a-zA-Z_]\w*)'\s*=\s*(.+)$")

    with open(filename, 'r') as file:
        for raw_line in file:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            # Parameters without derivatives
            if "=" in line and "'" not in line:
                parts = line.split("=", 1)
                key = parts[0].strip()
                val = parts[1].strip()

                if key.lower() == "y0":
                    arr_str = val.replace("[", "").replace("]", "")
                    data.y0 = [float(s.strip()) for s in arr_str.split(",")]
                elif key.lower() == "t_start":
                    data.t_start = float(val)
                elif key.lower() == "t_end":
                    data.t_end = float(val)
                elif key.lower() == "dt":
                    data.dt = float(val)
                else:
                    try:
                        result = eval_expression(val, {})
                        data.params[key] = result
                    except:
                        data.params[key] = val

            # Derivatives
            if "'" in line and "=" in line:
                m = deriv_pattern.match(line)
                if m:
                    data.equations.append(Equation(m.group(1).strip(), m.group(2).strip()))

    return data


def deriv(t: float, y: List[float], eqs: List[Equation], params: Dict[str, float]) -> List[float]:
    vars = {**params}
    for i in range(len(eqs)):
        vars[eqs[i].var] = y[i]
    vars['t'] = t

    dydt = [0.0] * len(eqs)
    for i in range(len(eqs)):
        dydt[i] = eval_expression(eqs[i].expr, vars)
    return dydt


def rk4(eqs: List[Equation], params: Dict[str, float], y0: List[float], t0: float, t_end: float, h: float):
    n = int((t_end - t0) / h) + 1
    t = [0.0] * n
    y = [[0.0 for _ in range(len(y0))] for _ in range(n)]
    t[0] = t0
    y[0] = y0.copy()

    for i in range(n - 1):
        k1 = deriv(t[i], y[i], eqs, params)

        yk2 = [y[i][j] + h * k1[j] / 2 for j in range(len(y0))]
        k2 = deriv(t[i] + h / 2, yk2, eqs, params)

        yk3 = [y[i][j] + h * k2[j] / 2 for j in range(len(y0))]
        k3 = deriv(t[i] + h / 2, yk3, eqs, params)

        yk4 = [y[i][j] + h * k3[j] for j in range(len(y0))]
        k4 = deriv(t[i] + h, yk4, eqs, params)

        y[i + 1] = [y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j])
                    for j in range(len(y0))]
        t[i + 1] = t[i] + h

    # Print results
    print(f"{'t':>12}", end="")
    for eq in eqs:
        print(f" | {eq.var + '_RK4':>12}", end="")
    print()

    for i in range(n):
        print(f"{t[i]:12.6f}", end="")
        for val in y[i]:
            print(f" | {val:12.6f}", end="")
        print()


def main():
    sys = read_system("system.txt")
    y0_arr = sys.y0.copy()
    rk4(sys.equations, sys.params, y0_arr, sys.t_start, sys.t_end, sys.dt)


if __name__ == "__main__":
    main()