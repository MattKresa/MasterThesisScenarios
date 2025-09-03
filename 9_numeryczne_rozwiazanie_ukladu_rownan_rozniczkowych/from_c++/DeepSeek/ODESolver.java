// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ODESolver.java) ---
package org.example;

import java.io.*;
import java.util.*;
import java.util.regex.*;
import net.objecthunter.exp4j.*;

class Equation {
    String var;
    String expr;

    public Equation(String var, String expr) {
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

public class ODESolver {

    public static SystemData readSystem(String filename) throws IOException {
        SystemData data = new SystemData();
        Pattern derivRe = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

        try (BufferedReader file = new BufferedReader(new FileReader(filename))) {
            String line;
            while ((line = file.readLine()) != null) {
                // Trim whitespace
                line = line.trim();

                if (line.isEmpty() || line.startsWith("#")) continue;

                // Parameters without derivatives
                if (line.contains("=") && !line.contains("'")) {
                    String[] parts = line.split("=", 2);
                    String key = parts[0].trim();
                    String val = parts[1].trim();

                    if (key.equals("y0")) {
                        // Remove brackets and split by commas
                        val = val.replaceAll("[\\[\\]]", "");
                        String[] nums = val.split(",");
                        for (String num : nums) {
                            try {
                                data.y0.add(Double.parseDouble(num.trim()));
                            } catch (NumberFormatException e) {
                                System.err.println("Error parsing number in y0: " + num);
                            }
                        }
                    }
                    else if (key.equals("t_start")) {
                        data.t_start = Double.parseDouble(val);
                    }
                    else if (key.equals("t_end")) {
                        data.t_end = Double.parseDouble(val);
                    }
                    else if (key.equals("dt")) {
                        data.dt = Double.parseDouble(val);
                    }
                    else {
                        try {
                            Expression expression = new ExpressionBuilder(val)
                                    .build();
                            data.params.put(key, expression.evaluate());
                        } catch (Exception e) {
                            System.err.println("Error parsing expression for " + key + ": " + val);
                            System.err.println(e.getMessage());
                        }
                    }
                }

                // Derivatives
                if (line.contains("'") && line.contains("=")) {
                    Matcher match = derivRe.matcher(line);
                    if (match.matches()) {
                        Equation eq = new Equation(match.group(1), match.group(2));
                        data.equations.add(eq);
                    }
                }
            }
        }
        return data;
    }

    static class DerivFunc {
        List<Equation> equations;
        Map<String, Double> vars = new HashMap<>();
        Map<String, Expression> compiledExprs = new HashMap<>();

        public DerivFunc(List<Equation> eqs, Map<String, Double> params) {
            this.equations = eqs;
            vars.putAll(params);

            // Initialize all equation variables to 0.0 if not already in params
            for (Equation eq : equations) {
                if (!vars.containsKey(eq.var)) {
                    vars.put(eq.var, 0.0);
                }
            }

            vars.put("t", 0.0);

            // Prepare expression for each equation
            for (Equation eq : equations) {
                try {
                    // Collect all variables used in this expression
                    Set<String> usedVars = new HashSet<>();
                    Pattern varPattern = Pattern.compile("[a-zA-Z_]\\w*");
                    Matcher m = varPattern.matcher(eq.expr);
                    while (m.find()) {
                        usedVars.add(m.group());
                    }

                    // Build expression with only the variables it actually uses
                    ExpressionBuilder builder = new ExpressionBuilder(eq.expr);
                    for (String var : usedVars) {
                        if (vars.containsKey(var)) {
                            builder.variable(var);
                        }
                    }

                    Expression expression = builder.build();
                    compiledExprs.put(eq.var, expression);
                } catch (Exception e) {
                    System.err.println("Error compiling expression for " + eq.var + ": " + eq.expr);
                    System.err.println(e.getMessage());
                }
            }
        }

        public double[] compute(double t, double[] y) {
            // Update time and y values
            vars.put("t", t);
            for (int i = 0; i < equations.size(); i++) {
                vars.put(equations.get(i).var, y[i]);
            }

            // Compute derivatives
            double[] dydt = new double[equations.size()];
            for (int i = 0; i < equations.size(); i++) {
                Equation eq = equations.get(i);
                Expression expr = compiledExprs.get(eq.var);
                if (expr != null) {
                    try {
                        // Set all variables
                        for (String var : expr.getVariableNames()) {
                            expr.setVariable(var, vars.get(var));
                        }
                        dydt[i] = expr.evaluate();
                    } catch (Exception e) {
                        System.err.println("Error evaluating expression for " + eq.var);
                        dydt[i] = 0.0;
                    }
                } else {
                    dydt[i] = 0.0;
                }
            }
            return dydt;
        }
    }

    static class RK4Result {
        double[] t;
        double[][] y;

        public RK4Result(double[] t, double[][] y) {
            this.t = t;
            this.y = y;
        }
    }

    public static RK4Result rk4(DerivFunc f, double[] y0, double t0, double t_end, double h) {
        int n = (int)((t_end - t0) / h) + 1;
        double[] t = new double[n];
        double[][] y = new double[n][y0.length];

        t[0] = t0;
        y[0] = Arrays.copyOf(y0, y0.length);

        for (int i = 0; i < n - 1; i++) {
            t[i + 1] = t[i] + h;
            double[] k1 = f.compute(t[i], y[i]);

            double[] yk = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                yk[j] = y[i][j] + h * k1[j] / 2;
            }
            double[] k2 = f.compute(t[i] + h / 2, yk);

            for (int j = 0; j < y0.length; j++) {
                yk[j] = y[i][j] + h * k2[j] / 2;
            }
            double[] k3 = f.compute(t[i] + h / 2, yk);

            for (int j = 0; j < y0.length; j++) {
                yk[j] = y[i][j] + h * k3[j];
            }
            double[] k4 = f.compute(t[i] + h, yk);

            for (int j = 0; j < y0.length; j++) {
                y[i + 1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
            }
        }

        return new RK4Result(t, y);
    }

    public static void main(String[] args) {
        try {
            SystemData sys = readSystem("system.txt");
            DerivFunc f = new DerivFunc(sys.equations, sys.params);

            // Convert y0 List to array
            double[] y0 = new double[sys.y0.size()];
            for (int i = 0; i < y0.length; i++) {
                y0[i] = sys.y0.get(i);
            }

            RK4Result result = rk4(f, y0, sys.t_start, sys.t_end, sys.dt);

            // Print header
            System.out.printf("%12s", "t");
            for (Equation eq : sys.equations) {
                System.out.printf(" | %12s", eq.var + "_RK4");
            }
            System.out.println();

            // Print results
            for (int i = 0; i < result.t.length; i++) {
                System.out.printf("%12.6f", result.t[i]);
                for (double val : result.y[i]) {
                    System.out.printf(" | %12.6f", val);
                }
                System.out.println();
            }
        } catch (IOException e) {
            System.err.println("Error reading system file: " + e.getMessage());
        }
    }
}