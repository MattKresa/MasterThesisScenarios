#define _USE_MATH_DEFINES
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <iomanip>
#include <regex>
#include <stdexcept>
#include "muParser.h"

using namespace std;

// Helper functions
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

struct SystemData {
    map<string, double> params;
    vector<double> y0;
    double t_start = 0;
    double t_end = 1;
    double dt = 0.1;
    vector<pair<string, string>> equations;
};

SystemData read_system(const string& filename) {
    SystemData data;
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }

    string line;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        // Parameters (without derivatives)
        if (line.find('=') != string::npos && line.find('\'') == string::npos) {
            auto parts = split(line, '=');
            if (parts.size() != 2) continue;
            string key = trim(parts[0]);
            string val = trim(parts[1]);

            try {
                // Simple expression evaluation with muParser
                mu::Parser parser;
                parser.SetExpr(val);
                data.params[key] = parser.Eval();
            }
            catch (...) {
                data.params[key] = 0;
            }
        }

        // Initial conditions and time
        if (line.find("y0") == 0 || line.find("Y0") == 0) {
            string val_str = trim(split(line, '=')[1]);
            val_str = val_str.substr(1, val_str.size() - 2); // Remove brackets
            auto vals = split(val_str, ',');
            for (const auto& v : vals) {
                data.y0.push_back(stod(trim(v)));
            }
        }
        else if (line.find("t_start") == 0 || line.find("T_START") == 0) {
            data.t_start = stod(trim(split(line, '=')[1]));
        }
        else if (line.find("t_end") == 0 || line.find("T_END") == 0) {
            data.t_end = stod(trim(split(line, '=')[1]));
        }
        else if (line.find("dt") == 0 || line.find("DT") == 0) {
            data.dt = stod(trim(split(line, '=')[1]));
        }

        // Derivatives
        if (line.find('\'') != string::npos && line.find('=') != string::npos) {
            regex pattern(R"(^([a-zA-Z_]\w*)'\s*=\s*(.+)$)");
            smatch matches;
            if (regex_search(line, matches, pattern)) {
                string var = trim(matches[1].str());
                string expr = trim(matches[2].str());
                data.equations.emplace_back(var, expr);
            }
        }
    }

    return data;
}

class DerivativeFunction {
    vector<pair<string, string>> equations;
    map<string, double> params;
    mu::Parser parser;

public:
    DerivativeFunction(const vector<pair<string, string>>& eqs, const map<string, double>& prms)
        : equations(eqs), params(prms) {
        // Define constants in parser
        parser.DefineConst("pi", M_PI);
        parser.DefineConst("e", M_E);
    }

    vector<double> operator()(double t, const vector<double>& y) {
        // Set up parser with current variables
        for (const auto& p : params) {
            parser.DefineVar(p.first, &const_cast<double&>(p.second));
        }

        // Define time variable
        double t_var = t;
        parser.DefineVar("t", &t_var);

        // Define state variables
        vector<double> y_vars(y.size());
        for (size_t i = 0; i < equations.size(); ++i) {
            y_vars[i] = y[i];
            parser.DefineVar(equations[i].first, &y_vars[i]);
        }

        vector<double> dydt;
        for (const auto& eq : equations) {
            try {
                parser.SetExpr(eq.second);
                dydt.push_back(parser.Eval());
            }
            catch (mu::Parser::exception_type& e) {
                cerr << "Error evaluating equation: " << e.GetMsg() << endl;
                dydt.push_back(0);
            }
        }
        return dydt;
    }
};

vector<pair<double, vector<double>>> rk4(
    DerivativeFunction f,
    const vector<double>& y0,
    double t0,
    double t_end,
    double h) {

    int n = static_cast<int>((t_end - t0) / h) + 1;
    vector<pair<double, vector<double>>> result;
    result.reserve(n);

    vector<double> y = y0;
    double t = t0;
    result.emplace_back(t, y);

    for (int i = 0; i < n - 1; ++i) {
        auto k1 = f(t, y);

        vector<double> y2;
        for (size_t j = 0; j < y.size(); ++j) {
            y2.push_back(y[j] + h * k1[j] / 2);
        }
        auto k2 = f(t + h / 2, y2);

        vector<double> y3;
        for (size_t j = 0; j < y.size(); ++j) {
            y3.push_back(y[j] + h * k2[j] / 2);
        }
        auto k3 = f(t + h / 2, y3);

        vector<double> y4;
        for (size_t j = 0; j < y.size(); ++j) {
            y4.push_back(y[j] + h * k3[j]);
        }
        auto k4 = f(t + h, y4);

        for (size_t j = 0; j < y.size(); ++j) {
            y[j] += (h / 6) * (k1[j] + 2 * k2[j] + 2 * k3[j] + k4[j]);
        }
        t += h;

        result.emplace_back(t, y);
    }

    return result;
}

int main() {
    try {
        auto data = read_system("system.txt");
        DerivativeFunction f(data.equations, data.params);

        auto result = rk4(f, data.y0, data.t_start, data.t_end, data.dt);

        // Print header
        cout << setw(12) << "t";
        for (const auto& eq : data.equations) {
            cout << " | " << setw(12) << eq.first + "_RK4";
        }
        cout << endl;

        // Print results
        for (const auto& point : result) {
            cout << fixed << setprecision(6) << setw(12) << point.first;
            for (double val : point.second) {
                cout << " | " << setw(12) << val;
            }
            cout << endl;
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}