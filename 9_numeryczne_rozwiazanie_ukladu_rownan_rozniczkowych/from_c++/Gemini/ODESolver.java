// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ODESolver.java) ---
package org.example;

import org.mariuszgromada.math.mxparser.Argument;
import org.mariuszgromada.math.mxparser.Expression;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.AbstractMap.SimpleEntry;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.text.DecimalFormat;

public class ODESolver {
    // Nested static classes to keep all related code in one file
    public static class Equation {
        public String var;
        public String expr;

        public Equation(String var, String expr) {
            this.var = var;
            this.expr = expr;
        }
    }

    public static class SystemData {
        public Map<String, Double> params = new HashMap<>();
        public List<Double> y0 = new ArrayList<>();
        public double t_start = 0.0;
        public double t_end = 1.0;
        public double dt = 0.1;
        public List<Equation> equations = new ArrayList<>();
    }

    public static class DerivFunc {
        private final List<Equation> equations;
        private final Map<String, Argument> vars = new HashMap<>();
        private final List<Expression> compiledExprs = new ArrayList<>();

        public DerivFunc(List<Equation> eqs, Map<String, Double> params) {
            this.equations = eqs;

            for (Map.Entry<String, Double> entry : params.entrySet()) {
                vars.put(entry.getKey(), new Argument(entry.getKey(), entry.getValue()));
            }

            for (Equation eq : equations) {
                if (!vars.containsKey(eq.var)) {
                    vars.put(eq.var, new Argument(eq.var, 0.0));
                }
            }

            vars.put("t", new Argument("t", 0.0));

            for (Equation eq : equations) {
                Expression expression = new Expression(eq.expr);
                for (Argument arg : vars.values()) {
                    expression.addArguments(arg);
                }
                if (!expression.checkSyntax()) {
                    throw new IllegalArgumentException("Syntax error in expression: " + eq.expr + " for variable: " + eq.var);
                }
                compiledExprs.add(expression);
            }
        }

        public List<Double> apply(double t, List<Double> y) {
            vars.get("t").setArgumentValue(t);
            for (int i = 0; i < equations.size(); i++) {
                vars.get(equations.get(i).var).setArgumentValue(y.get(i));
            }

            List<Double> dydt = new ArrayList<>();
            for (Expression expr : compiledExprs) {
                dydt.add(expr.calculate());
            }
            return dydt;
        }
    }

    public static class RK4 {
        public static Map.Entry<List<Double>, List<List<Double>>> solve(DerivFunc f, List<Double> y0, double t0, double t_end, double h) {
            int n = (int) Math.round((t_end - t0) / h) + 1;
            List<Double> t = new ArrayList<>(n);
            List<List<Double>> y = new ArrayList<>(n);

            t.add(t0);
            y.add(y0);

            for (int i = 0; i < n - 1; i++) {
                double currentT = t.get(i);
                List<Double> currentY = y.get(i);

                List<Double> k1 = f.apply(currentT, currentY);
                List<Double> yk = new ArrayList<>(y0.size());
                for (int j = 0; j < y0.size(); j++) {
                    yk.add(currentY.get(j) + h * k1.get(j) / 2);
                }

                List<Double> k2 = f.apply(currentT + h / 2, yk);
                yk = new ArrayList<>(y0.size());
                for (int j = 0; j < y0.size(); j++) {
                    yk.add(currentY.get(j) + h * k2.get(j) / 2);
                }

                List<Double> k3 = f.apply(currentT + h / 2, yk);
                yk = new ArrayList<>(y0.size());
                for (int j = 0; j < y0.size(); j++) {
                    yk.add(currentY.get(j) + h * k3.get(j));
                }

                List<Double> k4 = f.apply(currentT + h, yk);

                List<Double> nextY = new ArrayList<>(y0.size());
                for (int j = 0; j < y0.size(); j++) {
                    nextY.add(currentY.get(j) + (h / 6) * (k1.get(j) + 2 * k2.get(j) + 2 * k3.get(j) + k4.get(j)));
                }

                t.add(currentT + h);
                y.add(nextY);
            }

            return new SimpleEntry<>(t, y);
        }
    }

    public static void main(String[] args) {
        try {
            SystemData sys = readSystem("system.txt");
            DerivFunc f = new DerivFunc(sys.equations, sys.params);

            Map.Entry<List<Double>, List<List<Double>>> result = RK4.solve(f, sys.y0, sys.t_start, sys.t_end, sys.dt);
            List<Double> tRk4 = result.getKey();
            List<List<Double>> yRk4 = result.getValue();

            // Print header
            System.out.printf("%12s", "t");
            for (Equation eq : sys.equations) {
                System.out.printf(" | %12s", eq.var + "_RK4");
            }
            System.out.println();

            // Print results
            DecimalFormat df = new DecimalFormat("0.000000");
            for (int i = 0; i < tRk4.size(); i++) {
                System.out.printf("%12s", df.format(tRk4.get(i)));
                for (Double val : yRk4.get(i)) {
                    System.out.printf(" | %12s", df.format(val));
                }
                System.out.println();
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static SystemData readSystem(String filename) throws IOException {
        SystemData data = new SystemData();
        BufferedReader reader = new BufferedReader(new FileReader(filename));
        String line;
        Pattern derivRe = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

        while ((line = reader.readLine()) != null) {
            line = line.trim();

            if (line.isEmpty() || line.startsWith("#")) {
                continue;
            }

            if (line.contains("=") && !line.contains("'")) {
                String[] parts = line.split("=", 2);
                String key = parts[0].trim();
                String val = parts[1].trim();

                switch (key) {
                    case "y0":
                        val = val.replaceAll("[\\[\\]\\s]", "");
                        String[] y0Values = val.split(",");
                        for (String y0Str : y0Values) {
                            if (!y0Str.isEmpty()) {
                                data.y0.add(Double.parseDouble(y0Str));
                            }
                        }
                        break;
                    case "t_start":
                        data.t_start = Double.parseDouble(val);
                        break;
                    case "t_end":
                        data.t_end = Double.parseDouble(val);
                        break;
                    case "dt":
                        data.dt = Double.parseDouble(val);
                        break;
                    default:
                        Expression expr = new Expression(val);
                        if (!expr.checkSyntax()) {
                            throw new IllegalArgumentException("Error parsing expression for " + key + ": " + val);
                        }
                        data.params.put(key, expr.calculate());
                        break;
                }
            }

            if (line.contains("'") && line.contains("=")) {
                Matcher matcher = derivRe.matcher(line);
                if (matcher.matches()) {
                    String var = matcher.group(1);
                    String expr = matcher.group(2);
                    data.equations.add(new Equation(var, expr));
                }
            }
        }
        reader.close();
        return data;
    }
}