import re
from typing import List, Dict, Tuple, NamedTuple
import numpy as np
from sympy import symbols, sympify


class Equation(NamedTuple):
    var: str
    expr: str


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
            # Trim whitespace
            line = line.strip()

            if not line or line.startswith('#'):
                continue

            # Parameters without derivatives
            if '=' in line and "'" not in line:
                key, val = [s.strip() for s in line.split('=', 1)]

                if key == 'y0':
                    # Remove brackets and split by commas
                    val = val.strip('[]')
                    data.y0 = [float(x.strip()) for x in val.split(',') if x.strip()]
                elif key == 't_start':
                    data.t_start = float(val)
                elif key == 't_end':
                    data.t_end = float(val)
                elif key == 'dt':
                    data.dt = float(val)
                else:
                    # Use sympy to evaluate the expression
                    try:
                        data.params[key] = float(sympify(val).evalf())
                    except:
                        print(f"Error parsing expression for {key}: {val}")

            # Derivatives
            if "'" in line and '=' in line:
                match = deriv_re.match(line)
                if match:
                    eq = Equation(var=match.group(1), expr=match.group(2))
                    data.equations.append(eq)

    return data


class DerivFunc:
    def __init__(self, equations: List[Equation], params: Dict[str, float]):
        self.equations = equations
        self.vars = params.copy()

        # Initialize all equation variables to 0.0 if not already in params
        for eq in equations:
            if eq.var not in self.vars:
                self.vars[eq.var] = 0.0

        self.vars['t'] = 0.0

        self.symbols = {name: symbols(name) for name in self.vars.keys()}
        self.compiled_exprs = []

        all_vars = set(self.vars.keys())
        for eq in equations:
            # Extract variable names from expression using regex
            expr_vars = re.findall(r'\b[a-zA-Z_][a-zA-Z0-9_]*\b', eq.expr)
            all_vars.update(expr_vars)

        self.symbols = {name: symbols(name) for name in all_vars}

        for eq in equations:
            try:
                local_syms = {name: sym for name, sym in self.symbols.items()
                              if name in eq.expr}

                expr = sympify(eq.expr, locals=local_syms)
                self.compiled_exprs.append(expr)
            except:
                print(f"Error compiling expression for {eq.var}: {eq.expr}")
                self.compiled_exprs.append(0)

    def __call__(self, t: float, y: List[float]) -> List[float]:
        # Update time and y values
        self.vars['t'] = t
        for i, eq in enumerate(self.equations):
            self.vars[eq.var] = y[i]

        # Compute derivatives
        dydt = []

        for expr in self.compiled_exprs:
            try:
                subs_dict = {self.symbols[name]: value for name, value in self.vars.items() if name in self.symbols}
                dydt.append(float(expr.evalf(subs=subs_dict)))
            except Exception as e:
                print(f"Error evaluating expression: {str(e)}")
                dydt.append(0.0)

        return dydt


def rk4(f: DerivFunc, y0: List[float], t0: float, t_end: float, h: float
        ) -> Tuple[np.ndarray, np.ndarray]:
    n = int((t_end - t0) / h) + 1
    t = np.zeros(n)
    y = np.zeros((n, len(y0)))

    t[0] = t0
    y[0] = y0

    for i in range(n - 1):
        t[i + 1] = t[i] + h
        k1 = f(t[i], y[i])
        k2 = f(t[i] + h / 2, y[i] + h / 2 * np.array(k1))
        k3 = f(t[i] + h / 2, y[i] + h / 2 * np.array(k2))
        k4 = f(t[i] + h, y[i] + h * np.array(k3))

        y[i + 1] = y[i] + (h / 6) * (np.array(k1) + 2 * np.array(k2) + 2 * np.array(k3) + np.array(k4))

    return t, y


def main():
    sys = read_system("system.txt")
    f = DerivFunc(sys.equations, sys.params)

    t_rk4, y_rk4 = rk4(f, sys.y0, sys.t_start, sys.t_end, sys.dt)

    # Print header
    print(f"{'t':>12}", end="")
    for eq in sys.equations:
        print(f" | {eq.var + '_RK4':>12}", end="")
    print()

    # Print results
    for i in range(len(t_rk4)):
        print(f"{t_rk4[i]:12.6f}", end="")
        for val in y_rk4[i]:
            print(f" | {val:12.6f}", end="")
        print()


if __name__ == "__main__":
    main()