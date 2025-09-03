import re
from typing import List, Dict, Tuple


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


def read_system(filename: str) -> SystemData:
    data = SystemData()
    deriv_re = re.compile(r"^([a-zA-Z_]\w*)'\s*=\s*(.+)$")

    with open(filename, 'r') as file:
        for line in file:
            line = line.strip()
            if not line or line.startswith('#'):
                continue

            # Parameters without derivatives
            if '=' in line and "'" not in line:
                key, val = [x.strip() for x in line.split("=", 1)]

                if key == "y0":
                    nums = re.findall(r"[-+]?\d*\.?\d+", val)
                    data.y0 = [float(num) for num in nums]
                elif key == "t_start":
                    data.t_start = float(val)
                elif key == "t_end":
                    data.t_end = float(val)
                elif key == "dt":
                    data.dt = float(val)
                else:
                    # Evaluate constants safely
                    try:
                        data.params[key] = eval(val, {"__builtins__": None}, {})
                    except Exception:
                        raise ValueError(f"Error parsing expression for {key}: {val}")

            # Derivatives
            elif "'" in line and "=" in line:
                match = deriv_re.match(line)
                if match:
                    var, expr = match.groups()
                    data.equations.append(Equation(var, expr))

    return data


class DerivFunc:
    def __init__(self, eqs: List[Equation], params: Dict[str, float]):
        self.equations = eqs
        self.vars: Dict[str, float] = dict(params)
        for eq in eqs:
            self.vars.setdefault(eq.var, 0.0)
        self.vars["t"] = 0.0

    def __call__(self, t: float, y: List[float]) -> List[float]:
        self.vars["t"] = t
        for i, eq in enumerate(self.equations):
            self.vars[eq.var] = y[i]
        dydt = []
        for eq in self.equations:
            try:
                dydt.append(eval(eq.expr, {"__builtins__": None}, self.vars))
            except Exception:
                raise ValueError(f"Error evaluating expression: {eq.expr}")
        return dydt


def rk4(f: DerivFunc, y0: List[float], t0: float, t_end: float, h: float) -> Tuple[List[float], List[List[float]]]:
    n = int((t_end - t0) / h) + 1
    t = [0.0] * n
    y = [[0.0] * len(y0) for _ in range(n)]

    t[0] = t0
    y[0] = y0[:]

    for i in range(n - 1):
        t[i + 1] = t[i] + h
        k1 = f(t[i], y[i])
        yk = [y[i][j] + h * k1[j] / 2 for j in range(len(y0))]
        k2 = f(t[i] + h / 2, yk)
        yk = [y[i][j] + h * k2[j] / 2 for j in range(len(y0))]
        k3 = f(t[i] + h / 2, yk)
        yk = [y[i][j] + h * k3[j] for j in range(len(y0))]
        k4 = f(t[i] + h, yk)

        for j in range(len(y0)):
            y[i + 1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j])

    return t, y


if __name__ == "__main__":
    sys_data = read_system("system.txt")
    f = DerivFunc(sys_data.equations, sys_data.params)

    t_rk4, y_rk4 = rk4(f, sys_data.y0, sys_data.t_start, sys_data.t_end, sys_data.dt)

    header = ["t"] + [eq.var + "_RK4" for eq in sys_data.equations]
    print(" | ".join(f"{h:>12}" for h in header))

    for ti, yi in zip(t_rk4, y_rk4):
        print(f"{ti:12.6f} | " + " | ".join(f"{val:12.6f}" for val in yi))
