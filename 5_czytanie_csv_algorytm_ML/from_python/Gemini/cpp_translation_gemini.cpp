#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <random>
#include <fstream> // For file operations
#include <sstream> // For string stream operations
#include <cmath>   // For std::sqrt

// --- Custom CSV Reader (with actual loading) ---
struct DataFrame {
    std::vector<std::vector<double>> data;
    std::vector<std::string> column_names;
    std::map<std::string, int> species_to_int;
    std::vector<std::string> int_to_species; // Stores unique class names in order

    void load_csv(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        std::string line;
        
        // Read header line
        if (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string cell;
            while (std::getline(ss, cell, ',')) {
                column_names.push_back(cell);
            }
        } else {
            throw std::runtime_error("CSV file is empty or missing header.");
        }

        int species_col_idx = -1;
        for (size_t i = 0; i < column_names.size(); ++i) {
            if (column_names[i] == "species") { // Assuming "species" is the column name for labels
                species_col_idx = i;
                break;
            }
        }
        if (species_col_idx == -1) {
            throw std::runtime_error("Species column not found in CSV. Expected 'species'.");
        }

        // Read data lines
        int next_int_val = 0;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string cell;
            std::vector<double> row_data;
            std::string current_species_name;
            int current_col = 0;

            while (std::getline(ss, cell, ',')) {
                if (current_col == species_col_idx) {
                    current_species_name = cell;
                    // Placeholder for numerical species value, will be updated after mapping
                    row_data.push_back(-1.0);
                } else {
                    try {
                        row_data.push_back(std::stod(cell));
                    } catch (const std::invalid_argument& e) {
                        throw std::runtime_error("Invalid number format in CSV: " + cell);
                    } catch (const std::out_of_range& e) {
                        throw std::runtime_error("Number out of range in CSV: " + cell);
                    }
                }
                current_col++;
            }

            // Map species name to an integer
            if (species_to_int.find(current_species_name) == species_to_int.end()) {
                species_to_int[current_species_name] = next_int_val;
                int_to_species.push_back(current_species_name);
                next_int_val++;
            }
            // Update the placeholder with the actual integer value for species
            row_data[species_col_idx] = static_cast<double>(species_to_int[current_species_name]);

            data.push_back(row_data);
        }

        file.close();
    }

    std::vector<std::vector<double>> drop_column(const std::string& col_name) {
        int col_idx = -1;
        for (size_t i = 0; i < column_names.size(); ++i) {
            if (column_names[i] == col_name) {
                col_idx = i;
                break;
            }
        }
        if (col_idx == -1) {
            throw std::runtime_error("Column not found: " + col_name);
        }

        std::vector<std::vector<double>> new_data;
        new_data.reserve(data.size()); // Pre-allocate memory
        for (const auto& row : data) {
            std::vector<double> new_row;
            new_row.reserve(row.size() - 1); // Pre-allocate memory
            for (size_t i = 0; i < row.size(); ++i) {
                if (static_cast<int>(i) != col_idx) {
                    new_row.push_back(row[i]);
                }
            }
            new_data.push_back(new_row);
        }
        return new_data;
    }

    std::vector<int> get_column_as_int(const std::string& col_name) {
        int col_idx = -1;
        for (size_t i = 0; i < column_names.size(); ++i) {
            if (column_names[i] == col_name) {
                col_idx = i;
                break;
            }
        }
        if (col_idx == -1) {
            throw std::runtime_error("Column not found: " + col_name);
        }

        std::vector<int> column_data;
        column_data.reserve(data.size()); // Pre-allocate memory
        for (const auto& row : data) {
            column_data.push_back(static_cast<int>(row[col_idx]));
        }
        return column_data;
    }

    std::vector<std::string> get_unique_species_names() {
        return int_to_species;
    }
};

// --- Custom KNN Implementation (simplified) ---
class KNeighborsClassifier {
public:
    KNeighborsClassifier(int n_neighbors) : k(n_neighbors) {}

    void fit(const std::vector<std::vector<double>>& X_train, const std::vector<int>& y_train) {
        this->X_train = X_train;
        this->y_train = y_train;
    }

    std::vector<int> predict(const std::vector<std::vector<double>>& X_test) {
        std::vector<int> predictions;
        predictions.reserve(X_test.size()); // Pre-allocate memory
        for (const auto& test_point : X_test) {
            predictions.push_back(predict_single(test_point));
        }
        return predictions;
    }

private:
    int k;
    std::vector<std::vector<double>> X_train;
    std::vector<int> y_train;

