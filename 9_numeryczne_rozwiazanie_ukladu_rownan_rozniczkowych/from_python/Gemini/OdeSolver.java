import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import org.mariuszgromada.math.mxparser.*;

public class OdeSolver {

    public static class SystemData {
        public Map<String, Double> params = new HashMap<>();
        public List<Double> y0 = new ArrayList<>();
        public double tStart = 0;
        public double tEnd = 1;
        public double dt = 0.1;
        public List<Equation> equations = new ArrayList<>();
    }

    public static class Equation {
        String variable;
        String expression;

        public Equation(String variable, String expression) {
            this.variable = variable;
            this.expression = expression;
        }
    }

    public static SystemData readSystem(String filename) throws IOException {
        SystemData data = new SystemData();
        Pattern derivativePattern = Pattern.compile("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

        try (BufferedReader br = new BufferedReader(new FileReader(filename))) {
            String line;
            while ((line = br.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) {
                    continue;
                }

                if (line.contains("=")) {
                    String[] parts = line.split("=", 2);
                    String key = parts[0].trim();
                    String value = parts[1].trim();

                    // Parameters (without derivatives)
                    if (!key.contains("'")) {
                        try {
                            // Use mXparser to handle expressions like 'np.pi'
                            Expression expr = new Expression(value);
                            data.params.put(key, expr.calculate());
                        } catch (Exception e) {
                            // Handle cases where the value is not a simple number or expression
                            // For simplicity, we assume numeric values here.
                        }
                    }

                    // Initial conditions and time
                    if (key.equalsIgnoreCase("y0")) {
                        String y0String = value.replaceAll("\\[|\\]|np.array", "");
                        String[] y0Values = y0String.split(",");
                        for (String val : y0Values) {
                            Expression expr = new Expression(val.trim());
                            data.y0.add(expr.calculate());
                        }
                    } else if (key.equalsIgnoreCase("t_start")) {
                        data.tStart = Double.parseDouble(value);
                    } else if (key.equalsIgnoreCase("t_end")) {
                        data.tEnd = Double.parseDouble(value);
                    } else if (key.equalsIgnoreCase("dt")) {
                        data.dt = Double.parseDouble(value);
                    }
                }

                // Derivatives
                Matcher matcher = derivativePattern.matcher(line);
                if (matcher.find()) {
                    String var = matcher.group(1);
                    String expr = matcher.group(2);
                    data.equations.add(new Equation(var, expr));
                }
            }
        }
        return data;
    }
    
    public static class DerivativeFunction {
        private final List<Expression> expressions = new ArrayList<>();
        private final Argument tArg = new Argument("t", 0.0);
        private final Map<String, Argument> yArgs = new HashMap<>();

        public DerivativeFunction(List<Equation> equations, Map<String, Double> params) {
            // First, initialize arguments for parameters
            for (Map.Entry<String, Double> param : params.entrySet()) {
                Argument arg = new Argument(param.getKey(), param.getValue());
                yArgs.put(param.getKey(), arg);
            }

            // Then, set up arguments for the variables to be solved
            for (Equation eq : equations) {
                Argument arg = new Argument(eq.variable, 0.0);
                yArgs.put(eq.variable, arg);
            }

            // Finally, create expressions for each equation using the arguments
            for (Equation eq : equations) {
                Expression expr = new Expression(eq.expression);
                expr.addArguments(tArg);
                expr.addArguments(yArgs.values().toArray(new Argument[0]));
                expressions.add(expr);
            }
        }

        public double[] apply(double t, double[] y) {
            tArg.setArgumentValue(t);
            int i = 0;
            for (Map.Entry<String, Argument> entry : yArgs.entrySet()) {
                if (i < y.length) {
                    entry.getValue().setArgumentValue(y[i]);
                    i++;
                }
            }

            double[] dydt = new double[expressions.size()];
            for (int j = 0; j < expressions.size(); j++) {
                dydt[j] = expressions.get(j).calculate();
            }
            return dydt;
        }
    }

    public static class RK4Result {
        public double[] t;
        public double[][] y;
    
        public RK4Result(double[] t, double[][] y) {
            this.t = t;
            this.y = y;
        }
    }

    public static RK4Result rk4(DerivativeFunction f, double[] y0, double tStart, double tEnd, double h) {
        int n = (int) Math.round((tEnd - tStart) / h) + 1;
        double[] t = new double[n];
        for (int i = 0; i < n; i++) {
            t[i] = tStart + i * h;
        }
        
        double[][] y = new double[n][y0.length];
        y[0] = Arrays.copyOf(y0, y0.length);

        for (int i = 0; i < n - 1; i++) {
            double[] k1 = f.apply(t[i], y[i]);
            
            double[] yk2 = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                yk2[j] = y[i][j] + h * k1[j] / 2;
            }
            double[] k2 = f.apply(t[i] + h / 2, yk2);
            
            double[] yk3 = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                yk3[j] = y[i][j] + h * k2[j] / 2;
            }
            double[] k3 = f.apply(t[i] + h / 2, yk3);
            
            double[] yk4 = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                yk4[j] = y[i][j] + h * k3[j];
            }
            double[] k4 = f.apply(t[i] + h, yk4);
            
            y[i+1] = new double[y0.length];
            for (int j = 0; j < y0.length; j++) {
                y[i+1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
            }
        }
        return new RK4Result(t, y);
    }

    public static void main(String[] args) {
        try {
            SystemData data = readSystem("system.txt");
            
            DerivativeFunction f = new DerivativeFunction(data.equations, data.params);

            double[] y0Array = data.y0.stream().mapToDouble(d -> d).toArray();
            RK4Result result = rk4(f, y0Array, data.tStart, data.tEnd, data.dt);

            double[] tRk4 = result.t;
            double[][] yRk4 = result.y;

            List<String> header = new ArrayList<>();
            header.add("t");
            for (Equation eq : data.equations) {
                header.add(eq.variable + "_RK4");
            }
            String headerString = header.stream()
                                      .map(h -> String.format("%12s", h))
                                      .collect(Collectors.joining(" | "));
            System.out.println(headerString);

            for (int i = 0; i < tRk4.length; i++) {
                StringBuilder row = new StringBuilder();
                row.append(String.format("%12.6f", tRk4[i]));
                for (double val : yRk4[i]) {
                    row.append(" | ").append(String.format("%12.6f", val));
                }
                System.out.println(row.toString());
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}