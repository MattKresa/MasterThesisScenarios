#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <regex>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <functional>
#include <muParser.h>

struct SystemData {
    std::map<std::string, double> params;
    std::vector<double> y0;
    double t_start = 0.0;
    double t_end = 1.0;
    double dt = 0.1;
    std::vector<std::pair<std::string, std::string>> equations;
};

// Expression evaluator using muParser
double evaluateExpression(const std::string& expr, const std::map<std::string, double>& vars) {
    try {
        mu::Parser parser;
        
        // Set the expression
        parser.SetExpr(expr);
        
        // Define variables and their values
        std::map<std::string, double*> varPointers;
        std::map<std::string, double> localVars = vars; // Copy to ensure persistence
        
        for (auto& [name, value] : localVars) {
            varPointers[name] = &value;
            parser.DefineVar(name, varPointers[name]);
        }
        
        // Define common mathematical constants if not already defined
        if (vars.find("pi") == vars.end()) {
            localVars["pi"] = M_PI;
            varPointers["pi"] = &localVars["pi"];
            parser.DefineVar("pi", varPointers["pi"]);
        }
        if (vars.find("e") == vars.end()) {
            localVars["e"] = M_E;
            varPointers["e"] = &localVars["e"];
            parser.DefineVar("e", varPointers["e"]);
        }
        
        return parser.Eval();
    } catch (mu::Parser::exception_type& e) {
        std::cerr << "muParser Error: " << e.GetMsg() << std::endl;
        std::cerr << "Expression: " << expr << std::endl;
        return 0.0;
    }
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
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

// Parse array format like [1.0, 2.0, 3.0] or simple comma-separated values
std::vector<double> parseArray(const std::string& str) {
    std::vector<double> result;
    std::string cleaned = str;
    
    // Remove brackets if present
    cleaned = std::regex_replace(cleaned, std::regex("[\\[\\]]"), "");
    
    auto tokens = split(cleaned, ',');
    for (const auto& token : tokens) {
        if (!token.empty()) {
            try {
                result.push_back(std::stod(token));
            } catch (...) {
                std::cerr << "Warning: Could not parse array element: " << token << std::endl;
            }
        }
    }
    
    return result;
}

SystemData readSystem(const std::string& filename) {
    SystemData data;
    std::ifstream file(filename);
    std::string line;
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return data;
    }
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parameters (without derivatives)
        if (line.find('=') != std::string::npos && line.find('\'') == std::string::npos) {
            auto tokens = split(line, '=');
            if (tokens.size() >= 2) {
                std::string key = trim(tokens[0]);
                // Join remaining parts in case there are multiple '=' signs
                std::string val = tokens[1];
                for (size_t i = 2; i < tokens.size(); ++i) {
                    val += "=" + tokens[i];
                }
                val = trim(val);
                
                // Convert to lowercase for comparison
                std::string keyLower = key;
                std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), ::tolower);
                
                // Handle special parameters
                if (keyLower == "y0") {
                    data.y0 = parseArray(val);
                } else if (keyLower == "t_start") {
                    data.t_start = std::stod(val);
                } else if (keyLower == "t_end") {
                    data.t_end = std::stod(val);
                } else if (keyLower == "dt") {
                    data.dt = std::stod(val);
                } else {
                    // Try to parse as a mathematical expression
                    try {
                        std::map<std::string, double> emptyVars; // No variables for parameter evaluation
                        data.params[key] = evaluateExpression(val, emptyVars);
                    } catch (...) {
                        std::cerr << "Warning: Could not parse parameter: " << key << " = " << val << std::endl;
                    }
                }
            }
        }
        
        // Derivatives
        if (line.find('\'') != std::string::npos && line.find('=') != std::string::npos) {
            std::regex derivRegex("^([a-zA-Z_]\\w*)'\\s*=\\s*(.+)$");
            std::smatch match;
            if (std::regex_match(line, match, derivRegex)) {
                std::string var = trim(match[1].str());
                std::string expr = trim(match[2].str());
                data.equations.push_back({var, expr});
            }
        }
    }
    
    return data;
}

