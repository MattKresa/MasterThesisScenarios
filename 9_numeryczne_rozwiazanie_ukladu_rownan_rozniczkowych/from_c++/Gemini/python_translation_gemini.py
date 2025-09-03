import re
from typing import List, Dict, Tuple, Any

# You'll need to install and import a library for expression evaluation.
# `asteval` is a good choice for a safe and robust one.
# You can install it using: `pip install asteval`
from asteval import Interpreter


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
    """
    Reads system parameters and equations from a file.
    """
    data = SystemData()
    deriv_re = re.compile(r"^([a-zA-Z_]\w*)'\s*=\s*(.+)$")

    with open(filename, 'r') as file:
        for line in file:
            line = line.strip()

            if not line or line.startswith('#'):
                continue

            # Parameters without derivatives
            if '=' in line and "'" not in line:
                key, val = [s.strip() for s in line.split('=', 1)]

                if key == "y0":
                    # Assumes a simple comma-separated list of numbers
                    data.y0 = [float(num) for num in re.findall(r"[-+]?\d*\.?\d+", val)]
                elif key == "t_start":
                    data.t_start = float(val)
                elif key == "t_end":
                    data.t_end = float(val)
                elif key == "dt":
                    data.dt = float(val)
                else:
                    aeval = Interpreter()
                    try:
                        data.params[key] = aeval.eval(val)
                    except Exception as e:
                        print(f"Error parsing expression for {key}: {val}. Error: {e}")

            # Derivatives
            if "'" in line and '=' in line:
                match = deriv_re.match(line)
                if match:
                    eq = Equation(match.group(1), match.group(2))
                    data.equations.append(eq)

    return data


class DerivFunc:
    """
    A callable class to compute derivatives for the ODE system.
    """

    def __init__(self, equations: List[Equation], params: Dict[str, float]):
        self.aeval = Interpreter()
        self.equations = equations

        # Copy params to the asteval symbol table
        self.aeval.symtable.update(params)

        # Initialize ODE variables in the symbol table
        for eq in equations:
            if eq.var not in self.aeval.symtable:
                self.aeval.symtable[eq.var] = 0.0

        # Add time variable
        self.aeval.symtable['t'] = 0.0

    def __call__(self, t: float, y: List[float]) -> List[float]:
        """
        Calculates the derivatives at a given time and state.
        """
        # Update time and state variables in the symbol table
        self.aeval.symtable['t'] = t
        for i, eq in enumerate(self.equations):
            self.aeval.symtable[eq.var] = y[i]

        # Compute derivatives
        dydt = []
        for eq in self.equations:
            dydt.append(self.aeval.eval(eq.expr))

        return dydt


def rk4(f: DerivFunc, y0: List[float], t0: float, t_end: float, h: float) \
        -> Tuple[List[float], List[List[float]]]:
    """
    Solves an ODE system using the 4th order Runge-Kutta method.
    """
    n_steps = int(round((t_end - t0) / h)) + 1
    t = [t0 + i * h for i in range(n_steps)]
    y = [y0[:]]

    current_y = y0[:]
    for i in range(n_steps - 1):
        k1 = f(t[i], current_y)

        y_k2 = [current_y[j] + h * k1[j] / 2 for j in range(len(y0))]
        k2 = f(t[i] + h / 2, y_k2)

        y_k3 = [current_y[j] + h * k2[j] / 2 for j in range(len(y0))]
        k3 = f(t[i] + h / 2, y_k3)

        y_k4 = [current_y[j] + h * k3[j] for j in range(len(y0))]
        k4 = f(t[i] + h, y_k4)

        next_y = [
            current_y[j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j])
            for j in range(len(y0))
        ]

        current_y = next_y
        y.append(current_y[:])

    return t, y


def main():
    """
    Main function to run the simulation.
    """
    try:
        sys_data = read_system("system.txt")
        f = DerivFunc(sys_data.equations, sys_data.params)

        t_rk4, y_rk4 = rk4(f, sys_data.y0, sys_data.t_start, sys_data.t_end, sys_data.dt)

        # Print header
        header = f"{'t':>12}"
        for eq in sys_data.equations:
            header += f" | {eq.var + '_RK4':>12}"
        print(header)

        # Print data
        for i in range(len(t_rk4)):
            row = f"{t_rk4[i]:>12.6f}"
            for val in y_rk4[i]:
                row += f" | {val:>12.6f}"
            print(row)

    except FileNotFoundError:
        print("Error: The file 'system.txt' was not found.")


if __name__ == "__main__":
    main()