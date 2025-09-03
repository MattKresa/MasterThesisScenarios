#include <mlpack.hpp>

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>

using namespace mlpack;
using namespace mlpack::neighbor; // For KNN
using namespace mlpack::data;
using namespace arma;
using namespace std;

int main()
{
    // 1. Load dataset (features and labels manually)
    std::ifstream file("iris.csv");
    std::string line;
    std::vector<std::vector<double>> featureRows;
    std::vector<std::string> labelStrings;

    // Skip header
    std::getline(file, line);

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string item;
        std::vector<double> features;
        for (int i = 0; i < 4; ++i) {
            std::getline(ss, item, ',');
            features.push_back(std::stod(item));
        }
        featureRows.push_back(features);

        std::getline(ss, item, ',');
        labelStrings.push_back(item);
    }

    size_t n_samples = featureRows.size();
    size_t n_features = 4;

    arma::mat X(n_features, n_samples);
    arma::Row<size_t> y(n_samples);

    // Encode labels
    std::unordered_map<std::string, size_t> labelMap;
    std::unordered_map<size_t, std::string> reverseLabelMap;
    size_t currentLabel = 0;

    for (size_t i = 0; i < n_samples; ++i)
    {
        for (size_t j = 0; j < n_features; ++j)
            X(j, i) = featureRows[i][j];

        const std::string& label = labelStrings[i];
        if (labelMap.count(label) == 0)
        {
            labelMap[label] = currentLabel;
            reverseLabelMap[currentLabel] = label;
            currentLabel++;
        }
        y[i] = labelMap[label];
    }

    // 2. Train-test split (70% train, 30% test)
    mat X_train, X_test;
    Row<size_t> y_train, y_test;
    const double testRatio = 0.3;
    data::Split(X, y, X_train, X_test, y_train, y_test, testRatio, true);

    // 3. Train KNN model (k=5)
    KNN knn(X_train);
    Mat<size_t> neighbors;
    mat distances;
    knn.Search(X_test, 5, neighbors, distances);

    // 4. Majority vote for each test point
    Row<size_t> y_pred(y_test.n_elem);
    for (size_t i = 0; i < neighbors.n_cols; ++i)
    {
        std::map<size_t, size_t> count;
        for (size_t j = 0; j < neighbors.n_rows; ++j)
        {
            size_t label = y_train[neighbors(j, i)];
            count[label]++;
        }

        size_t maxCount = 0, bestLabel = 0;
        for (auto& kv : count)
        {
            if (kv.second > maxCount)
            {
                maxCount = kv.second;
                bestLabel = kv.first;
            }
        }
        y_pred[i] = bestLabel;
    }

    // 5. Overall accuracy
    size_t correct = arma::accu(y_pred == y_test);
    double accuracy = (double)correct / y_test.n_elem;
    std::cout << "Overall accuracy: " << accuracy << std::endl;

    // 6. Accuracy per class
    for (const auto& kv : reverseLabelMap)
    {
        size_t label = kv.first;
        std::string className = kv.second;

        size_t total = arma::accu(y_test == label);
        size_t correct_class = arma::accu((y_test == label) % (y_pred == label));

        std::cout << "Accuracy for class " << className << ": ";
        std::cout << std::fixed << std::setprecision(2)
            << (double)correct_class / total << std::endl;
    }

    return 0;
}
