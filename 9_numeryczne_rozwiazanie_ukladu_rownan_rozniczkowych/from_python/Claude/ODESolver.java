// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ODESolver.java) ---
package org.example;

import java.io.*;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.function.BiFunction;
import net.objecthunter.exp4j.Expression;
import net.objecthunter.exp4j.ExpressionBuilder;

class SystemData {
    public Map<String, Double> params;
    public List<Double> y0;
    public double tStart;
    public double tEnd;
    public double dt;
    public List<Equation> equations;

    public SystemData() {
        this.params = new HashMap<>();
        this.y0 = new ArrayList<>();
        this.tStart = 0.0;
        this.tEnd = 1.0;
        this.dt = 0.1;
        this.equations = new ArrayList<>();
    }
}

class Equation {
    public String variable;
    public String expression;

    public Equation(String variable, String expression) {
        this.variable = variable;
        this.expression = expression;
    }

    @Override
    public String toString() {
        return variable + "' = " + expression;
    }
}

class RK4Result {
    public double[] t;
    public double[][] y;

    public RK4Result(double[] t, double[][] y) {
        this.t = t;
        this.y = y;
    }
}

public class ODESolver {

    // Expression evaluator using exp4j
    private static double evaluateExpression(String expr, Map<String, Double> variables) {
        try {
            ExpressionBuilder builder = new ExpressionBuilder(expr);

            // Add all variables to the expression builder
            for (String varName : variables.keySet()) {
                builder.variable(varName);
            }

            Expression expression = builder.build();

            // Set variable values
            for (Map.Entry<String, Double> entry : variables.entrySet()) {
                expression.setVariable(entry.getKey(), entry.getValue());
            }

            return expression.evaluate();
        } catch (Exception e) {
            System.err.println("Error evaluating expression: " + expr);
            System.err.println("Variables: " + variables);
            e.printStackTrace();
            return 0.0;
        }
    }

    private static String trim(String str) {
        return str.trim();
    }

    private static String[] split(String str, String delimiter) {
        return str.split(delimiter);
    }

    // Parse array format like [1.0, 2.0, 3.0] or simple comma-separated values
    private static List<Double> parseArray(String str) {
        List<Double> result = new ArrayList<>();

        // Remove brackets if present
        String cleaned = str.replaceAll("[\\[\\]]", "");

        String[] tokens = cleaned.split(",");
        for (String token : tokens) {
            token = trim(token);
            if (!token.isEmpty()) {
                try {
                    result.add(Double.parseDouble(token));
                } catch (NumberFormatException e) {
                    System.err.println("Warning: Could not parse array element: " + token);
                }
            }
        }

        return result;
    }

    public static SystemData readSystem(String filename) {
        SystemData data = new SystemData();

        try (BufferedReader reader = new BufferedReader(new FileReader(filename))) {
            String line;

            while ((line = reader.readLine()) != null) {
                line = trim(line);
                if (line.isEmpty() || line.startsWith("#")) {
                    continue;
                }

                // Parameters (without derivatives)
                if (line.contains("=") && !line.contains("'")) {
                    String[] tokens = line.split("=", 2);
                    if (tokens.length >= 2) {
                        String key = trim(tokens[0]);
                        String val = trim(tokens[1]);

                        // Convert key to lowercase for comparison
                        String keyLower = key.toLowerCase();

                        // Handle special parameters
                        if (keyLower.equals("y0")) {
                            data.y0 = parseArray(val);
                        } else if (keyLower.equals("t_start")) {
                            data.tStart = Double.parseDouble(val);
                        } else if (keyLower.equals("t_end")) {
                            data.tEnd = Double.parseDouble(val);
                        } else if (keyLower.equals("dt")) {
                            data.dt = Double.parseDouble(val);
                        } else {
                            // Try to parse as a mathematical expression
                            try {
                                Map<String, Double> emptyVars = new HashMap<>();
                                data.params.put(key, evaluateExpression(val, emptyVars));
                            } catch (Exception e) {
                                System.err.println("Warning: Could not parse parameter: " + key + " = " + val);
                            }
                        }
                    }
                }

                // Derivatives
                if (line.contains("'") && line.contains("=")) {
                    Pattern derivPattern = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");
                    Matcher matcher = derivPattern.matcher(line);
                    if (matcher.matches()) {
                        String var = trim(matcher.group(1));
                        String expr = trim(matcher.group(2));
                        data.equations.add(new Equation(var, expr));
                    }
                }
            }
        } catch (IOException e) {
            System.err.println("Error reading file: " + filename);
            e.printStackTrace();
        }

        return data;
    }

