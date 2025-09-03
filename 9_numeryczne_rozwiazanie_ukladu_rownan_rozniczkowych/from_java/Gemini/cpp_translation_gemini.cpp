#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <regex>
#include <cmath>
#include <iomanip>
#include "exprtk.hpp" // Include the ExprTk library

// Represents an equation, like 'x' = 'some_expression'
struct Equation {
    std::string var;
    std::string expr;
};

// Represents the system's data parsed from a file
struct SystemData {
    std::map<std::string, double> params;
    std::vector<double> y0;
    double tStart = 0;
    double tEnd = 1;
    double dt = 0.1;
    std::vector<Equation> equations;
};

// --- ExprTk related types and functions ---
// Map variables to their corresponding pointers for ExprTk
using symbol_table_t = exprtk::symbol_table<double>;
using expression_t = exprtk::expression<double>;
using parser_t = exprtk::parser<double>;

// This function evaluates an expression using ExprTk
double evalExpression(const std::string& expr, const std::map<std::string, double*>& vars) {
    symbol_table_t symbol_table;
    for (const auto& pair : vars) {
        symbol_table.add_variable(pair.first, *pair.second);
    }
    symbol_table.add_constants();

    expression_t expression;
    expression.register_symbol_table(symbol_table);

    parser_t parser;
    if (!parser.compile(expr, expression)) {
        throw std::runtime_error("Error compiling expression: " + expr);
    }
    return expression.value();
}

// Reads system parameters and equations from a file
SystemData readSystem(const std::string& filename) {
    SystemData data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    std::regex derivPattern(R"(^([a-zA-Z_]\w*)\'\s*=\s*(.+)$)");

    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Handle parameter assignments
        if (line.find('=') != std::string::npos && line.find('\'') == std::string::npos) {
            std::stringstream ss(line);
            std::string key, val;
            std::getline(ss, key, '=');
            std::getline(ss, val);
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);

            if (key == "y0") {
                val.erase(0, val.find_first_not_of("[]"));
                val.erase(val.find_last_not_of("[]") + 1);
                std::stringstream val_ss(val);
                std::string item;
                while (std::getline(val_ss, item, ',')) {
                    data.y0.push_back(std::stod(item));
                }
            } else if (key == "t_start") {
                data.tStart = std::stod(val);
            } else if (key == "t_end") {
                data.tEnd = std::stod(val);
            } else if (key == "dt") {
                data.dt = std::stod(val);
            } else {
                try {
                    data.params[key] = std::stod(val);
                } catch (const std::exception& e) {
                    // Assuming non-numeric params are not supported.
                }
            }
        }

        // Handle derivative equations
        std::smatch sm;
        if (std::regex_search(line, sm, derivPattern)) {
            data.equations.push_back({sm[1], sm[2]});
        }
    }
    return data;
}

// Calculates the derivatives at a given time and state
std::vector<double> deriv(double t, const std::vector<double>& y, const std::vector<Equation>& eqs, const std::map<std::string, double>& params) {
    std::map<std::string, double> vars_map = params;
    std::map<std::string, double*> vars_pointers;

    // Use a temporary map to hold all variables, including y and t
    for(const auto& pair : params) {
        vars_map[pair.first] = pair.second;
    }
    for(size_t i = 0; i < eqs.size(); ++i) {
        vars_map[eqs[i].var] = y[i];
    }
    vars_map["t"] = t;

    // Create a map of pointers for ExprTk
    for(auto& pair : vars_map) {
        vars_pointers[pair.first] = &pair.second;
    }

    std::vector<double> dydt(eqs.size());
    for (size_t i = 0; i < eqs.size(); ++i) {
        dydt[i] = evalExpression(eqs[i].expr, vars_pointers);
    }
    return dydt;
}

// The Runge-Kutta 4th-order method
void rk4(const std::vector<Equation>& eqs, const std::map<std::string, double>& params, std::vector<double> y0, double t0, double tEnd, double h) {
    int n = static_cast<int>((tEnd - t0) / h) + 1;
    std::vector<double> t(n);
    std::vector<std::vector<double>> y(n, std::vector<double>(y0.size()));

    t[0] = t0;
    y[0] = y0;

    for (int i = 0; i < n - 1; ++i) {
        std::vector<double> k1 = deriv(t[i], y[i], eqs, params);
        std::vector<double> yk2(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk2[j] = y[i][j] + h * k1[j] / 2.0;
        }

        std::vector<double> k2 = deriv(t[i] + h / 2.0, yk2, eqs, params);
        std::vector<double> yk3(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk3[j] = y[i][j] + h * k2[j] / 2.0;
        }

        std::vector<double> k3 = deriv(t[i] + h / 2.0, yk3, eqs, params);
        std::vector<double> yk4(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk4[j] = y[i][j] + h * k3[j];
        }

        std::vector<double> k4 = deriv(t[i] + h, yk4, eqs, params);

        for (size_t j = 0; j < y0.size(); ++j) {
            y[i + 1][j] = y[i][j] + (h / 6.0) * (k1[j] + 2.0 * k2[j] + 2.0 * k3[j] + k4[j]);
        }
        t[i + 1] = t[i] + h;
    }

    // Print results
    std::cout << std::left << std::setw(12) << "t";
    for (const auto& eq : eqs) {
        std::cout << " | " << std::left << std::setw(12) << (eq.var + "_RK4");
    }
    std::cout << std::endl;
    for (int i = 0; i < n; ++i) {
        std::cout << std::fixed << std::setprecision(6) << std::setw(12) << t[i];
        for (size_t j = 0; j < y0.size(); ++j) {
            std::cout << " | " << std::fixed << std::setprecision(6) << std::setw(12) << y[i][j];
        }
        std::cout << std::endl;
    }
}

int main() {
    try {
        SystemData sys = readSystem("system.txt");
        rk4(sys.equations, sys.params, sys.y0, sys.tStart, sys.tEnd, sys.dt);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}