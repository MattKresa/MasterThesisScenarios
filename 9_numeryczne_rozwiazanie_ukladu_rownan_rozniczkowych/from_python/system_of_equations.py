import numpy as np
import re

def read_system(filename):
    params = {}
    equations = []
    y0 = []
    t_start = 0
    t_end = 1
    dt = 0.1

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            # parameters (without derivatives)
            if "=" in line and "'" not in line:
                key, val = [s.strip() for s in line.split("=", 1)]
                try:
                    params[key] = eval(val, {"np": np})
                except:
                    params[key] = val

            # initial conditions and time
            if line.lower().startswith("y0"):
                y0 = eval(line.split("=")[1].strip(), {"np": np})
            elif line.lower().startswith("t_start"):
                t_start = float(line.split("=")[1].strip())
            elif line.lower().startswith("t_end"):
                t_end = float(line.split("=")[1].strip())
            elif line.lower().startswith("dt"):
                dt = float(line.split("=")[1].strip())

            # derivatives
            if "'" in line and "=" in line:
                match = re.match(r"^([a-zA-Z_]\w*)'\s*=\s*(.+)$", line)
                if match:
                    var = match.group(1).strip()
                    expr = match.group(2).strip()
                    equations.append((var, expr))

    return params, y0, t_start, t_end, dt, equations

def make_deriv_func(equations, params):
    def f(t, y):
        local_vars = dict(params)
        for (var, _), val in zip(equations, y):
            local_vars[var] = val
        dydt = []
        for var, expr in equations:
            dydt.append(eval(expr, {"np": np}, local_vars))
        return np.array(dydt)
    return f

def rk4(f, y0, t0, t_end, h):
    n = int((t_end - t0) / h) + 1
    t = np.linspace(t0, t_end, n)
    y = np.zeros((n, len(y0)))
    y[0] = y0
    for i in range(n - 1):
        k1 = f(t[i], y[i])
        k2 = f(t[i] + h/2, y[i] + h*k1/2)
        k3 = f(t[i] + h/2, y[i] + h*k2/2)
        k4 = f(t[i] + h, y[i] + h*k3)
        y[i+1] = y[i] + (h/6)*(k1 + 2*k2 + 2*k3 + k4)
    return t, y

# --- MAIN ---
params, y0, t_start, t_end, dt, equations = read_system("system.txt")
f = make_deriv_func(equations, params)

# RK4
t_rk4, y_rk4 = rk4(f, np.array(y0), t_start, t_end, dt)

# Show result
header = ["t"] + [f"{var}_RK4" for var, _ in equations]
print(" | ".join(f"{h:>12}" for h in header))
for i in range(len(t_rk4)):
    row = [t_rk4[i]] + list(y_rk4[i])
    print(" | ".join(f"{val:12.6f}" for val in row))
