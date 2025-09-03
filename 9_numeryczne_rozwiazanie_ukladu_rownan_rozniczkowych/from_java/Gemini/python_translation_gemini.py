import re
import math


class RK4Solver:
    """
    A Python implementation of the RK4Solver from the provided Java code.
    This class reads a system of ordinary differential equations (ODEs) from a file
    and solves them using the Runge-Kutta 4th order (RK4) method.
    """

    class Equation:
        """Represents a single differential equation."""

        def __init__(self, var, expr):
            self.var = var
            self.expr = expr

    class SystemData:
        """A data structure to hold the system's parameters and equations."""

        def __init__(self):
            self.params = {}
            self.y0 = []
            self.t_start = 0.0
            self.t_end = 1.0
            self.dt = 0.1
            self.equations = []

    @staticmethod
    def eval_expression(expr, vars_map):
        """
        Evaluates a given string expression using a dictionary of variables.
        Note: This is a simplified version and may not handle all edge cases.
        """
        local_vars = {key: value for key, value in vars_map.items()}
        # Replace 'np.' with 'math.' for NumPy-like math functions
        expr = expr.replace("np.", "math.")
        return eval(expr, {"__builtins__": None, "math": math}, local_vars)

    @staticmethod
    def read_system(filename):
        """
        Reads system parameters and equations from a file.

        Args:
            filename (str): The path to the file containing system data.

        Returns:
            RK4Solver.SystemData: An object containing the parsed system data.
        """
        data = RK4Solver.SystemData()
        deriv_pattern = re.compile(r"^([a-zA-Z_]\w*)'\s*=\s*(.+)$")

        with open(filename, 'r') as f:
            for raw_line in f:
                line = raw_line.strip()
                if not line or line.startswith('#'):
                    continue

                if '=' in line and "'" not in line:
                    key, val = line.split('=', 1)
                    key = key.strip().lower()
                    val = val.strip()

                    if key == 'y0':
                        data.y0 = [float(s.strip()) for s in val.strip('[]').split(',')]
                    elif key == 't_start':
                        data.t_start = float(val)
                    elif key == 't_end':
                        data.t_end = float(val)
                    elif key == 'dt':
                        data.dt = float(val)
                    else:
                        try:
                            # Evaluate parameter value
                            data.params[key] = RK4Solver.eval_expression(val, {})
                        except (NameError, SyntaxError):
                            # Store as string if evaluation fails
                            data.params[key] = val

                if "'" in line and '=' in line:
                    match = deriv_pattern.match(line)
                    if match:
                        var = match.group(1).strip()
                        expr = match.group(2).strip()
                        data.equations.append(RK4Solver.Equation(var, expr))
        return data

    @staticmethod
    def deriv(t, y, eqs, params):
        """
        Calculates the derivatives for the given system at a specific time and state.

        Args:
            t (float): Current time.
            y (list): List of current state variables.
            eqs (list): List of Equation objects.
            params (dict): Dictionary of system parameters.

        Returns:
            list: A list of derivative values (dydt).
        """
        vars_map = {**params, 't': t}
        for i, eq in enumerate(eqs):
            vars_map[eq.var] = y[i]

        dydt = [RK4Solver.eval_expression(eq.expr, vars_map) for eq in eqs]
        return dydt

    @staticmethod
    def rk4(eqs, params, y0, t0, t_end, h):
        """
        Solves the system of ODEs using the RK4 method and prints the results.

        Args:
            eqs (list): List of Equation objects.
            params (dict): Dictionary of system parameters.
            y0 (list): Initial state variables.
            t0 (float): Start time.
            t_end (float): End time.
            h (float): Time step size.
        """
        num_steps = int(round((t_end - t0) / h)) + 1
        t = [0.0] * num_steps
        y = [[0.0] * len(y0) for _ in range(num_steps)]

        t[0] = t0
        y[0] = y0[:]

        for i in range(num_steps - 1):
            k1 = RK4Solver.deriv(t[i], y[i], eqs, params)

            y_k2_in = [y[i][j] + h * k1[j] / 2 for j in range(len(y0))]
            k2 = RK4Solver.deriv(t[i] + h / 2, y_k2_in, eqs, params)

            y_k3_in = [y[i][j] + h * k2[j] / 2 for j in range(len(y0))]
            k3 = RK4Solver.deriv(t[i] + h / 2, y_k3_in, eqs, params)

            y_k4_in = [y[i][j] + h * k3[j] for j in range(len(y0))]
            k4 = RK4Solver.deriv(t[i] + h, y_k4_in, eqs, params)

            for j in range(len(y0)):
                y[i + 1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j])

            t[i + 1] = t[i] + h

        # Printing results
        header = f"{'t':>12s}"
        for eq in eqs:
            header += f" | {eq.var + '_RK4':>12s}"
        print(header)

        for i in range(num_steps):
            row = f"{t[i]:>12.6f}"
            for val in y[i]:
                row += f" | {val:>12.6f}"
            print(row)


if __name__ == '__main__':
    try:
        system_data = RK4Solver.read_system("system.txt")
        RK4Solver.rk4(
            system_data.equations,
            system_data.params,
            system_data.y0,
            system_data.t_start,
            system_data.t_end,
            system_data.dt
        )
    except FileNotFoundError:
        print("Error: The file 'system.txt' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")