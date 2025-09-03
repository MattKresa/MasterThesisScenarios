#include <mlpack/core.hpp>
#include <mlpack/methods/neighbor_search/knn.hpp>
#include <mlpack/core/data/split_data.hpp>
#include <unordered_map>
#include <iostream>

using namespace mlpack;
using namespace mlpack::neighbor;
using namespace mlpack::data;
using namespace arma;
using namespace std;

int main()
{
    // 1. Load dataset
    mat dataset;
    DatasetInfo info;
    if (!Load("iris.csv", dataset, info, true)) {
        cerr << "Cannot load dataset." << endl;
        return 1;
    }

    // Assume last row is labels (species)
    rowvec labels = dataset.row(dataset.n_rows - 1);
    dataset.shed_row(dataset.n_rows - 1); // Remove labels from features

    // Map string labels to integers (like LabelEncoder)
    unordered_map<string, size_t> labelMap;
    vector<string> classNames;

    for (size_t i = 0; i < info.NumMappings(dataset.n_rows); ++i) {
        classNames.push_back(info.UnmapString(i, dataset.n_rows)); 
    }

    // 2. Train-test split
    mat X_train, X_test;
    rowvec y_train, y_test;

    Split(dataset, labels, X_train, X_test, y_train, y_test, 0.3, true);

    // 3. Train KNN
    KNN knn(X_train.t()); // mlpack expects columns as points
    arma::Mat<size_t> neighbors;
    arma::mat distances;

    knn.Search(X_test.t(), 5, neighbors, distances);

    // Majority vote prediction
    rowvec y_pred(y_test.n_elem, fill::zeros);

    for (size_t i = 0; i < neighbors.n_cols; ++i) {
        unordered_map<size_t, size_t> voteCount;
        for (size_t j = 0; j < neighbors.n_rows; ++j) {
            size_t neighborLabel = y_train(neighbors(j, i));
            voteCount[neighborLabel]++;
        }
        // Find max vote
        size_t bestLabel = 0, bestCount = 0;
        for (auto& kv : voteCount) {
            if (kv.second > bestCount) {
                bestLabel = kv.first;
                bestCount = kv.second;
            }
        }
        y_pred(i) = bestLabel;
    }

    // 4. Overall accuracy
    size_t correctTotal = 0;
    for (size_t i = 0; i < y_test.n_elem; ++i) {
        if (y_pred(i) == y_test(i)) correctTotal++;
    }
    double overallAcc = (double)correctTotal / y_test.n_elem;
    cout << "Overall accuracy: " << overallAcc << endl;

    // 5. Accuracy per class
    for (size_t c = 0; c < classNames.size(); ++c) {
        size_t correct = 0, total = 0;
        for (size_t i = 0; i < y_test.n_elem; ++i) {
            if (y_test(i) == c) {
                total++;
                if (y_pred(i) == c) correct++;
            }
        }
        cout << "Accuracy for class " << classNames[c]
             << ": " << (double)correct / total << endl;
    }

    return 0;
}
