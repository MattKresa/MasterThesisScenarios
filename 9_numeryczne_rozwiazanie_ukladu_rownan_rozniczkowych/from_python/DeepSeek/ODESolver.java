// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ODESolver.java) ---
package org.example;

import java.io.*;
import java.util.*;
import java.util.regex.*;
import net.objecthunter.exp4j.Expression;
import net.objecthunter.exp4j.ExpressionBuilder;

public class ODESolver {
    static class SystemData {
        Map<String, Double> params = new HashMap<>();
        List<Double> y0 = new ArrayList<>();
        double t_start = 0;
        double t_end = 1;
        double dt = 0.1;
        List<Map.Entry<String, String>> equations = new ArrayList<>();
    }

    public static SystemData readSystem(String filename) throws IOException {
        SystemData data = new SystemData();
        Pattern equationPattern = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

        try (BufferedReader br = new BufferedReader(new FileReader(filename))) {
            String line;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) {
                    continue;
                }

                // Parameters (without derivatives)
                if (line.contains("=") && !line.contains("'")) {
                    String[] parts = line.split("=", 2);
                    String key = parts[0].trim();
                    String val = parts[1].trim();
                    try {
                        Expression e = new ExpressionBuilder(val).build();
                        data.params.put(key, e.evaluate());
                    } catch (Exception e) {
                        // If not a valid expression, try parsing as double
                        try {
                            data.params.put(key, Double.parseDouble(val));
                        } catch (NumberFormatException ex) {
                            data.params.put(key, 0.0); // Default value
                        }
                    }
                }

                // Initial conditions and time
                if (line.toLowerCase().startsWith("y0")) {
                    String valStr = line.split("=")[1].trim();
                    valStr = valStr.substring(1, valStr.length() - 1); // Remove brackets
                    String[] vals = valStr.split(",");
                    for (String v : vals) {
                        data.y0.add(Double.parseDouble(v.trim()));
                    }
                } else if (line.toLowerCase().startsWith("t_start")) {
                    data.t_start = Double.parseDouble(line.split("=")[1].trim());
                } else if (line.toLowerCase().startsWith("t_end")) {
                    data.t_end = Double.parseDouble(line.split("=")[1].trim());
                } else if (line.toLowerCase().startsWith("dt")) {
                    data.dt = Double.parseDouble(line.split("=")[1].trim());
                }

                // Derivatives
                if (line.contains("'") && line.contains("=")) {
                    Matcher m = equationPattern.matcher(line);
                    if (m.matches()) {
                        String var = m.group(1).trim();
                        String expr = m.group(2).trim();
                        data.equations.add(new AbstractMap.SimpleEntry<>(var, expr));
                    }
                }
            }
        }
        return data;
    }

    public interface DerivativeFunction {
        double[] evaluate(double t, double[] y);
    }

    public static DerivativeFunction makeDerivFunc(List<Map.Entry<String, String>> equations,
                                                   Map<String, Double> params) {
        return (t, y) -> {
            Map<String, Double> variables = new HashMap<>(params);
            variables.put("t", t);

            // Add state variables
            for (int i = 0; i < equations.size(); i++) {
                variables.put(equations.get(i).getKey(), y[i]);
            }

            double[] dydt = new double[equations.size()];
            for (int i = 0; i < equations.size(); i++) {
                String expr = equations.get(i).getValue();
                try {
                    Expression e = new ExpressionBuilder(expr)
                            .variables(variables.keySet())
                            .build()
                            .setVariables(variables);
                    dydt[i] = e.evaluate();
                } catch (Exception e) {
                    System.err.println("Error evaluating equation: " + e.getMessage());
                    dydt[i] = 0;
                }
            }
            return dydt;
        };
    }

    public static class RK4Result {
        public double[] t;
        public double[][] y;

        public RK4Result(double[] t, double[][] y) {
            this.t = t;
            this.y = y;
        }
    }

    public static RK4Result rk4(DerivativeFunction f, double[] y0, double t0, double t_end, double h) {
        int n = (int) ((t_end - t0) / h) + 1;
        double[] t = new double[n];
        double[][] y = new double[n][y0.length];

        // Initialize
        t[0] = t0;
        System.arraycopy(y0, 0, y[0], 0, y0.length);

        for (int i = 0; i < n - 1; i++) {
            double[] k1 = f.evaluate(t[i], y[i]);

            double[] y2 = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                y2[j] = y[i][j] + h * k1[j] / 2;
            }
            double[] k2 = f.evaluate(t[i] + h/2, y2);

            double[] y3 = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                y3[j] = y[i][j] + h * k2[j] / 2;
            }
            double[] k3 = f.evaluate(t[i] + h/2, y3);

            double[] y4 = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                y4[j] = y[i][j] + h * k3[j];
            }
            double[] k4 = f.evaluate(t[i] + h, y4);

            for (int j = 0; j < y0.length; j++) {
                y[i+1][j] = y[i][j] + (h/6) * (k1[j] + 2*k2[j] + 2*k3[j] + k4[j]);
            }
            t[i+1] = t[i] + h;
        }

        return new RK4Result(t, y);
    }

    public static void main(String[] args) {
        try {
            SystemData data = readSystem("system.txt");

            // Convert y0 list to array
            double[] y0 = new double[data.y0.size()];
            for (int i = 0; i < y0.length; i++) {
                y0[i] = data.y0.get(i);
            }

            DerivativeFunction f = makeDerivFunc(data.equations, data.params);
            RK4Result result = rk4(f, y0, data.t_start, data.t_end, data.dt);

            // Print header
            System.out.printf("%12s", "t");
            for (Map.Entry<String, String> eq : data.equations) {
                System.out.printf(" | %12s", eq.getKey() + "_RK4");
            }
            System.out.println();

            // Print results
            for (int i = 0; i < result.t.length; i++) {
                System.out.printf("%12.6f", result.t[i]);
                for (int j = 0; j < result.y[i].length; j++) {
                    System.out.printf(" | %12.6f", result.y[i][j]);
                }
                System.out.println();
            }
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}