import re
import math

class Equation:
    def __init__(self, var, expr):
        self.var = var
        self.expr = expr

def eval_python_like(expr, vars_dict):
    """
    Evaluate the expression with given variables.
    Replaces 'np.' with 'math.' so we can use Python's math functions.
    """
    expr = re.sub(r"\bnp\.", "math.", expr)
    return eval(expr, {"math": math}, vars_dict)

class SystemData:
    def __init__(self):
        self.params = {}
        self.y0 = []
        self.t_start = 0
        self.t_end = 1
        self.dt = 0.1
        self.equations = []

def read_system(filename):
    data = SystemData()
    deriv_pattern = re.compile(r"^([a-zA-Z_]\w*)'\s*=\s*(.+)$")

    with open(filename, "r") as f:
        for raw_line in f:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            # Parameters without derivatives
            if "=" in line and "'" not in line:
                key, val = map(str.strip, line.split("=", 1))
                if key.lower() == "y0":
                    arr_str = re.sub(r"[\[\]]", "", val)
                    for s in arr_str.split(","):
                        data.y0.append(float(s.strip()))
                elif key.lower() == "t_start":
                    data.t_start = float(val)
                elif key.lower() == "t_end":
                    data.t_end = float(val)
                elif key.lower() == "dt":
                    data.dt = float(val)
                else:
                    try:
                        res = eval_python_like(val, {})
                        data.params[key] = res
                    except Exception:
                        data.params[key] = val

            # Derivatives
            if "'" in line and "=" in line:
                m = deriv_pattern.match(line)
                if m:
                    data.equations.append(Equation(m.group(1).strip(), m.group(2).strip()))

    return data

def deriv(t, y, eqs, params):
    vars_dict = dict(params)
    for i, eq in enumerate(eqs):
        vars_dict[eq.var] = y[i]
    vars_dict["t"] = t

    dydt = []
    for eq in eqs:
        dydt.append(float(eval_python_like(eq.expr, vars_dict)))
    return dydt

def rk4(eqs, params, y0, t0, t_end, h):
    n = int((t_end - t0) / h) + 1
    t = [0.0] * n
    y = [[0.0] * len(y0) for _ in range(n)]

    t[0] = t0
    y[0] = y0[:]

    for i in range(n - 1):
        k1 = deriv(t[i], y[i], eqs, params)
        yk2 = [y[i][j] + h * k1[j] / 2 for j in range(len(y0))]
        k2 = deriv(t[i] + h / 2, yk2, eqs, params)

        yk3 = [y[i][j] + h * k2[j] / 2 for j in range(len(y0))]
        k3 = deriv(t[i] + h / 2, yk3, eqs, params)

        yk4 = [y[i][j] + h * k3[j] for j in range(len(y0))]
        k4 = deriv(t[i] + h, yk4, eqs, params)

        for j in range(len(y0)):
            y[i + 1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j])
        t[i + 1] = t[i] + h

    # Print results
    print(f"{'t':>12}", end="")
    for eq in eqs:
        print(f" | {eq.var + '_RK4':>12}", end="")
    print()
    for i in range(n):
        print(f"{t[i]:12.6f}", end="")
        for j in range(len(y0)):
            print(f" | {y[i][j]:12.6f}", end="")
        print()

if __name__ == "__main__":
    sys_data = read_system("system.txt")
    rk4(sys_data.equations, sys_data.params, sys_data.y0, sys_data.t_start, sys_data.t_end, sys_data.dt)
