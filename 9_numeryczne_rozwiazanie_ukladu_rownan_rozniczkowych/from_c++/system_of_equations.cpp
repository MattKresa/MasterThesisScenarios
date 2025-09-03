#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <iomanip>
#include "exprtk.hpp" // https://www.partow.net/programming/exprtk/

struct Equation {
    std::string var;
    std::string expr;
};

struct SystemData {
    std::map<std::string, double> params;
    std::vector<double> y0;
    double t_start = 0.0;
    double t_end = 1.0;
    double dt = 0.1;
    std::vector<Equation> equations;
};

SystemData read_system(const std::string& filename) {
    SystemData data;
    std::ifstream file(filename);
    std::string line;
    std::regex deriv_re(R"(^([a-zA-Z_]\w*)'\s*=\s*(.+)$)");

    while (std::getline(file, line)) {
        // Trim
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        // Parameters without derivatives
        if (line.find("=") != std::string::npos && line.find("'") == std::string::npos) {
            auto pos = line.find("=");
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);

            if (key == "y0") {
                std::stringstream ss(val);
                char c;
                double num;
                while (ss >> c) { // skip '[' or ',' 
                    if (isdigit(c) || c == '-' || c == '.') {
                        ss.putback(c);
                        ss >> num;
                        data.y0.push_back(num);
                    }
                }
            }
            else if (key == "t_start") data.t_start = std::stod(val);
            else if (key == "t_end") data.t_end = std::stod(val);
            else if (key == "dt") data.dt = std::stod(val);
            else {
                exprtk::symbol_table<double> symbol_table;
                symbol_table.add_constants();

                exprtk::expression<double> expression;
                expression.register_symbol_table(symbol_table);

                exprtk::parser<double> parser;
                if (!parser.compile(val, expression)) {
                    std::cerr << "Error parsing expression for " << key << ": " << val << "\n";
                }

                data.params[key] = expression.value();
            }
        }

        // Derivatives
        if (line.find("'") != std::string::npos && line.find("=") != std::string::npos) {
            std::smatch match;
            if (std::regex_match(line, match, deriv_re)) {
                Equation eq;
                eq.var = match[1];
                eq.expr = match[2];
                data.equations.push_back(eq);
            }
        }
    }
    return data;
}

class DerivFunc {
public:
    DerivFunc(const std::vector<Equation>& eqs, const std::map<std::string, double>& params)
        : equations(eqs)
    {
        // Copy params to variables
        for (auto& p : params) {
            vars[p.first] = p.second;
        }

        for (auto& eq : equations) {
            if (vars.find(eq.var) == vars.end()) {
                vars[eq.var] = 0.0;
            }
        }

        vars["t"] = 0.0;

        // Register variables in symbol table
        for (auto& p : vars) {
            symbol_table.add_variable(p.first, vars[p.first]);
        }
        symbol_table.add_constants();

        // Prepare expression for each equation
        for (auto& eq : equations) {
            exprtk::expression<double> expression;
            expression.register_symbol_table(symbol_table);
            parser.compile(eq.expr, expression);
            compiled_exprs.push_back(expression);
        }
    }

    std::vector<double> operator()(double t, const std::vector<double>& y) {
        // Update time and y values
        vars["t"] = t;
        for (size_t i = 0; i < equations.size(); ++i) {
            vars[equations[i].var] = y[i];
        }
        // Compute derivatives
        std::vector<double> dydt;
        dydt.reserve(equations.size());
        for (auto& expr : compiled_exprs) {
            dydt.push_back(expr.value());
        }
        return dydt;
    }

private:
    std::vector<Equation> equations;
    std::map<std::string, double> vars;
    exprtk::symbol_table<double> symbol_table;
    exprtk::parser<double> parser;
    std::vector<exprtk::expression<double>> compiled_exprs;
};

std::pair<std::vector<double>, std::vector<std::vector<double>>>
rk4(DerivFunc& f, const std::vector<double>& y0, double t0, double t_end, double h) {
    int n = static_cast<int>((t_end - t0) / h) + 1;
    std::vector<double> t(n);
    std::vector<std::vector<double>> y(n, std::vector<double>(y0.size()));

    t[0] = t0;
    y[0] = y0;

    for (int i = 0; i < n - 1; ++i) {
        t[i + 1] = t[i] + h;
        auto k1 = f(t[i], y[i]);
        auto yk = y[i];
        for (size_t j = 0; j < y0.size(); ++j) yk[j] = y[i][j] + h * k1[j] / 2;
        auto k2 = f(t[i] + h / 2, yk);
        for (size_t j = 0; j < y0.size(); ++j) yk[j] = y[i][j] + h * k2[j] / 2;
        auto k3 = f(t[i] + h / 2, yk);
        for (size_t j = 0; j < y0.size(); ++j) yk[j] = y[i][j] + h * k3[j];
        auto k4 = f(t[i] + h, yk);

        for (size_t j = 0; j < y0.size(); ++j) {
            y[i + 1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
        }
    }

    return { t, y };
}

int main() {
    SystemData sys = read_system("system.txt");
    DerivFunc f(sys.equations, sys.params);

    auto [t_rk4, y_rk4] = rk4(f, sys.y0, sys.t_start, sys.t_end, sys.dt);

    std::cout << std::setw(12) << "t";
    for (auto& eq : sys.equations) {
        std::cout << " | " << std::setw(12) << eq.var + "_RK4";
    }
    std::cout << "\n";

    for (size_t i = 0; i < t_rk4.size(); ++i) {
        std::cout << std::setw(12) << std::fixed << std::setprecision(6) << t_rk4[i];
        for (auto val : y_rk4[i]) {
            std::cout << " | " << std::setw(12) << std::fixed << std::setprecision(6) << val;
        }
        std::cout << "\n";
    }
}
