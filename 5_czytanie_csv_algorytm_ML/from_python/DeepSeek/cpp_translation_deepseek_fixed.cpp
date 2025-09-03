#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <dlib/svm.h>
//#include <dlib/data_io.h>

using namespace std;
//using namespace dlib;

// Function to trim whitespace from a string
string trim(const string& str) {
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first) return str;
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

// Function to split a string by a delimiter
std::vector<string> split(const string& s, char delimiter) {
    std::vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        token = trim(token);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// Function to load and parse CSV file
void load_iris_data(const string& filename,
    std::vector<std::vector<double>>& samples,
    std::vector<string>& labels) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }

    string line;
    bool header = true;

    while (getline(file, line)) {
        if (header) {  // Skip header row
            header = false;
            continue;
        }

        std::vector<string> tokens = split(line, ',');
        if (tokens.size() != 5) continue;  // iris.csv has 5 columns (4 features + label)

        std::vector<double> sample;
        for (size_t i = 0; i < 4; ++i) {  // First 4 columns are features
            sample.push_back(stod(tokens[i]));
        }
        samples.push_back(sample);
        labels.push_back(tokens[4]);  // Last column is label
    }
}

// KNN Classifier implementation
class KNeighborsClassifier {
private:
    int n_neighbors;
    vector<vector<double>> X_train;
    vector<int> y_train;

    double euclidean_distance(const vector<double>& a, const vector<double>& b) {
        double sum = 0.0;
        for (size_t i = 0; i < a.size(); ++i) {
            sum += pow(a[i] - b[i], 2);
        }
        return sqrt(sum);
    }

public:
    KNeighborsClassifier(int k = 5) : n_neighbors(k) {}

    void fit(const vector<vector<double>>& X, const vector<int>& y) {
        X_train = X;
        y_train = y;
    }

    int predict(const vector<double>& sample) {
        vector<pair<double, int>> distances;

        for (size_t i = 0; i < X_train.size(); ++i) {
            double dist = euclidean_distance(sample, X_train[i]);
            distances.emplace_back(dist, y_train[i]);
        }

        // Sort by distance
        sort(distances.begin(), distances.end(),
            [](const pair<double, int>& a, const pair<double, int>& b) {
                return a.first < b.first;
            });

        // Get k nearest neighbors
        map<int, int> classVotes;
        for (int i = 0; i < n_neighbors; ++i) {
            classVotes[distances[i].second]++;
        }

        // Find the class with most votes
        int predictedClass = -1;
        int maxVotes = -1;
        for (const auto& vote : classVotes) {
            if (vote.second > maxVotes) {
                maxVotes = vote.second;
                predictedClass = vote.first;
            }
        }

        return predictedClass;
    }

    vector<int> predict(const vector<vector<double>>& samples) {
        vector<int> predictions;
        for (const auto& sample : samples) {
            predictions.push_back(predict(sample));
        }
        return predictions;
    }
};

int main()
{
    try
    {
        // 1. Load the dataset
        std::vector<std::vector<double>> samples;
        std::vector<string> labels;
        load_iris_data("iris.csv", samples, labels);

        if (samples.empty()) {
            cout << "No data loaded!" << endl;
            return 1;
        }

        // 2. Encode labels as integers
        std::map<string, int> label_encoder;
        std::vector<int> encoded_labels;
        std::vector<string> class_names;

        // Create mapping from string labels to integers
        int class_id = 0;
        for (const auto& label : labels) {
            if (label_encoder.find(label) == label_encoder.end()) {
                label_encoder[label] = class_id;
                class_names.push_back(label);
                class_id++;
            }
            encoded_labels.push_back(label_encoder[label]);
        }

        // 3. Train-test split
        std::vector<size_t> indices(samples.size());
        iota(indices.begin(), indices.end(), 0);

        // Shuffle indices
        std::mt19937 g(42);
        shuffle(indices.begin(), indices.end(), g);

        size_t split_point = samples.size() * 0.7;
        std::vector<std::vector<double>> train_samples, test_samples;
        std::vector<int> train_labels, test_labels;

        for (size_t i = 0; i < indices.size(); ++i) {
            if (i < split_point) {
                train_samples.push_back(samples[indices[i]]);
                train_labels.push_back(encoded_labels[indices[i]]);
            }
            else {
                test_samples.push_back(samples[indices[i]]);
                test_labels.push_back(encoded_labels[indices[i]]);
            }
        }

        // 4. Train KNN model
        KNeighborsClassifier model(5);
        model.fit(train_samples, train_labels);

        // 5. Predictions
        vector<int> predictions = model.predict(test_samples);

        // 6. Overall accuracy
        int correct = 0;
        for (size_t i = 0; i < test_labels.size(); ++i) {
            if (predictions[i] == test_labels[i]) {
                correct++;
            }
        }
        double accuracy = static_cast<double>(correct) / test_labels.size();
        cout << "Overall accuracy: " << accuracy << endl;

        // 7. Accuracy per class
        for (size_t cls = 0; cls < class_names.size(); ++cls) {
            int class_correct = 0;
            int class_total = 0;

            for (size_t i = 0; i < test_labels.size(); ++i) {
                if (test_labels[i] == cls) {
                    class_total++;
                    if (predictions[i] == test_labels[i]) {
                        class_correct++;
                    }
                }
            }

            if (class_total > 0) {
                double class_accuracy = static_cast<double>(class_correct) / class_total;
                cout << "Accuracy for class " << class_names[cls] << ": " << class_accuracy << endl;
            }
        }
    }
    catch (exception& e)
    {
        cout << "Error: " << e.what() << endl;
    }

    return 0;
}