    double euclidean_distance(const std::vector<double>& p1, const std::vector<double>& p2) {
        double sum_sq = 0.0;
        for (size_t i = 0; i < p1.size(); ++i) {
            sum_sq += (p1[i] - p2[i]) * (p1[i] - p2[i]);
        }
        return std::sqrt(sum_sq);
    }

    int predict_single(const std::vector<double>& test_point) {
        std::vector<std::pair<double, int>> distances; // distance, label
        distances.reserve(X_train.size()); // Pre-allocate memory

        for (size_t i = 0; i < X_train.size(); ++i) {
            distances.push_back({euclidean_distance(test_point, X_train[i]), y_train[i]});
        }

        std::sort(distances.begin(), distances.end());

        std::map<int, int> label_counts;
        for (int i = 0; i < k && i < distances.size(); ++i) {
            label_counts[distances[i].second]++;
        }

        int majority_label = -1;
        int max_count = -1;
        // Find the label with the highest count (mode)
        for (const auto& pair : label_counts) {
            if (pair.second > max_count) {
                max_count = pair.second;
                majority_label = pair.first;
            }
        }
        return majority_label;
    }
};

// --- Helper Functions ---
void train_test_split(
    const std::vector<std::vector<double>>& X,
    const std::vector<int>& y,
    double test_size,
    int random_state,
    std::vector<std::vector<double>>& X_train,
    std::vector<std::vector<double>>& X_test,
    std::vector<int>& y_train,
    std::vector<int>& y_test
) {
    std::vector<size_t> indices(X.size());
    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, 2, ...

    std::mt19937 g(random_state);
    std::shuffle(indices.begin(), indices.end(), g); // Shuffle indices

    size_t num_test = static_cast<size_t>(X.size() * test_size);
    // Ensure at least one element in train if test_size is very large
    size_t num_train = X.size() - num_test;
    if (num_train == 0 && X.size() > 0) { // Handle case where test_size is 1.0 or very close
        num_train = 1;
        num_test = X.size() - 1;
    } else if (X.size() == 0) { // Handle empty input
        num_train = 0;
        num_test = 0;
    }
    
    X_train.reserve(num_train);
    y_train.reserve(num_train);
    X_test.reserve(num_test);
    y_test.reserve(num_test);

    // This simplified split does NOT implement 'stratify' directly.
    // Stratification requires more complex logic to ensure proportional
    // representation of classes in both train and test sets.
    for (size_t i = 0; i < num_train; ++i) {
        X_train.push_back(X[indices[i]]);
        y_train.push_back(y[indices[i]]);
    }

    for (size_t i = num_train; i < X.size(); ++i) {
        X_test.push_back(X[indices[i]]);
        y_test.push_back(y[indices[i]]);
    }
}

double accuracy_score(const std::vector<int>& y_true, const std::vector<int>& y_pred) {
    if (y_true.empty()) return 0.0; // Handle empty input to prevent division by zero

    int correct_count = 0;
    for (size_t i = 0; i < y_true.size(); ++i) {
        if (y_true[i] == y_pred[i]) {
            correct_count++;
        }
    }
    return static_cast<double>(correct_count) / y_true.size();
}


// --- Main Program ---
int main() {
    try {
        // 1. Load the dataset
        DataFrame df;
        df.load_csv("iris.csv"); // Make sure 'iris.csv' is in the same directory

        // 2. Separate features and labels
        std::vector<std::vector<double>> X = df.drop_column("species");
        std::vector<int> y = df.get_column_as_int("species");
        std::vector<std::string> class_names = df.get_unique_species_names();

        // 3. Train-test split
        std::vector<std::vector<double>> X_train, X_test;
        std::vector<int> y_train, y_test;
        train_test_split(X, y, 0.3, 42, X_train, X_test, y_train, y_test);

        // 4. Train KNN model
        KNeighborsClassifier model(5);
        model.fit(X_train, y_train);

        // 5. Predictions
        std::vector<int> y_pred = model.predict(X_test);

        // 6. Overall accuracy
        std::cout << "Overall accuracy: " << accuracy_score(y_test, y_pred) << std::endl;

        // 7. Accuracy per class
        for (size_t i = 0; i < class_names.size(); ++i) {
            int correct = 0;
            int total = 0;
            for (size_t j = 0; j < y_test.size(); ++j) {
                if (y_test[j] == static_cast<int>(i)) {
                    total++;
                    if (y_pred[j] == static_cast<int>(i)) {
                        correct++;
                    }
                }
            }
            // Handle division by zero for classes not present in the test set
            double accuracy_cls = (total == 0) ? 0.0 : static_cast<double>(correct) / total;
            printf("Accuracy for class %s: %.2f\n", class_names[i].c_str(), accuracy_cls);
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}