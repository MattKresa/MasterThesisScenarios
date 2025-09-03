// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/ODESolver.java) ---
package org.example;

import java.io.*;
import java.util.*;
import java.util.regex.*;
import javax.script.*;

class Equation {
    public String var;
    public String expr;

    public Equation(String var, String expr) {
        this.var = var;
        this.expr = expr;
    }
}

class SystemData {
    public Map<String, Double> params;
    public List<Double> y0;
    public double tStart = 0.0;
    public double tEnd = 1.0;
    public double dt = 0.1;
    public List<Equation> equations;

    public SystemData() {
        this.params = new HashMap<>();
        this.y0 = new ArrayList<>();
        this.equations = new ArrayList<>();
    }
}

class ODESolver {

    public static SystemData readSystem(String filename) throws IOException {
        SystemData data = new SystemData();
        Pattern derivPattern = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

        try (BufferedReader reader = new BufferedReader(new FileReader(filename))) {
            String line;
            while ((line = reader.readLine()) != null) {
                // Trim whitespace
                line = line.trim();

                // Skip empty lines and comments
                if (line.isEmpty() || line.startsWith("#")) {
                    continue;
                }

                // Parameters without derivatives
                if (line.contains("=") && !line.contains("'")) {
                    String[] parts = line.split("=", 2);
                    String key = parts[0].trim();
                    String val = parts[1].trim();

                    if (key.equals("y0")) {
                        // Parse array-like string: [1, 2, 3] or 1,2,3
                        val = val.replaceAll("[\\[\\]\\s]", "");
                        String[] values = val.split(",");
                        for (String v : values) {
                            if (!v.isEmpty()) {
                                data.y0.add(Double.parseDouble(v));
                            }
                        }
                    } else if (key.equals("t_start")) {
                        data.tStart = Double.parseDouble(val);
                    } else if (key.equals("t_end")) {
                        data.tEnd = Double.parseDouble(val);
                    } else if (key.equals("dt")) {
                        data.dt = Double.parseDouble(val);
                    } else {
                        // Evaluate mathematical expressions
                        try {
                            double value = evaluateExpression(val);
                            data.params.put(key, value);
                        } catch (Exception e) {
                            System.err.println("Error parsing expression for " + key + ": " + val);
                            data.params.put(key, Double.parseDouble(val));
                        }
                    }
                }

                // Derivatives (equations with prime notation)
                else if (line.contains("'") && line.contains("=")) {
                    Matcher matcher = derivPattern.matcher(line);
                    if (matcher.matches()) {
                        String var = matcher.group(1);
                        String expr = matcher.group(2);
                        data.equations.add(new Equation(var, expr));
                    }
                }
            }
        }

        return data;
    }

    private static double evaluateExpression(String expression) throws ScriptException {
        ScriptEngineManager manager = new ScriptEngineManager();
        ScriptEngine engine = manager.getEngineByName("JavaScript");

        // Replace mathematical notation
        expression = expression.replace("^", "**");

        // Add mathematical functions
        engine.eval("var sin = Math.sin;");
        engine.eval("var cos = Math.cos;");
        engine.eval("var tan = Math.tan;");
        engine.eval("var exp = Math.exp;");
        engine.eval("var log = Math.log;");
        engine.eval("var sqrt = Math.sqrt;");
        engine.eval("var abs = Math.abs;");
        engine.eval("var pow = Math.pow;");
        engine.eval("var pi = Math.PI;");
        engine.eval("var e = Math.E;");

        Object result = engine.eval(expression);
        return ((Number) result).doubleValue();
    }

    static class DerivFunc {
        private List<Equation> equations;
        private Map<String, Double> vars;
        private ScriptEngine engine;
        private List<String> compiledExprs;

