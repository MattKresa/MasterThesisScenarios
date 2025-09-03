// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/RK4Solver.java) ---
package org.example;

import javax.script.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

public class RK4Solver {

    static class Equation {
        String var;
        String expr;
        Equation(String var, String expr) {
            this.var = var;
            this.expr = expr;
        }
    }

    public static Object evalExpression(String expr, Map<String, Object> vars, ScriptEngine engine) throws ScriptException {
        Bindings bindings = engine.createBindings();
        bindings.putAll(vars);
        return engine.eval(expr, bindings);
    }

    public static Object evalPythonLike(String expr, Map<String, Object> vars, ScriptEngine engine) throws ScriptException {
        // Replace "np" with "Math" (for mathematical functions)
        expr = expr.replaceAll("\\bnp\\.", "Math.");
        return evalExpression(expr, vars, engine);
    }

    public static class SystemData {
        Map<String, Object> params = new HashMap<>();
        List<Double> y0 = new ArrayList<>();
        double tStart = 0;
        double tEnd = 1;
        double dt = 0.1;
        List<Equation> equations = new ArrayList<>();
    }

    public static SystemData readSystem(String filename, ScriptEngine engine) throws IOException, ScriptException {
        SystemData data = new SystemData();
        Pattern derivPattern = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

        for (String rawLine : Files.readAllLines(Paths.get(filename))) {
            String line = rawLine.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;

            // Parameters without derivatives
            if (line.contains("=") && !line.contains("'")) {
                String[] parts = line.split("=", 2);
                String key = parts[0].trim();
                String val = parts[1].trim();
                if (key.equalsIgnoreCase("y0")) {
                    String arrStr = val.replaceAll("[\\[\\]]", "");
                    for (String s : arrStr.split(",")) {
                        data.y0.add(Double.parseDouble(s.trim()));
                    }
                } else if (key.equalsIgnoreCase("t_start")) {
                    data.tStart = Double.parseDouble(val);
                } else if (key.equalsIgnoreCase("t_end")) {
                    data.tEnd = Double.parseDouble(val);
                } else if (key.equalsIgnoreCase("dt")) {
                    data.dt = Double.parseDouble(val);
                } else {
                    try {
                        Object res = evalPythonLike(val, Collections.emptyMap(), engine);
                        data.params.put(key, res);
                    } catch (Exception e) {
                        data.params.put(key, val);
                    }
                }
            }

            // Derivatives
            if (line.contains("'") && line.contains("=")) {
                Matcher m = derivPattern.matcher(line);
                if (m.find()) {
                    data.equations.add(new Equation(m.group(1).trim(), m.group(2).trim()));
                }
            }
        }
        return data;
    }

    public static double[] deriv(double t, double[] y, List<Equation> eqs, Map<String, Object> params, ScriptEngine engine) throws ScriptException {
        Map<String, Object> vars = new HashMap<>(params);
        for (int i = 0; i < eqs.size(); i++) {
            vars.put(eqs.get(i).var, y[i]);
        }
        vars.put("t", t);

        double[] dydt = new double[eqs.size()];
        for (int i = 0; i < eqs.size(); i++) {
            dydt[i] = ((Number) evalPythonLike(eqs.get(i).expr, vars, engine)).doubleValue();
        }
        return dydt;
    }

    public static void rk4(List<Equation> eqs, Map<String, Object> params, double[] y0, double t0, double tEnd, double h, ScriptEngine engine) throws ScriptException {
        int n = (int) ((tEnd - t0) / h) + 1;
        double[] t = new double[n];
        double[][] y = new double[n][y0.length];
        t[0] = t0;
        System.arraycopy(y0, 0, y[0], 0, y0.length);

        for (int i = 0; i < n - 1; i++) {
            double[] k1 = deriv(t[i], y[i], eqs, params, engine);
            double[] yk2 = new double[y0.length];
            double[] yk3 = new double[y0.length];
            double[] yk4 = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                yk2[j] = y[i][j] + h * k1[j] / 2;
            }
            double[] k2 = deriv(t[i] + h / 2, yk2, eqs, params, engine);
            for (int j = 0; j < y0.length; j++) {
                yk3[j] = y[i][j] + h * k2[j] / 2;
            }
            double[] k3 = deriv(t[i] + h / 2, yk3, eqs, params, engine);
            for (int j = 0; j < y0.length; j++) {
                yk4[j] = y[i][j] + h * k3[j];
            }
            double[] k4 = deriv(t[i] + h, yk4, eqs, params, engine);

            for (int j = 0; j < y0.length; j++) {
                y[i + 1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
            }
            t[i + 1] = t[i] + h;
        }

        // Printing results
        System.out.printf("%12s", "t");
        for (Equation eq : eqs) {
            System.out.printf(" | %12s", eq.var + "_RK4");
        }
        System.out.println();
        for (int i = 0; i < n; i++) {
            System.out.printf("%12.6f", t[i]);
            for (int j = 0; j < y0.length; j++) {
                System.out.printf(" | %12.6f", y[i][j]);
            }
            System.out.println();
        }
    }

    public static void main(String[] args) throws Exception {
        ScriptEngine engine = new ScriptEngineManager().getEngineByName("mvel");

        SystemData sys = readSystem("system.txt", engine);

        double[] y0Arr = sys.y0.stream().mapToDouble(Double::doubleValue).toArray();
        rk4(sys.equations, sys.params, y0Arr, sys.tStart, sys.tEnd, sys.dt, engine);
    }
}
