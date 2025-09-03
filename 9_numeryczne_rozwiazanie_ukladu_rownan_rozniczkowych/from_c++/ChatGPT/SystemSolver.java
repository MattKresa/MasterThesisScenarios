// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/SystemSolver.java) ---
package org.example;

import java.io.*;
import java.util.*;
import java.util.regex.*;
import net.objecthunter.exp4j.Expression;
import net.objecthunter.exp4j.ExpressionBuilder;

class Equation {
    String var;
    String expr;

    Equation(String var, String expr) {
        this.var = var;
        this.expr = expr;
    }
}

class SystemData {
    Map<String, Double> params = new HashMap<>();
    List<Double> y0 = new ArrayList<>();
    double t_start = 0.0;
    double t_end = 1.0;
    double dt = 0.1;
    List<Equation> equations = new ArrayList<>();
}

public class SystemSolver {

    public static SystemData readSystem(String filename) throws IOException {
        SystemData data = new SystemData();
        Pattern derivRe = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

        try (BufferedReader br = new BufferedReader(new FileReader(filename))) {
            String line;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) continue;

                // Parameters without derivatives
                if (line.contains("=") && !line.contains("'")) {
                    String[] parts = line.split("=", 2);
                    String key = parts[0].trim();
                    String val = parts[1].trim();

                    if (key.equals("y0")) {
                        Matcher m = Pattern.compile("[-+]?\\d*\\.?\\d+").matcher(val);
                        while (m.find()) {
                            data.y0.add(Double.parseDouble(m.group()));
                        }
                    } else if (key.equals("t_start")) {
                        data.t_start = Double.parseDouble(val);
                    } else if (key.equals("t_end")) {
                        data.t_end = Double.parseDouble(val);
                    } else if (key.equals("dt")) {
                        data.dt = Double.parseDouble(val);
                    } else {
                        try {
                            Expression expr = new ExpressionBuilder(val)
                                    .variables()
                                    .build();
                            double value = expr.evaluate();
                            data.params.put(key, value);
                        } catch (Exception e) {
                            System.err.println("Error parsing expression for " + key + ": " + val);
                        }
                    }
                }

                // Derivatives
                if (line.contains("'") && line.contains("=")) {
                    Matcher match = derivRe.matcher(line);
                    if (match.matches()) {
                        data.equations.add(new Equation(match.group(1), match.group(2)));
                    }
                }
            }
        }
        return data;
    }

    static class DerivFunc {
        List<Equation> equations;
        Map<String, Double> vars = new HashMap<>();
        List<Expression> compiledExprs = new ArrayList<>();

        DerivFunc(List<Equation> eqs, Map<String, Double> params) {
            equations = eqs;

            // Copy params
            vars.putAll(params);

            for (Equation eq : eqs) {
                vars.putIfAbsent(eq.var, 0.0);
            }

            vars.put("t", 0.0);

            // Compile each equation expression
            for (Equation eq : eqs) {
                Expression expr = new ExpressionBuilder(eq.expr)
                        .variables(vars.keySet())
                        .build();
                compiledExprs.add(expr);
            }
        }

        List<Double> apply(double t, List<Double> y) {
            vars.put("t", t);
            for (int i = 0; i < equations.size(); i++) {
                vars.put(equations.get(i).var, y.get(i));
            }
            List<Double> dydt = new ArrayList<>();
            for (int i = 0; i < compiledExprs.size(); i++) {
                Expression expr = compiledExprs.get(i);
                for (Map.Entry<String, Double> entry : vars.entrySet()) {
                    expr.setVariable(entry.getKey(), entry.getValue());
                }
                dydt.add(expr.evaluate());
            }
            return dydt;
        }
    }

    public static Pair<List<Double>, List<List<Double>>> rk4(
            DerivFunc f, List<Double> y0, double t0, double t_end, double h) {

        int n = (int) ((t_end - t0) / h) + 1;
        List<Double> t = new ArrayList<>(Collections.nCopies(n, 0.0));
        List<List<Double>> y = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            y.add(new ArrayList<>(Collections.nCopies(y0.size(), 0.0)));
        }

        t.set(0, t0);
        y.set(0, new ArrayList<>(y0));

        for (int i = 0; i < n - 1; i++) {
            t.set(i + 1, t.get(i) + h);

            List<Double> k1 = f.apply(t.get(i), y.get(i));

            List<Double> yk = new ArrayList<>();
            for (int j = 0; j < y0.size(); j++) {
                yk.add(y.get(i).get(j) + h * k1.get(j) / 2);
            }
            List<Double> k2 = f.apply(t.get(i) + h / 2, yk);

            yk.clear();
            for (int j = 0; j < y0.size(); j++) {
                yk.add(y.get(i).get(j) + h * k2.get(j) / 2);
            }
            List<Double> k3 = f.apply(t.get(i) + h / 2, yk);

            yk.clear();
            for (int j = 0; j < y0.size(); j++) {
                yk.add(y.get(i).get(j) + h * k3.get(j));
            }
            List<Double> k4 = f.apply(t.get(i) + h, yk);

            for (int j = 0; j < y0.size(); j++) {
                double newVal = y.get(i).get(j) + (h / 6) *
                        (k1.get(j) + 2 * k2.get(j) + 2 * k3.get(j) + k4.get(j));
                y.get(i + 1).set(j, newVal);
            }
        }

        return new Pair<>(t, y);
    }

    public static void main(String[] args) throws IOException {
        SystemData sys = readSystem("system.txt");
        DerivFunc f = new DerivFunc(sys.equations, sys.params);

        Pair<List<Double>, List<List<Double>>> result = rk4(f, sys.y0, sys.t_start, sys.t_end, sys.dt);
        List<Double> t_rk4 = result.first;
        List<List<Double>> y_rk4 = result.second;

        System.out.printf("%12s", "t");
        for (Equation eq : sys.equations) {
            System.out.printf(" | %12s", eq.var + "_RK4");
        }
        System.out.println();

        for (int i = 0; i < t_rk4.size(); i++) {
            System.out.printf("%12.6f", t_rk4.get(i));
            for (double val : y_rk4.get(i)) {
                System.out.printf(" | %12.6f", val);
            }
            System.out.println();
        }
    }

    // Simple pair utility
    static class Pair<F, S> {
        final F first;
        final S second;
        Pair(F f, S s) { first = f; second = s; }
    }
}
