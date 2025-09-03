#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include "exprtk.hpp"  // Download from: https://github.com/ArashPartow/exprtk

class Equation {
public:
    std::string var;
    std::string expr;

    Equation(const std::string& variable, const std::string& expression)
        : var(variable), expr(expression) {
    }
};

class SystemData {
public:
    std::map<std::string, double> params;
    std::vector<double> y0;
    double tStart = 0.0;
    double tEnd = 1.0;
    double dt = 0.1;
    std::vector<Equation> equations;
};

// Expression evaluator using exprtk library
class ExpressionEvaluator {
private:
    exprtk::symbol_table<double> symbol_table;
    exprtk::expression<double> expression;
    exprtk::parser<double> parser;

public:
    ExpressionEvaluator() {
        // Register mathematical constants
        symbol_table.add_constants();
        expression.register_symbol_table(symbol_table);
    }

    double evaluate(const std::string& expr_str, const std::map<std::string, double>& vars) {
        // Clear previous variables
        symbol_table.clear_variables();

        // Add all variables to symbol table
        for (const auto& var : vars) {
            symbol_table.add_variable(var.first, const_cast<double&>(var.second));
        }

        // Preprocess expression to handle numpy-like syntax
        std::string processed_expr = expr_str;

        // Replace np. with nothing (exprtk has built-in math functions)
        std::regex np_regex("\\bnp\\.");
        processed_expr = std::regex_replace(processed_expr, np_regex, "");

        // Replace Math. with nothing
        std::regex math_regex("\\bMath\\.");
        processed_expr = std::regex_replace(processed_expr, math_regex, "");

        // Replace ** with ^ for power operator
        std::regex power_regex("\\*\\*");
        processed_expr = std::regex_replace(processed_expr, power_regex, "^");

        if (!parser.compile(processed_expr, expression)) {
            throw std::runtime_error("Failed to parse expression: " + expr_str +
                "\nError: " + parser.error());
        }

        return expression.value();
    }
};

// Global expression evaluator instance
ExpressionEvaluator evaluator;

double evalPythonLike(const std::string& expr, const std::map<std::string, double>& vars) {
    return evaluator.evaluate(expr, vars);
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

SystemData readSystem(const std::string& filename) {
    SystemData data;
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::regex derivPattern("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        // Parameters without derivatives
        if (line.find('=') != std::string::npos && line.find('\'') == std::string::npos) {
            auto parts = split(line, '=');
            if (parts.size() != 2) continue;

            std::string key = trim(parts[0]);
            std::string val = trim(parts[1]);

            if (toLower(key) == "y0") {
                // Remove brackets and parse array
                val.erase(std::remove(val.begin(), val.end(), '['), val.end());
                val.erase(std::remove(val.begin(), val.end(), ']'), val.end());

                auto values = split(val, ',');
                for (const auto& v : values) {
                    if (!v.empty()) {
                        data.y0.push_back(std::stod(v));
                    }
                }
            }
            else if (toLower(key) == "t_start") {
                data.tStart = std::stod(val);
            }
            else if (toLower(key) == "t_end") {
                data.tEnd = std::stod(val);
            }
            else if (toLower(key) == "dt") {
                data.dt = std::stod(val);
            }
            else {
                try {
                    std::map<std::string, double> emptyVars;
                    double result = evalPythonLike(val, emptyVars);
                    data.params[key] = result;
                }
                catch (...) {
                    // If evaluation fails, try to parse as number
                    try {
                        data.params[key] = std::stod(val);
                    }
                    catch (...) {
                        std::cerr << "Warning: Could not parse parameter " << key << " = " << val << std::endl;
                    }
                }
            }
        }

        // Derivatives
        if (line.find('\'') != std::string::npos && line.find('=') != std::string::npos) {
            std::smatch match;
            if (std::regex_match(line, match, derivPattern)) {
                std::string var = trim(match[1].str());
                std::string expr = trim(match[2].str());
                data.equations.emplace_back(var, expr);
            }
        }
    }

    file.close();
    return data;
}

std::vector<double> deriv(double t, const std::vector<double>& y,
    const std::vector<Equation>& equations,
    const std::map<std::string, double>& params) {
    std::map<std::string, double> vars = params;

    // Add current state variables
    for (size_t i = 0; i < equations.size(); ++i) {
        vars[equations[i].var] = y[i];
    }
    vars["t"] = t;

    std::vector<double> dydt(equations.size());
    for (size_t i = 0; i < equations.size(); ++i) {
        dydt[i] = evalPythonLike(equations[i].expr, vars);
    }

    return dydt;
}

void rk4(const std::vector<Equation>& equations,
    const std::map<std::string, double>& params,
    const std::vector<double>& y0,
    double t0, double tEnd, double h) {

    int n = static_cast<int>((tEnd - t0) / h) + 1;
    std::vector<double> t(n);
    std::vector<std::vector<double>> y(n, std::vector<double>(y0.size()));

    t[0] = t0;
    y[0] = y0;

    for (int i = 0; i < n - 1; ++i) {
        auto k1 = deriv(t[i], y[i], equations, params);

        std::vector<double> yk2(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk2[j] = y[i][j] + h * k1[j] / 2;
        }
        auto k2 = deriv(t[i] + h / 2, yk2, equations, params);

        std::vector<double> yk3(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk3[j] = y[i][j] + h * k2[j] / 2;
        }
        auto k3 = deriv(t[i] + h / 2, yk3, equations, params);

        std::vector<double> yk4(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk4[j] = y[i][j] + h * k3[j];
        }
        auto k4 = deriv(t[i] + h, yk4, equations, params);

        // Update y and t
        for (size_t j = 0; j < y0.size(); ++j) {
            y[i + 1][j] = y[i][j] + (h / 6.0) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
        }
        t[i + 1] = t[i] + h;
    }

    // Print results
    std::cout << std::setw(12) << "t";
    for (const auto& eq : equations) {
        std::cout << " | " << std::setw(12) << (eq.var + "_RK4");
    }
    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(6);
    for (int i = 0; i < n; ++i) {
        std::cout << std::setw(12) << t[i];
        for (size_t j = 0; j < y0.size(); ++j) {
            std::cout << " | " << std::setw(12) << y[i][j];
        }
        std::cout << std::endl;
    }
}

int main() {
    try {
        SystemData systemData = readSystem("system.txt");

        if (systemData.equations.empty()) {
            std::cerr << "No differential equations found in system.txt" << std::endl;
            return 1;
        }

        if (systemData.y0.empty()) {
            std::cerr << "No initial conditions (y0) found in system.txt" << std::endl;
            return 1;
        }

        if (systemData.y0.size() != systemData.equations.size()) {
            std::cerr << "Mismatch: " << systemData.equations.size()
                << " equations but " << systemData.y0.size()
                << " initial conditions" << std::endl;
            return 1;
        }

        rk4(systemData.equations, systemData.params, systemData.y0,
            systemData.tStart, systemData.tEnd, systemData.dt);

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}