    public static BiFunction<Double, double[], double[]> makeDerivFunc(
            List<Equation> equations, Map<String, Double> params) {

        return (t, y) -> {
            Map<String, Double> localVars = new HashMap<>(params);
            localVars.put("t", t);

            // Set current variable values
            for (int i = 0; i < equations.size() && i < y.length; i++) {
                localVars.put(equations.get(i).variable, y[i]);
            }

            double[] dydt = new double[equations.size()];
            for (int i = 0; i < equations.size(); i++) {
                dydt[i] = evaluateExpression(equations.get(i).expression, localVars);
            }

            return dydt;
        };
    }

    public static RK4Result rk4(BiFunction<Double, double[], double[]> f,
                                double[] y0, double t0, double tEnd, double h) {

        int n = (int) ((tEnd - t0) / h) + 1;
        double[] t = new double[n];
        double[][] y = new double[n][y0.length];

        // Initialize time vector
        for (int i = 0; i < n; i++) {
            t[i] = t0 + i * h;
        }

        // Copy initial conditions
        System.arraycopy(y0, 0, y[0], 0, y0.length);

        for (int i = 0; i < n - 1; i++) {
            double[] k1 = f.apply(t[i], y[i]);

            double[] yTemp1 = new double[y[i].length];
            for (int j = 0; j < y[i].length; j++) {
                yTemp1[j] = y[i][j] + h * k1[j] / 2.0;
            }
            double[] k2 = f.apply(t[i] + h/2.0, yTemp1);

            double[] yTemp2 = new double[y[i].length];
            for (int j = 0; j < y[i].length; j++) {
                yTemp2[j] = y[i][j] + h * k2[j] / 2.0;
            }
            double[] k3 = f.apply(t[i] + h/2.0, yTemp2);

            double[] yTemp3 = new double[y[i].length];
            for (int j = 0; j < y[i].length; j++) {
                yTemp3[j] = y[i][j] + h * k3[j];
            }
            double[] k4 = f.apply(t[i] + h, yTemp3);

            for (int j = 0; j < y[i].length; j++) {
                y[i+1][j] = y[i][j] + (h/6.0) * (k1[j] + 2*k2[j] + 2*k3[j] + k4[j]);
            }
        }

        return new RK4Result(t, y);
    }

    public static void main(String[] args) {
        // Read system from file
        SystemData data = readSystem("system.txt");

        if (data.equations.isEmpty()) {
            System.err.println("Error: No equations found in system file.");
            return;
        }

        if (data.y0.isEmpty()) {
            System.err.println("Error: No initial conditions (y0) found.");
            return;
        }

        System.out.println("System loaded successfully:");
        System.out.print("Parameters: ");
        for (Map.Entry<String, Double> entry : data.params.entrySet()) {
            System.out.print(entry.getKey() + "=" + entry.getValue() + " ");
        }
        System.out.println();

        System.out.print("Initial conditions: ");
        for (Double val : data.y0) {
            System.out.print(val + " ");
        }
        System.out.println();

        System.out.printf("Time range: [%.3f, %.3f], dt=%.3f%n",
                data.tStart, data.tEnd, data.dt);

        System.out.println("Equations:");
        for (Equation eq : data.equations) {
            System.out.println("  " + eq);
        }
        System.out.println();

        // Create derivative function
        BiFunction<Double, double[], double[]> f = makeDerivFunc(data.equations, data.params);

        // Convert y0 to array
        double[] y0Array = data.y0.stream().mapToDouble(Double::doubleValue).toArray();

        // Solve using RK4
        RK4Result result = rk4(f, y0Array, data.tStart, data.tEnd, data.dt);

        // Print results
        System.out.printf("%12s", "t");
        for (Equation eq : data.equations) {
            System.out.printf(" | %12s", eq.variable + "_RK4");
        }
        System.out.println();

        // Print separator line
        System.out.print("------------");
        for (int i = 0; i < data.equations.size(); i++) {
            System.out.print("-+------------");
        }
        System.out.println();

        for (int i = 0; i < result.t.length; i++) {
            System.out.printf("%12.6f", result.t[i]);
            for (int j = 0; j < result.y[i].length && j < data.equations.size(); j++) {
                System.out.printf(" | %12.6f", result.y[i][j]);
            }
            System.out.println();
        }
    }
}