std::function<std::vector<double>(double, const std::vector<double>&)> 
makeDerivFunc(const std::vector<std::pair<std::string, std::string>>& equations, 
              const std::map<std::string, double>& params) {
    
    return [equations, params](double t, const std::vector<double>& y) -> std::vector<double> {
        std::map<std::string, double> localVars = params;
        localVars["t"] = t;
        
        // Set current variable values
        for (size_t i = 0; i < equations.size() && i < y.size(); ++i) {
            localVars[equations[i].first] = y[i];
        }
        
        std::vector<double> dydt;
        for (const auto& [var, expr] : equations) {
            double result = evaluateExpression(expr, localVars);
            dydt.push_back(result);
        }
        
        return dydt;
    };
}

std::pair<std::vector<double>, std::vector<std::vector<double>>> 
rk4(std::function<std::vector<double>(double, const std::vector<double>&)> f,
    const std::vector<double>& y0, double t0, double t_end, double h) {
    
    int n = static_cast<int>((t_end - t0) / h) + 1;
    std::vector<double> t(n);
    std::vector<std::vector<double>> y(n, std::vector<double>(y0.size()));
    
    // Initialize time vector
    for (int i = 0; i < n; ++i) {
        t[i] = t0 + i * h;
    }
    
    y[0] = y0;
    
    for (int i = 0; i < n - 1; ++i) {
        auto k1 = f(t[i], y[i]);
        
        std::vector<double> y_temp1(y[i].size());
        for (size_t j = 0; j < y[i].size(); ++j) {
            y_temp1[j] = y[i][j] + h * k1[j] / 2.0;
        }
        auto k2 = f(t[i] + h/2.0, y_temp1);
        
        std::vector<double> y_temp2(y[i].size());
        for (size_t j = 0; j < y[i].size(); ++j) {
            y_temp2[j] = y[i][j] + h * k2[j] / 2.0;
        }
        auto k3 = f(t[i] + h/2.0, y_temp2);
        
        std::vector<double> y_temp3(y[i].size());
        for (size_t j = 0; j < y[i].size(); ++j) {
            y_temp3[j] = y[i][j] + h * k3[j];
        }
        auto k4 = f(t[i] + h, y_temp3);
        
        for (size_t j = 0; j < y[i].size(); ++j) {
            y[i+1][j] = y[i][j] + (h/6.0) * (k1[j] + 2*k2[j] + 2*k3[j] + k4[j]);
        }
    }
    
    return {t, y};
}

int main() {
    // Read system from file
    SystemData data = readSystem("system.txt");
    
    if (data.equations.empty()) {
        std::cerr << "Error: No equations found in system file." << std::endl;
        return 1;
    }
    
    if (data.y0.empty()) {
        std::cerr << "Error: No initial conditions (y0) found." << std::endl;
        return 1;
    }
    
    std::cout << "System loaded successfully:" << std::endl;
    std::cout << "Parameters: ";
    for (const auto& [key, val] : data.params) {
        std::cout << key << "=" << val << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Initial conditions: ";
    for (double val : data.y0) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Time range: [" << data.t_start << ", " << data.t_end << "], dt=" << data.dt << std::endl;
    
    std::cout << "Equations:" << std::endl;
    for (const auto& [var, expr] : data.equations) {
        std::cout << "  " << var << "' = " << expr << std::endl;
    }
    std::cout << std::endl;
    
    // Create derivative function
    auto f = makeDerivFunc(data.equations, data.params);
    
    // Solve using RK4
    auto [t_rk4, y_rk4] = rk4(f, data.y0, data.t_start, data.t_end, data.dt);
    
    // Print results
    std::cout << std::setw(12) << "t";
    for (const auto& [var, _] : data.equations) {
        std::cout << " | " << std::setw(12) << (var + "_RK4");
    }
    std::cout << std::endl;
    
    // Print separator line
    std::cout << std::string(12, '-');
    for (size_t i = 0; i < data.equations.size(); ++i) {
        std::cout << "-+-" << std::string(12, '-');
    }
    std::cout << std::endl;
    
    for (size_t i = 0; i < t_rk4.size(); ++i) {
        std::cout << std::setw(12) << std::fixed << std::setprecision(6) << t_rk4[i];
        for (size_t j = 0; j < y_rk4[i].size() && j < data.equations.size(); ++j) {
            std::cout << " | " << std::setw(12) << std::fixed << std::setprecision(6) << y_rk4[i][j];
        }
        std::cout << std::endl;
    }
    
    return 0;
}