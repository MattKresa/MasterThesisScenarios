#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <iomanip>

struct DataPoint {
    std::vector<double> features;
    std::string label;
    int labelIndex;
};

class IrisKNN {
private:
    std::vector<DataPoint> trainData;
    std::vector<DataPoint> testData;
    std::map<std::string, int> labelToIndex;
    std::vector<std::string> indexToLabel;
    int k;

    // Calculate Euclidean distance between two feature vectors
    double calculateDistance(const std::vector<double>& a, const std::vector<double>& b) {
        double sum = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            double diff = a[i] - b[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }

    // Parse CSV line
    std::vector<std::string> parseCsvLine(const std::string& line) {
        std::vector<std::string> result;
        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) {
            // Remove leading/trailing whitespace
            cell.erase(0, cell.find_first_not_of(" \t"));
            cell.erase(cell.find_last_not_of(" \t") + 1);
            result.push_back(cell);
        }
        return result;
    }

    // Convert string labels to indices
    void processLabels(std::vector<DataPoint>& data) {
        int labelIndex = 0;
        for (auto& point : data) {
            if (labelToIndex.find(point.label) == labelToIndex.end()) {
                labelToIndex[point.label] = labelIndex;
                indexToLabel.push_back(point.label);
                labelIndex++;
            }
            point.labelIndex = labelToIndex[point.label];
        }
    }

public:
    IrisKNN(int kValue = 5) : k(kValue) {}

    // Load dataset from CSV file
    bool loadDataset(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        std::vector<DataPoint> allData;
        std::string line;
        bool isFirstLine = true;

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            // Skip header if it exists (check if first line contains non-numeric data)
            if (isFirstLine) {
                std::vector<std::string> firstRow = parseCsvLine(line);
                if (firstRow.size() > 0) {
                    // Try to parse first feature as number
                    try {
                        std::stod(firstRow[0]);
                    }
                    catch (...) {
                        // First line is header, skip it
                        isFirstLine = false;
                        continue;
                    }
                }
                isFirstLine = false;
            }

            std::vector<std::string> tokens = parseCsvLine(line);
            if (tokens.size() < 2) continue; // Need at least one feature and one label

            DataPoint point;
            // All columns except last are features
            for (size_t i = 0; i < tokens.size() - 1; ++i) {
                try {
                    point.features.push_back(std::stod(tokens[i]));
                }
                catch (...) {
                    std::cerr << "Error parsing feature: " << tokens[i] << std::endl;
                    return false;
                }
            }
            // Last column is label
            point.label = tokens.back();
            allData.push_back(point);
        }

        if (allData.empty()) {
            std::cerr << "Error: No data loaded from file" << std::endl;
            return false;
        }

        // Process labels
        processLabels(allData);

        // Split data (70% train, 30% test) with stratification
        stratifiedSplit(allData, 0.7, 42);

        std::cout << "Loaded " << allData.size() << " data points" << std::endl;
        std::cout << "Training set: " << trainData.size() << " points" << std::endl;
        std::cout << "Test set: " << testData.size() << " points" << std::endl;

        return true;
    }

    // Stratified train-test split
    void stratifiedSplit(std::vector<DataPoint>& data, double trainRatio, int seed) {
        std::mt19937 rng(seed);

        // Group data by class
        std::map<int, std::vector<DataPoint>> classGroups;
        for (const auto& point : data) {
            classGroups[point.labelIndex].push_back(point);
        }

        // Split each class proportionally
        for (auto& [classLabel, classData] : classGroups) {
            std::shuffle(classData.begin(), classData.end(), rng);

            size_t trainSize = static_cast<size_t>(classData.size() * trainRatio);

            for (size_t i = 0; i < trainSize; ++i) {
                trainData.push_back(classData[i]);
            }
            for (size_t i = trainSize; i < classData.size(); ++i) {
                testData.push_back(classData[i]);
            }
        }

        // Shuffle the final train and test sets
        std::shuffle(trainData.begin(), trainData.end(), rng);
        std::shuffle(testData.begin(), testData.end(), rng);
    }

    // Predict label for a single data point
    int predict(const std::vector<double>& features) {
        std::vector<std::pair<double, int>> distances;

        // Calculate distances to all training points
        for (const auto& trainPoint : trainData) {
            double dist = calculateDistance(features, trainPoint.features);
            distances.push_back({ dist, trainPoint.labelIndex });
        }

        // Sort by distance and take k nearest
        std::sort(distances.begin(), distances.end());

        // Count votes from k nearest neighbors
        std::map<int, int> votes;
        for (int i = 0; i < std::min(k, static_cast<int>(distances.size())); ++i) {
            votes[distances[i].second]++;
        }

        // Return class with most votes
        int bestClass = 0;
        int maxVotes = 0;
        for (const auto& [classLabel, voteCount] : votes) {
            if (voteCount > maxVotes) {
                maxVotes = voteCount;
                bestClass = classLabel;
            }
        }

        return bestClass;
    }

    // Evaluate model on test set
    void evaluate() {
        if (testData.empty()) {
            std::cerr << "No test data available" << std::endl;
            return;
        }

        std::vector<int> predictions;
        std::vector<int> actual;

        // Make predictions
        for (const auto& testPoint : testData) {
            int prediction = predict(testPoint.features);
            predictions.push_back(prediction);
            actual.push_back(testPoint.labelIndex);
        }

        // Calculate overall accuracy
        int correct = 0;
        for (size_t i = 0; i < predictions.size(); ++i) {
            if (predictions[i] == actual[i]) {
                correct++;
            }
        }

        double overallAccuracy = static_cast<double>(correct) / predictions.size();
        std::cout << "Overall accuracy: " << std::fixed << std::setprecision(2)
            << overallAccuracy << std::endl;

        // Calculate per-class accuracy (recall)
        std::cout << "\nAccuracy per class:" << std::endl;
        for (size_t classIdx = 0; classIdx < indexToLabel.size(); ++classIdx) {
            int classCorrect = 0;
            int classTotal = 0;

            for (size_t i = 0; i < actual.size(); ++i) {
                if (actual[i] == static_cast<int>(classIdx)) {
                    classTotal++;
                    if (predictions[i] == static_cast<int>(classIdx)) {
                        classCorrect++;
                    }
                }
            }

            double classAccuracy = classTotal > 0 ?
                static_cast<double>(classCorrect) / classTotal : 0.0;

            std::cout << "Accuracy for class " << indexToLabel[classIdx]
                << ": " << std::fixed << std::setprecision(2)
                << classAccuracy << std::endl;
        }

        // Print confusion matrix
        std::cout << "\nConfusion Matrix:" << std::endl;
        std::vector<std::vector<int>> confusionMatrix(indexToLabel.size(),
            std::vector<int>(indexToLabel.size(), 0));

        for (size_t i = 0; i < actual.size(); ++i) {
            confusionMatrix[actual[i]][predictions[i]]++;
        }

        std::cout << "Actual\\Predicted\t";
        for (const auto& label : indexToLabel) {
            std::cout << label << "\t";
        }
        std::cout << std::endl;

        for (size_t i = 0; i < indexToLabel.size(); ++i) {
            std::cout << indexToLabel[i] << "\t\t";
            for (size_t j = 0; j < indexToLabel.size(); ++j) {
                std::cout << confusionMatrix[i][j] << "\t";
            }
            std::cout << std::endl;
        }
    }
};

int main() {
    try {
        // Create KNN classifier with K=5
        IrisKNN knn(5);

        // Load dataset
        if (!knn.loadDataset("iris.csv")) {
            return 1;
        }

        // Evaluate model
        knn.evaluate();

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}