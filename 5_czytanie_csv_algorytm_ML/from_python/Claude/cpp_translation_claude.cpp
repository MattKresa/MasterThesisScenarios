#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <cmath>
#include <random>
#include <iomanip>

struct DataPoint {
    std::vector<double> features;
    int label;
    std::string class_name;
};

class KNNClassifier {
private:
    std::vector<DataPoint> training_data;
    int k;
    
    double euclideanDistance(const std::vector<double>& a, const std::vector<double>& b) {
        double sum = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            sum += (a[i] - b[i]) * (a[i] - b[i]);
        }
        return std::sqrt(sum);
    }
    
public:
    KNNClassifier(int neighbors = 5) : k(neighbors) {}
    
    void fit(const std::vector<DataPoint>& train_data) {
        training_data = train_data;
    }
    
    int predict(const std::vector<double>& features) {
        std::vector<std::pair<double, int>> distances;
        
        // Calculate distances to all training points
        for (const auto& point : training_data) {
            double dist = euclideanDistance(features, point.features);
            distances.push_back({dist, point.label});
        }
        
        // Sort by distance
        std::sort(distances.begin(), distances.end());
        
        // Count votes from k nearest neighbors
        std::map<int, int> votes;
        for (int i = 0; i < k && i < distances.size(); ++i) {
            votes[distances[i].second]++;
        }
        
        // Return class with most votes
        int best_class = -1;
        int max_votes = 0;
        for (const auto& vote : votes) {
            if (vote.second > max_votes) {
                max_votes = vote.second;
                best_class = vote.first;
            }
        }
        
        return best_class;
    }
    
    std::vector<int> predict(const std::vector<DataPoint>& test_data) {
        std::vector<int> predictions;
        for (const auto& point : test_data) {
            predictions.push_back(predict(point.features));
        }
        return predictions;
    }
};

class LabelEncoder {
private:
    std::map<std::string, int> class_to_label;
    std::map<int, std::string> label_to_class;
    int next_label = 0;
    
public:
    std::vector<int> fit_transform(const std::vector<std::string>& classes) {
        std::vector<int> labels;
        
        for (const std::string& cls : classes) {
            if (class_to_label.find(cls) == class_to_label.end()) {
                class_to_label[cls] = next_label;
                label_to_class[next_label] = cls;
                next_label++;
            }
            labels.push_back(class_to_label[cls]);
        }
        
        return labels;
    }
    
    std::vector<std::string> getClassNames() {
        std::vector<std::string> names;
        for (const auto& pair : label_to_class) {
            names.push_back(pair.second);
        }
        return names;
    }
};

std::vector<DataPoint> loadIrisDataset(const std::string& filename) {
    std::vector<DataPoint> data;
    std::ifstream file(filename);
    std::string line;
    
    // Skip header if present
    if (std::getline(file, line)) {
        // Check if first line looks like header
        if (line.find("sepal") != std::string::npos || 
            line.find("petal") != std::string::npos ||
            line.find("species") != std::string::npos) {
            // Skip header line
        } else {
            // Process first line as data
            file.seekg(0);
        }
    }
    
    std::vector<std::string> species_list;
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        DataPoint point;
        
        // Read features (assuming 4 features for iris dataset)
        for (int i = 0; i < 4; ++i) {
            if (std::getline(ss, token, ',')) {
                point.features.push_back(std::stod(token));
            }
        }
        
        // Read species
        if (std::getline(ss, token, ',')) {
            point.class_name = token;
            species_list.push_back(token);
        }
        
        data.push_back(point);
    }
    
    // Encode labels
    LabelEncoder encoder;
    std::vector<int> labels = encoder.fit_transform(species_list);
    
    for (size_t i = 0; i < data.size(); ++i) {
        data[i].label = labels[i];
    }
    
    return data;
}

void trainTestSplit(const std::vector<DataPoint>& data, 
                    std::vector<DataPoint>& train, 
                    std::vector<DataPoint>& test,
                    double test_size = 0.3, 
                    int random_state = 42) {
    
    // Stratified split by class
    std::map<int, std::vector<DataPoint>> class_data;
    
    // Group by class
    for (const auto& point : data) {
        class_data[point.label].push_back(point);
    }
    
    std::mt19937 rng(random_state);
    
    // Split each class proportionally
    for (auto& pair : class_data) {
        std::vector<DataPoint>& class_points = pair.second;
        std::shuffle(class_points.begin(), class_points.end(), rng);
        
        size_t test_count = static_cast<size_t>(class_points.size() * test_size);
        
        for (size_t i = 0; i < test_count; ++i) {
            test.push_back(class_points[i]);
        }
        
        for (size_t i = test_count; i < class_points.size(); ++i) {
            train.push_back(class_points[i]);
        }
    }
    
    // Shuffle final sets
    std::shuffle(train.begin(), train.end(), rng);
    std::shuffle(test.begin(), test.end(), rng);
}

double calculateAccuracy(const std::vector<int>& y_true, const std::vector<int>& y_pred) {
    int correct = 0;
    for (size_t i = 0; i < y_true.size(); ++i) {
        if (y_true[i] == y_pred[i]) {
            correct++;
        }
    }
    return static_cast<double>(correct) / y_true.size();
}

int main() {
    // 1. Load the dataset
    std::vector<DataPoint> df = loadIrisDataset("iris.csv");
    
    if (df.empty()) {
        std::cerr << "Error: Could not load iris.csv" << std::endl;
        return 1;
    }
    
    // Get unique class names
    std::vector<std::string> class_names;
    std::map<std::string, bool> seen;
    for (const auto& point : df) {
        if (seen.find(point.class_name) == seen.end()) {
            class_names.push_back(point.class_name);
            seen[point.class_name] = true;
        }
    }
    
    // 3. Train-test split
    std::vector<DataPoint> X_train, X_test;
    trainTestSplit(df, X_train, X_test, 0.3, 42);
    
    // Extract true labels for test set
    std::vector<int> y_test;
    for (const auto& point : X_test) {
        y_test.push_back(point.label);
    }
    
    // 4. Train KNN model
    KNNClassifier model(5);
    model.fit(X_train);
    
    // 5. Predictions
    std::vector<int> y_pred = model.predict(X_test);
    
    // 6. Overall accuracy
    double overall_accuracy = calculateAccuracy(y_test, y_pred);
    std::cout << "Overall accuracy: " << std::fixed << std::setprecision(4) 
              << overall_accuracy << std::endl;
    
    // 7. Accuracy per class
    for (size_t cls_idx = 0; cls_idx < class_names.size(); ++cls_idx) {
        int correct = 0;
        int total = 0;
        
        for (size_t i = 0; i < y_test.size(); ++i) {
            if (y_test[i] == static_cast<int>(cls_idx)) {
                total++;
                if (y_pred[i] == y_test[i]) {
                    correct++;
                }
            }
        }
        
        if (total > 0) {
            double class_accuracy = static_cast<double>(correct) / total;
            std::cout << "Accuracy for class " << class_names[cls_idx] 
                      << ": " << std::fixed << std::setprecision(2) 
                      << class_accuracy << std::endl;
        }
    }
    
    return 0;
}