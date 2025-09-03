#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <iomanip>
#include <regex>
#include <stdexcept>

// Include ExprTk header (you'll need to download and include this in your project)
#include "exprtk.hpp"

using namespace std;

class Equation {
public:
    string var;
    string expr;
    Equation(const string& var, const string& expr) : var(var), expr(expr) {}
};

class SystemData {
public:
    map<string, double> params;
    vector<double> y0;
    double tStart = 0;
    double tEnd = 1;
    double dt = 0.1;
    vector<Equation> equations;
};

class ExpressionEvaluator {
private:
    exprtk::symbol_table<double> symbol_table;
    exprtk::expression<double> expression;
    exprtk::parser<double> parser;

public:
    static double evaluate(const string& expr_str, const map<string, double>& vars) {
        exprtk::symbol_table<double> symbol_table;

        // Add all variables to the symbol table
        for (const auto& var : vars) {
            symbol_table.add_variable(var.first, const_cast<double&>(var.second));
        }

        // Add constants and functions
        symbol_table.add_constants();

        exprtk::expression<double> expression;
        expression.register_symbol_table(symbol_table);

        exprtk::parser<double> parser;
        if (!parser.compile(expr_str, expression)) {
            throw runtime_error("Failed to compile expression: " + expr_str);
        }

        return expression.value();
    }
};

SystemData readSystem(const string& filename) {
    SystemData data;
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }

    regex derivPattern(R"(^([a-zA-Z_]\w*)'\s*=\s*(.+)$)");
    string line;

    while (getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty() || line[0] == '#') continue;

        // Parameters without derivatives
        if (line.find('=') != string::npos && line.find('\'') == string::npos) {
            size_t eqPos = line.find('=');
            string key = line.substr(0, eqPos);
            key.erase(key.find_last_not_of(" \t") + 1);

            string val = line.substr(eqPos + 1);
            val.erase(0, val.find_first_not_of(" \t"));

            if (key == "y0") {
                // Parse array [val1, val2, ...]
                val.erase(remove(val.begin(), val.end(), '['), val.end());
                val.erase(remove(val.begin(), val.end(), ']'), val.end());
                istringstream iss(val);
                string num;
                while (getline(iss, num, ',')) {
                    num.erase(0, num.find_first_not_of(" \t"));
                    num.erase(num.find_last_not_of(" \t") + 1);
                    data.y0.push_back(stod(num));
                }
            }
            else if (key == "t_start") {
                data.tStart = stod(val);
            }
            else if (key == "t_end") {
                data.tEnd = stod(val);
            }
            else if (key == "dt") {
                data.dt = stod(val);
            }
            else {
                try {
                    double res = ExpressionEvaluator::evaluate(val, {});
                    data.params[key] = res;
                }
                catch (...) {
                    cerr << "Warning: Could not evaluate parameter " << key << ", treating as 0" << endl;
                    data.params[key] = 0;
                }
            }
        }

        // Derivatives
        if (line.find('\'') != string::npos && line.find('=') != string::npos) {
            smatch matches;
            if (regex_search(line, matches, derivPattern)) {
                data.equations.emplace_back(matches[1].str(), matches[2].str());
            }
        }
    }

    return data;
}

vector<double> deriv(double t, const vector<double>& y, const vector<Equation>& eqs, const map<string, double>& params) {
    map<string, double> vars(params);
    for (size_t i = 0; i < eqs.size(); ++i) {
        vars[eqs[i].var] = y[i];
    }
    vars["t"] = t;

    vector<double> dydt(eqs.size());
    for (size_t i = 0; i < eqs.size(); ++i) {
        try {
            dydt[i] = ExpressionEvaluator::evaluate(eqs[i].expr, vars);
        }
        catch (const exception& e) {
            cerr << "Error evaluating derivative for " << eqs[i].var << ": " << e.what() << endl;
            throw;
        }
    }
    return dydt;
}

void rk4(const vector<Equation>& eqs, const map<string, double>& params, const vector<double>& y0,
    double t0, double tEnd, double h) {
    int n = static_cast<int>((tEnd - t0) / h) + 1;
    vector<double> t(n);
    vector<vector<double>> y(n, vector<double>(y0.size()));
    t[0] = t0;
    y[0] = y0;

    for (int i = 0; i < n - 1; ++i) {
        vector<double> k1 = deriv(t[i], y[i], eqs, params);

        vector<double> yk2(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk2[j] = y[i][j] + h * k1[j] / 2;
        }
        vector<double> k2 = deriv(t[i] + h / 2, yk2, eqs, params);

        vector<double> yk3(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk3[j] = y[i][j] + h * k2[j] / 2;
        }
        vector<double> k3 = deriv(t[i] + h / 2, yk3, eqs, params);

        vector<double> yk4(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) {
            yk4[j] = y[i][j] + h * k3[j];
        }
        vector<double> k4 = deriv(t[i] + h, yk4, eqs, params);

        for (size_t j = 0; j < y0.size(); ++j) {
            y[i + 1][j] = y[i][j] + (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
        }
        t[i + 1] = t[i] + h;
    }

    // Print results
    cout << setw(12) << "t";
    for (const auto& eq : eqs) {
        cout << " | " << setw(12) << eq.var + "_RK4";
    }
    cout << endl;

    for (int i = 0; i < n; ++i) {
        cout << fixed << setprecision(6) << setw(12) << t[i];
        for (size_t j = 0; j < y0.size(); ++j) {
            cout << " | " << setw(12) << y[i][j];
        }
        cout << endl;
    }
}

int main(int argc, char* argv[]) {
    try {
        string filename = "system.txt";
        if (argc > 1) {
            filename = argv[1];
        }

        SystemData sys = readSystem(filename);
        rk4(sys.equations, sys.params, sys.y0, sys.tStart, sys.tEnd, sys.dt);
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}