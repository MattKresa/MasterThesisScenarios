#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <numeric>
#include <algorithm>
#include <map>
#include <cmath>
#include <random>

// A struct to represent a data point
struct DataPoint {
    std::vector<double> features;
    int label;
};

// A struct to hold a neighbor's distance and label
struct Neighbor {
    double distance;
    int label;
};

// Comparator for sorting neighbors by distance
bool compareNeighbors(const Neighbor& a, const Neighbor& b) {
    return a.distance < b.distance;
}

// Function to calculate Euclidean distance between two feature vectors
double euclideanDistance(const std::vector<double>& a, const std::vector<double>& b) {
    double sum = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        sum += pow(a[i] - b[i], 2);
    }
    return sqrt(sum);
}

// Function to load the Iris dataset from a CSV file
std::vector<DataPoint> load_iris_data(const std::string& filename) {
    std::vector<DataPoint> data_points;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return data_points;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;
        DataPoint dp;
        
        for (int i = 0; i < 4; ++i) {
            std::getline(ss, cell, ',');
            dp.features.push_back(std::stod(cell));
        }

        std::getline(ss, cell);
        if (cell == "setosa") {
            dp.label = 0;
        } else if (cell == "versicolor") {
            dp.label = 1;
        } else if (cell == "virginica") {
            dp.label = 2;
        }
        data_points.push_back(dp);
    }
    return data_points;
}

// Function to classify a single data point using the k-NN algorithm
int knn_classify(const DataPoint& test_point, 
                 const std::vector<DataPoint>& train_data, 
                 int k) {
    std::vector<Neighbor> neighbors;
    for (const auto& train_point : train_data) {
        double dist = euclideanDistance(test_point.features, train_point.features);
        neighbors.push_back({dist, train_point.label});
    }

    // Sort neighbors by distance
    std::sort(neighbors.begin(), neighbors.end(), compareNeighbors);

    // Get the k nearest neighbors
    std::map<int, int> label_counts;
    for (int i = 0; i < k; ++i) {
        label_counts[neighbors[i].label]++;
    }

    // Find the most common label among the k neighbors
    int max_count = 0;
    int predicted_label = -1;
    for (const auto& pair : label_counts) {
        if (pair.second > max_count) {
            max_count = pair.second;
            predicted_label = pair.first;
        }
    }
    return predicted_label;
}

int main() {
    // 1. Load dataset
    std::vector<DataPoint> all_data = load_iris_data("iris.csv");
    if (all_data.empty()) {
        return -1;
    }

    // 2. Train-test split (70% train, 30% test)
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(all_data.begin(), all_data.end(), g);

    size_t train_size = static_cast<size_t>(all_data.size() * 0.7);
    std::vector<DataPoint> train_data(all_data.begin(), all_data.begin() + train_size);
    std::vector<DataPoint> test_data(all_data.begin() + train_size, all_data.end());

    // 3. Set K and evaluate
    int k = 5;
    double correct_predictions = 0.0;
    std::map<int, double> correct_per_class;
    std::map<int, double> total_per_class;

    for (const auto& test_point : test_data) {
        int predicted_label = knn_classify(test_point, train_data, k);
        int true_label = test_point.label;

        total_per_class[true_label]++;
        if (predicted_label == true_label) {
            correct_predictions++;
            correct_per_class[true_label]++;
        }
    }

    // 4. Print overall accuracy
    double overall_accuracy = (correct_predictions / test_data.size()) * 100.0;
    std::cout << "Overall accuracy: " << overall_accuracy << "%" << std::endl;
    std::cout << "---" << std::endl;

    // 5. Print accuracy per class
    std::map<int, std::string> label_map = {{0, "setosa"}, {1, "versicolor"}, {2, "virginica"}};
    for (const auto& pair : label_map) {
        int class_label = pair.first;
        if (total_per_class.count(class_label) && total_per_class[class_label] > 0) {
            double class_accuracy = (correct_per_class[class_label] / total_per_class[class_label]) * 100.0;
            std::cout << "Accuracy for class " << pair.second << ": " << class_accuracy << "%" << std::endl;
        } else {
            std::cout << "No instances of class " << pair.second << " in test set." << std::endl;
        }
    }

    return 0;
}