#include <mlpack/core.hpp>
#include <mlpack/methods/neighbor_search/knn.hpp>
#include <armadillo>
#include <iostream>
#include <vector>
#include <iomanip>
#include <unordered_map>

using namespace mlpack::neighbor;
using namespace mlpack;
using namespace arma;
using namespace std;

int main() {
    // 1. Load dataset
    mat dataset; // Armadillo matrix: columns = points, rows = features
    data::DatasetInfo info; // stores info about each dimension (numeric/nominal)
    data::Load("iris.csv", dataset, info, true); // transpose automatically if needed

    // Convert last column (species) to numeric labels
    Row<size_t> labels(dataset.n_cols); 
    for (size_t i = 0; i < dataset.n_cols; ++i) {
        // Last row is species name, so we map to numeric ID
        labels[i] = info.MapString<size_t>(dataset(dataset.n_rows - 1, i), dataset.n_rows - 1);
    }

    // Remove label row from dataset (now only features remain)
    dataset.shed_row(dataset.n_rows - 1);

    // 2. Train-test split (70% train, 30% test)
    arma::mat trainData, testData;
    arma::Row<size_t> trainLabels, testLabels;
    data::Split(dataset, labels, trainData, testData, trainLabels, testLabels, 0.3, true);

    // 3. Train KNN model (K=5)
    KNN knn(trainData);

    // 4. Find nearest neighbors for test set
    arma::Mat<size_t> neighbors;
    arma::mat distances;
    knn.Search(testData, 5, neighbors, distances);

    // Majority vote classification
    Row<size_t> predictions(testLabels.n_elem);
    for (size_t i = 0; i < neighbors.n_cols; ++i) {
        unordered_map<size_t, int> voteCount;
        for (size_t k = 0; k < neighbors.n_rows; ++k) {
            size_t label = trainLabels[neighbors(k, i)];
            voteCount[label]++;
        }
        size_t bestLabel = voteCount.begin()->first;
        int bestCount = 0;
        for (auto &p : voteCount) {
            if (p.second > bestCount) {
                bestCount = p.second;
                bestLabel = p.first;
            }
        }
        predictions[i] = bestLabel;
    }

    // 5. Overall accuracy
    int correct = 0;
    for (size_t i = 0; i < predictions.n_elem; ++i) {
        if (predictions[i] == testLabels[i]) correct++;
    }
    double accuracy = static_cast<double>(correct) / predictions.n_elem;
    cout << "Overall accuracy: " << fixed << setprecision(2) << accuracy << endl;

    // 6. Accuracy per class
    unordered_map<size_t, pair<int,int>> perClass; // label -> (correct,total)
    for (size_t i = 0; i < predictions.n_elem; ++i) {
        perClass[testLabels[i]].second++; // total
        if (predictions[i] == testLabels[i]) {
            perClass[testLabels[i]].first++; // correct
        }
    }

    for (auto &entry : perClass) {
        size_t classLabel = entry.first;
        double classAcc = static_cast<double>(entry.second.first) / entry.second.second;
        cout << "Accuracy for class " << info.UnmapString(classLabel, dataset.n_rows)
             << ": " << fixed << setprecision(2) << classAcc << endl;
    }

    return 0;
}
