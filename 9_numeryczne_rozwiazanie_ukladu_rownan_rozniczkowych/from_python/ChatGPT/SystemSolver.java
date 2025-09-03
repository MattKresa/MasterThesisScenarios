// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/SystemSolver.java) ---
package org.example;

import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;
import net.objecthunter.exp4j.Expression;
import net.objecthunter.exp4j.ExpressionBuilder;

public class SystemSolver {

    static class Equation {
        String var;
        String expr;
        Equation(String var, String expr) {
            this.var = var;
            this.expr = expr;
        }
    }

    // Read system from file
    public static Object[] readSystem(String filename) throws IOException {
        Map<String, Double> params = new HashMap<>();
        List<Equation> equations = new ArrayList<>();
        List<Double> y0 = new ArrayList<>();
        double tStart = 0;
        double tEnd = 1;
        double dt = 0.1;

        Pattern derivPattern = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

        List<String> lines = Files.readAllLines(Paths.get(filename));
        for (String rawLine : lines) {
            String line = rawLine.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;

            // Parameters without derivatives
            if (line.contains("=") && !line.contains("'")) {
                String[] parts = line.split("=", 2);
                String key = parts[0].trim();
                String val = parts[1].trim();
                try {
                    params.put(key, Double.parseDouble(val));
                } catch (NumberFormatException e) {
                    params.put(key, 0.0); // variable placeholder
                }
            }

            // Initial conditions and time
            String lower = line.toLowerCase();
            if (lower.startsWith("y0")) {
                String arr = line.split("=")[1].trim();
                arr = arr.replaceAll("[\\[\\]]", "");
                for (String s : arr.split(",")) {
                    y0.add(Double.parseDouble(s.trim()));
                }
            } else if (lower.startsWith("t_start")) {
                tStart = Double.parseDouble(line.split("=")[1].trim());
            } else if (lower.startsWith("t_end")) {
                tEnd = Double.parseDouble(line.split("=")[1].trim());
            } else if (lower.startsWith("dt")) {
                dt = Double.parseDouble(line.split("=")[1].trim());
            }

            // Derivatives
            if (line.contains("'") && line.contains("=")) {
                Matcher m = derivPattern.matcher(line);
                if (m.matches()) {
                    equations.add(new Equation(m.group(1).trim(), m.group(2).trim()));
                }
            }
        }

        return new Object[]{params, y0, tStart, tEnd, dt, equations};
    }

    // Create derivative function
    public static DerivFunc makeDerivFunc(List<Equation> equations, Map<String, Double> params) {
        return (t, y) -> {
            Map<String, Double> localVars = new HashMap<>(params);
            for (int i = 0; i < equations.size(); i++) {
                localVars.put(equations.get(i).var, y[i]);
            }
            localVars.put("t", t);

            double[] dydt = new double[equations.size()];
            for (int i = 0; i < equations.size(); i++) {
                Equation eq = equations.get(i);
                Expression e = new ExpressionBuilder(eq.expr)
                        .variables(localVars.keySet())
                        .build()
                        .setVariables(localVars);
                dydt[i] = e.evaluate();
            }
            return dydt;
        };
    }

    // RK4 solver
    public static Object[] rk4(DerivFunc f, double[] y0, double t0, double tEnd, double h) {
        int n = (int) ((tEnd - t0) / h) + 1;
        double[] t = new double[n];
        double[][] y = new double[n][y0.length];
        t[0] = t0;
        System.arraycopy(y0, 0, y[0], 0, y0.length);

        for (int i = 0; i < n - 1; i++) {
            t[i + 1] = t[i] + h;
            double[] k1 = f.eval(t[i], y[i]);
            double[] yk = new double[y0.length];

            for (int j = 0; j < y0.length; j++)
                yk[j] = y[i][j] + h * k1[j] / 2;
            double[] k2 = f.eval(t[i] + h / 2, yk);

            for (int j = 0; j < y0.length; j++)
                yk[j] = y[i][j] + h * k2[j] / 2;
            double[] k3 = f.eval(t[i] + h / 2, yk);

            for (int j = 0; j < y0.length; j++)
                yk[j] = y[i][j] + h * k3[j];
            double[] k4 = f.eval(t[i] + h, yk);

            for (int j = 0; j < y0.length; j++) {
                y[i + 1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
            }
        }
        return new Object[]{t, y};
    }

    // Functional interface for derivatives
    @FunctionalInterface
    interface DerivFunc {
        double[] eval(double t, double[] y);
    }

    // --- MAIN ---
    public static void main(String[] args) {
        try {
            Object[] res = readSystem("system.txt");
            Map<String, Double> params = (Map<String, Double>) res[0];
            List<Double> y0List = (List<Double>) res[1];
            double tStart = (double) res[2];
            double tEnd = (double) res[3];
            double dt = (double) res[4];
            List<Equation> equations = (List<Equation>) res[5];

            double[] y0 = y0List.stream().mapToDouble(Double::doubleValue).toArray();
            DerivFunc f = makeDerivFunc(equations, params);

            Object[] rk = rk4(f, y0, tStart, tEnd, dt);
            double[] t = (double[]) rk[0];
            double[][] y = (double[][]) rk[1];

            // Output
            System.out.printf("%12s", "t");
            for (Equation eq : equations) {
                System.out.printf(" | %12s", eq.var + "_RK4");
            }
            System.out.println();
            for (int i = 0; i < t.length; i++) {
                System.out.printf("%12.6f", t[i]);
                for (double val : y[i]) {
                    System.out.printf(" | %12.6f", val);
                }
                System.out.println();
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
