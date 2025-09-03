#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <regex>
#include <iomanip>
#include <cmath>
#include <functional>

#include "muParser.h"

using namespace std;
using namespace mu;

// Global vars
Parser p;
map<string, double*> parserVars;

// --- read_system ---
tuple<map<string, double>, vector<double>, double, double, double, vector<pair<string, string>>> read_system(const string& filename) {
    map<string, double> params;
    vector<double> y0;
    double t_start = 0.0;
    double t_end = 1.0;
    double dt = 0.1;
    vector<pair<string, string>> equations;

    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }

    string line;
    while (getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line.find("=") != string::npos && line.find("'") == string::npos) {
            size_t eqPos = line.find("=");
            string key = line.substr(0, eqPos);
            string valStr = line.substr(eqPos + 1);

            key.erase(0, key.find_first_not_of(" \t\n\r\f\v"));
            key.erase(key.find_last_not_of(" \t\n\r\f\v") + 1);
            valStr.erase(0, valStr.find_first_not_of(" \t\n\r\f\v"));
            valStr.erase(valStr.find_last_not_of(" \t\n\r\f\v") + 1);

            if (key == "y0") {
                string num_str;
                string val_str = valStr.substr(1, valStr.size() - 2);
                stringstream ss(val_str);
                while (getline(ss, num_str, ',')) {
                    y0.push_back(stod(num_str));
                }
            }
            else if (key == "t_start") {
                t_start = stod(valStr);
            }
            else if (key == "t_end") {
                t_end = stod(valStr);
            }
            else if (key == "dt") {
                dt = stod(valStr);
            }
            else {
                mu::Parser parser;
                parser.SetExpr(valStr);
                params[key] = parser.Eval();
            }
        }

        if (line.find("'") != string::npos && line.find("=") != string::npos) {
            regex deriv_regex(R"(^([a-zA-Z_]\w*)\s*'\s*=\s*(.+)$)");
            smatch matches;
            if (regex_match(line, matches, deriv_regex)) {
                string var = matches[1].str();
                string expr = matches[2].str();
                equations.push_back({ var, expr });
            }
        }
    }
    return make_tuple(params, y0, t_start, t_end, dt, equations);
}

// --- deriv_func ---
vector<double> deriv_func(double t, const vector<double>& y, const vector<pair<string, string>>& eq_pairs) {
    vector<double> dydt(y.size());
    *parserVars["t"] = t;

    for (size_t i = 0; i < y.size(); ++i) {
        *parserVars[eq_pairs[i].first] = y[i];
    }

    // Eval of each expression
    try {
        for (size_t i = 0; i < eq_pairs.size(); ++i) {
            p.SetExpr(eq_pairs[i].second);
            dydt[i] = p.Eval();
        }
    }
    catch (Parser::exception_type& e) {
        cerr << "muParser error: " << e.GetMsg() << endl;
    }
    return dydt;
}

// --- rk4 ---
vector<vector<double>> rk4(function<vector<double>(double, const vector<double>&, const vector<pair<string, string>>&)> f,
    const vector<double>& y0, double t0, double t_end, double h, const vector<pair<string, string>>& eq_pairs) {
    int n = static_cast<int>((t_end - t0) / h) + 1;
    vector<vector<double>> y(n, vector<double>(y0.size()));

    y[0] = y0;

    for (int i = 0; i < n - 1; ++i) {
        double current_t = t0 + i * h;
        vector<double> k1 = f(current_t, y[i], eq_pairs);

        vector<double> y_k2(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) y_k2[j] = y[i][j] + h * k1[j] / 2.0;
        vector<double> k2 = f(current_t + h / 2.0, y_k2, eq_pairs);

        vector<double> y_k3(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) y_k3[j] = y[i][j] + h * k2[j] / 2.0;
        vector<double> k3 = f(current_t + h / 2.0, y_k3, eq_pairs);

        vector<double> y_k4(y0.size());
        for (size_t j = 0; j < y0.size(); ++j) y_k4[j] = y[i][j] + h * k3[j];
        vector<double> k4 = f(current_t + h, y_k4, eq_pairs);

        for (size_t j = 0; j < y0.size(); ++j) {
            y[i + 1][j] = y[i][j] + (h / 6.0) * (k1[j] + 2.0 * k2[j] + 2.0 * k3[j] + k4[j]);
        }
    }

    return y;
}

// --- main ---
int main() {
    try {
        auto system_data = read_system("system.txt");
        map<string, double> params = get<0>(system_data);
        vector<double> y0 = get<1>(system_data);
        double t_start = get<2>(system_data);
        double t_end = get<3>(system_data);
        double dt = get<4>(system_data);
        vector<pair<string, string>> eq_pairs = get<5>(system_data);

        double t = 0;
        parserVars["t"] = &t;
        p.DefineVar("t", parserVars["t"]);

        for (const auto& pair : params) {
            p.DefineConst(pair.first, pair.second);
        }

        for (const auto& eq : eq_pairs) {
            parserVars[eq.first] = new double(0);
            p.DefineVar(eq.first, parserVars[eq.first]);
        }

        // RK4 method
        vector<vector<double>> y_rk4 = rk4(
            [&](double current_t, const vector<double>& current_y, const vector<pair<string, string>>& pairs) {
                return deriv_func(current_t, current_y, pairs);
            },
            y0, t_start, t_end, dt, eq_pairs
        );

        cout << setw(12) << "t";
        for (const auto& eq : eq_pairs) {
            cout << " | " << setw(12) << (eq.first + "_RK4");
        }
        cout << endl;

        int n = static_cast<int>((t_end - t_start) / dt) + 1;
        for (int i = 0; i < n; ++i) {
            cout << fixed << setprecision(6) << setw(12) << t_start + i * dt;
            for (const auto& val : y_rk4[i]) {
                cout << " | " << setw(12) << val;
            }
            cout << endl;
        }

    }
    catch (exception& e) {
        cerr << "An error occurred: " << e.what() << endl;
        return 1;
    }

    return 0;
}