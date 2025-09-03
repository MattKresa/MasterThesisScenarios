#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <iomanip>
#include <stdexcept>
#include <cmath>
#include "exprtk.hpp" // Requires exprtk.hpp from: https://www.partow.net/programming/exprtk/

struct Equation {
    std::string var;
    std::string expr;
    Equation(const std::string& v, const std::string& e) : var(v), expr(e) {}
};

struct SystemData {
    std::map<std::string, double> params;
    std::vector<double> y0;
    double tStart = 0.0;
    double tEnd = 1.0;
    double dt = 0.1;
    std::vector<Equation> equations;
};

double evalPythonLike(const std::string& expression,
    const std::map<std::string, double>& vars)
{
    // Replace "np." with "math." equivalent
    std::string expr_str = std::regex_replace(expression, std::regex("\\bnp\\."), "");

    typedef exprtk::symbol_table<double> symbol_table_t;
    typedef exprtk::expression<double> expression_t;
    typedef exprtk::parser<double> parser_t;

    symbol_table_t symbol_table;
    for (const auto& kv : vars) {
        symbol_table.add_variable(kv.first, const_cast<double&>(kv.second));
    }
    symbol_table.add_constants();

    expression_t expr;
    expr.register_symbol_table(symbol_table);

    parser_t parser;
    if (!parser.compile(expr_str, expr)) {
        throw std::runtime_error("Error parsing expression: " + expression);
    }
    return expr.value();
}

SystemData readSystem(const std::string& filename)
{
    SystemData data;
    std::regex derivPattern("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");

    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Cannot open file: " + filename);

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        std::string line = std::regex_replace(rawLine, std::regex("^\\s+|\\s+$"), "");
        if (line.empty() || line[0] == '#') continue;

        if (line.find('=') != std::string::npos && line.find('\'') == std::string::npos) {
            auto pos = line.find('=');
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);

            key = std::regex_replace(key, std::regex("^\\s+|\\s+$"), "");
            val = std::regex_replace(val, std::regex("^\\s+|\\s+$"), "");

            if (key == "y0" || key == "Y0") {
                val = std::regex_replace(val, std::regex("[\\[\\]]"), "");
                std::stringstream ss(val);
                std::string num;
                while (std::getline(ss, num, ',')) {
                    data.y0.push_back(std::stod(num));
                }
            }
            else if (key == "t_start") {
                data.tStart = std::stod(val);
            }
            else if (key == "t_end") {
                data.tEnd = std::stod(val);
            }
            else if (key == "dt") {
                data.dt = std::stod(val);
            }
            else {
                try {
                    data.params[key] = evalPythonLike(val, {});
                }
                catch (...) {
                    // Store as 0 if not evaluatable immediately
                    data.params[key] = 0.0;
                }
            }
        }

        if (line.find('\'') != std::string::npos && line.find('=') != std::string::npos) {
            std::smatch match;
            if (std::regex_match(line, match, derivPattern)) {
                data.equations.emplace_back(match[1].str(), match[2].str());
            }
        }
    }
    return data;
}

std::vector<double> deriv(double t,
    const std::vector<double>& y,
    const std::vector<Equation>& eqs,
    const std::map<std::string, double>& params)
{
    std::map<std::string, double> vars = params;
    for (size_t i = 0; i < eqs.size(); ++i) {
        vars[eqs[i].var] = y[i];
    }
    vars["t"] = t;

    std::vector<double> dydt(eqs.size());
    for (size_t i = 0; i < eqs.size(); ++i) {
        dydt[i] = evalPythonLike(eqs[i].expr, vars);
    }
    return dydt;
}

void rk4(const std::vector<Equation>& eqs,
    const std::map<std::string, double>& params,
    const std::vector<double>& y0,
    double t0, double tEnd, double h)
{
    int n = static_cast<int>((tEnd - t0) / h) + 1;
    std::vector<double> t(n);
    std::vector<std::vector<double>> y(n, std::vector<double>(y0.size()));

    t[0] = t0;
    y[0] = y0;

    for (int i = 0; i < n - 1; ++i) {
        auto k1 = deriv(t[i], y[i], eqs, params);

        std::vector<double> yk2(y0.size()), yk3(y0.size()), yk4(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk2[j] = y[i][j] + h * k1[j] / 2;
        }
        auto k2 = deriv(t[i] + h / 2, yk2, eqs, params);

        for (size_t j = 0; j < y0.size(); ++j) {
            yk3[j] = y[i][j] + h * k2[j] / 2;
        }
        auto k3 = deriv(t[i] + h / 2, yk3, eqs, params);

        for (size_t j = 0; j < y0.size(); ++j) {
            yk4[j] = y[i][j] + h * k3[j];
        }
        auto k4 = deriv(t[i] + h, yk4, eqs, params);

        for (size_t j = 0; j < y0.size(); ++j) {
            y[i + 1][j] = y[i][j] + (h / 6.0) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
        }
        t[i + 1] = t[i] + h;
    }

    // Print results
    std::cout << std::setw(12) << "t";
    for (const auto& eq : eqs) {
        std::cout << " | " << std::setw(12) << eq.var + "_RK4";
    }
    std::cout << "\n";
    for (int i = 0; i < n; ++i) {
        std::cout << std::setw(12) << std::fixed << std::setprecision(6) << t[i];
        for (size_t j = 0; j < y0.size(); ++j) {
            std::cout << " | " << std::setw(12) << std::fixed << std::setprecision(6) << y[i][j];
        }
        std::cout << "\n";
    }
}

int main()
{
    try {
        SystemData sys = readSystem("system.txt");
        rk4(sys.equations, sys.params, sys.y0, sys.tStart, sys.tEnd, sys.dt);
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