        public DerivFunc(List<Equation> equations, Map<String, Double> params) throws ScriptException {
            this.equations = new ArrayList<>(equations);
            this.vars = new HashMap<>(params);
            this.compiledExprs = new ArrayList<>();

            // Initialize variables for each equation
            for (Equation eq : equations) {
                if (!vars.containsKey(eq.var)) {
                    vars.put(eq.var, 0.0);
                }
            }
            vars.put("t", 0.0);

            // Setup JavaScript engine
            ScriptEngineManager manager = new ScriptEngineManager();
            engine = manager.getEngineByName("JavaScript");

            // Add mathematical functions
            engine.eval("var sin = Math.sin;");
            engine.eval("var cos = Math.cos;");
            engine.eval("var tan = Math.tan;");
            engine.eval("var exp = Math.exp;");
            engine.eval("var log = Math.log;");
            engine.eval("var sqrt = Math.sqrt;");
            engine.eval("var abs = Math.abs;");
            engine.eval("var pow = Math.pow;");
            engine.eval("var pi = Math.PI;");
            engine.eval("var e = Math.E;");

            // Prepare expressions
            for (Equation eq : equations) {
                String expr = eq.expr.replace("^", "**");
                compiledExprs.add(expr);
            }
        }

        public List<Double> apply(double t, List<Double> y) throws ScriptException {
            // Update time and state variables
            vars.put("t", t);
            for (int i = 0; i < equations.size(); i++) {
                vars.put(equations.get(i).var, y.get(i));
            }

            // Set variables in script engine
            for (Map.Entry<String, Double> entry : vars.entrySet()) {
                engine.put(entry.getKey(), entry.getValue());
            }

            // Compute derivatives
            List<Double> dydt = new ArrayList<>();
            for (String expr : compiledExprs) {
                try {
                    Object result = engine.eval(expr);
                    dydt.add(((Number) result).doubleValue());
                } catch (Exception e) {
                    System.err.println("Error evaluating expression: " + expr);
                    dydt.add(0.0);
                }
            }

            return dydt;
        }
    }

    public static class Result {
        public List<Double> t;
        public List<List<Double>> y;

        public Result(List<Double> t, List<List<Double>> y) {
            this.t = t;
            this.y = y;
        }
    }

    public static Result rk4(DerivFunc f, List<Double> y0, double t0, double tEnd, double h) throws ScriptException {
        int n = (int) ((tEnd - t0) / h) + 1;
        List<Double> t = new ArrayList<>(n);
        List<List<Double>> y = new ArrayList<>(n);

        // Initialize arrays
        for (int i = 0; i < n; i++) {
            t.add(0.0);
            y.add(new ArrayList<>(Collections.nCopies(y0.size(), 0.0)));
        }

        t.set(0, t0);
        y.set(0, new ArrayList<>(y0));

        for (int i = 0; i < n - 1; i++) {
            t.set(i + 1, t.get(i) + h);

            List<Double> k1 = f.apply(t.get(i), y.get(i));

            List<Double> yk = new ArrayList<>(y.get(i));
            for (int j = 0; j < y0.size(); j++) {
                yk.set(j, y.get(i).get(j) + h * k1.get(j) / 2);
            }
            List<Double> k2 = f.apply(t.get(i) + h / 2, yk);

            for (int j = 0; j < y0.size(); j++) {
                yk.set(j, y.get(i).get(j) + h * k2.get(j) / 2);
            }
            List<Double> k3 = f.apply(t.get(i) + h / 2, yk);

            for (int j = 0; j < y0.size(); j++) {
                yk.set(j, y.get(i).get(j) + h * k3.get(j));
            }
            List<Double> k4 = f.apply(t.get(i) + h, yk);

            for (int j = 0; j < y0.size(); j++) {
                double newValue = y.get(i).get(j) + (h / 6) *
                        (k1.get(j) + 2 * k2.get(j) + 2 * k3.get(j) + k4.get(j));
                y.get(i + 1).set(j, newValue);
            }
        }

        return new Result(t, y);
    }

    public static void main(String[] args) {
        try {
            SystemData sys = readSystem("system.txt");
            DerivFunc f = new DerivFunc(sys.equations, sys.params);

            Result result = rk4(f, sys.y0, sys.tStart, sys.tEnd, sys.dt);

            // Print header
            System.out.printf("%12s", "t");
            for (Equation eq : sys.equations) {
                System.out.printf(" | %12s", eq.var + "_RK4");
            }
            System.out.println();

            // Print results
            for (int i = 0; i < result.t.size(); i++) {
                System.out.printf("%12.6f", result.t.get(i));
                for (int j = 0; j < result.y.get(i).size(); j++) {
                    System.out.printf(" | %12.6f", result.y.get(i).get(j));
                }
                System.out.println();
            }

        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}