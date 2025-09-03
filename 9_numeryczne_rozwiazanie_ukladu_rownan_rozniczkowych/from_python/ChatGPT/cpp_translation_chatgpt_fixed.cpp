#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <regex>
#include <cmath>
#include "exprtk.hpp" // External header: https://www.partow.net/programming/exprtk/

struct Equation {
    std::string var;
    std::string expr;
};

// Read system from file
void read_system(
    const std::string& filename,
    std::map<std::string, double>& params,
    std::vector<double>& y0,
    double& t_start,
    double& t_end,
    double& dt,
    std::vector<Equation>& equations
) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    std::regex deriv_regex(R"(^([a-zA-Z_]\w*)'\s*=\s*(.+)$)");

    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        // parameters (without derivatives)
        if (line.find("=") != std::string::npos && line.find("'") == std::string::npos) {
            auto pos = line.find("=");
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);

            // trim
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);

            try {
                exprtk::symbol_table<double> symbol_table;
                symbol_table.add_constants();

                exprtk::expression<double> expression;
                expression.register_symbol_table(symbol_table);

                exprtk::parser<double> parser;
                if (!parser.compile(val, expression) && key!="y0") {
                    std::cerr << "Error parsing expression for " << key << ": " << val << "\n";
                }

                params[key] = expression.value();
            }
            catch (...) {
                // If it's not a number, leave as variable (0 init)
                params[key] = 0.0;
            }
        }

        // initial conditions and time
        std::string low = line;
        for (auto& c : low) c = std::tolower(c);

        if (low.rfind("y0", 0) == 0) {
            auto pos = line.find("=");
            std::string arr = line.substr(pos + 1);
            arr.erase(std::remove(arr.begin(), arr.end(), '['), arr.end());
            arr.erase(std::remove(arr.begin(), arr.end(), ']'), arr.end());
            std::stringstream ss(arr);
            double val;
            while (ss >> val) {
                y0.push_back(val);
                if (ss.peek() == ',' || ss.peek() == ' ') ss.ignore();
            }
        }
        else if (low.rfind("t_start", 0) == 0) {
            t_start = std::stod(line.substr(line.find("=") + 1));
        }
        else if (low.rfind("t_end", 0) == 0) {
            t_end = std::stod(line.substr(line.find("=") + 1));
        }
        else if (low.rfind("dt", 0) == 0) {
            dt = std::stod(line.substr(line.find("=") + 1));
        }

        // derivatives
        std::smatch match;
        if (std::regex_match(line, match, deriv_regex)) {
            Equation eq;
            eq.var = match[1];
            eq.expr = match[2];
            equations.push_back(eq);
        }
    }
}

// Create derivative function
auto make_deriv_func(const std::vector<Equation>& equations,
    const std::map<std::string, double>& params)
{
    return [equations, params](double t, const std::vector<double>& y) {
        using symbol_table_t = exprtk::symbol_table<double>;
        using expression_t = exprtk::expression<double>;
        using parser_t = exprtk::parser<double>;

        std::vector<double> dydt;
        dydt.reserve(equations.size());

        symbol_table_t symbol_table;
        // Copy parameters
        std::map<std::string, double> vars = params;
        vars["t"] = t;

        // Map y-values to variable names
        for (size_t i = 0; i < equations.size(); ++i) {
            vars[equations[i].var] = y[i];
        }

        for (auto& [name, value] : vars) {
            symbol_table.add_variable(name, value);
        }

        for (auto& eq : equations) {
            expression_t expression;
            expression.register_symbol_table(symbol_table);
            parser_t parser;
            if (!parser.compile(eq.expr, expression)) {
                throw std::runtime_error("Failed to parse: " + eq.expr);
            }
            dydt.push_back(expression.value());
        }
        return dydt;
        };
}

// RK4 solver
std::pair<std::vector<double>, std::vector<std::vector<double>>>
rk4(const std::function<std::vector<double>(double, const std::vector<double>&)>& f,
    const std::vector<double>& y0,
    double t0, double t_end, double h)
{
    int n = static_cast<int>((t_end - t0) / h) + 1;
    std::vector<double> t(n);
    std::vector<std::vector<double>> y(n, std::vector<double>(y0.size()));

    t[0] = t0;
    y[0] = y0;

    for (int i = 0; i < n - 1; ++i) {
        t[i + 1] = t[i] + h;

        auto k1 = f(t[i], y[i]);

        std::vector<double> yk(y[i].size());
        for (size_t j = 0; j < y[i].size(); ++j)
            yk[j] = y[i][j] + h * k1[j] / 2.0;
        auto k2 = f(t[i] + h / 2.0, yk);

        for (size_t j = 0; j < y[i].size(); ++j)
            yk[j] = y[i][j] + h * k2[j] / 2.0;
        auto k3 = f(t[i] + h / 2.0, yk);

        for (size_t j = 0; j < y[i].size(); ++j)
            yk[j] = y[i][j] + h * k3[j];
        auto k4 = f(t[i] + h, yk);

        for (size_t j = 0; j < y[i].size(); ++j) {
            y[i + 1][j] = y[i][j] + (h / 6.0) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
        }
    }

    return { t, y };
}

// --- MAIN ---
int main() {
    std::map<std::string, double> params;
    std::vector<double> y0;
    double t_start = 0.0, t_end = 1.0, dt = 0.1;
    std::vector<Equation> equations;

    try {
        read_system("system.txt", params, y0, t_start, t_end, dt, equations);

        auto f = make_deriv_func(equations, params);
        auto [t_rk4, y_rk4] = rk4(f, y0, t_start, t_end, dt);

        std::cout << std::setw(12) << "t";
        for (auto& eq : equations)
            std::cout << " | " << std::setw(12) << eq.var + "_RK4";
        std::cout << "\n";

        for (size_t i = 0; i < t_rk4.size(); ++i) {
            std::cout << std::setw(12) << std::fixed << std::setprecision(6) << t_rk4[i];
            for (double val : y_rk4[i]) {
                std::cout << " | " << std::setw(12) << std::fixed << std::setprecision(6) << val;
            }
            std::cout << "\n";
        }

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
