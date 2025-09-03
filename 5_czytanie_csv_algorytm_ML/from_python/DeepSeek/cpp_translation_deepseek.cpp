#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <dlib/svm.h>
#include <dlib/data_io.h>

using namespace std;
using namespace dlib;

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
        std::vector<std::vector<double>> train_samples, test_samples;
        std::vector<int> train_labels, test_labels;

        // Manually split the data (in practice, use a library or implement randomization)
        // Here we'll do a simple 70-30 split for demonstration
        size_t split_point = samples.size() * 0.7;
        train_samples.assign(samples.begin(), samples.begin() + split_point);
        test_samples.assign(samples.begin() + split_point, samples.end());
        train_labels.assign(encoded_labels.begin(), encoded_labels.begin() + split_point);
        test_labels.assign(encoded_labels.begin() + split_point, encoded_labels.end());

        // 4. Train KNN model
        typedef matrix<double, 4, 1> sample_type;
        typedef radial_basis_kernel<sample_type> kernel_type;

        krr_trainer<kernel_type> trainer;
        trainer.set_kernel(kernel_type(0.1));

        // Convert training data to dlib format
        std::vector<sample_type> dlib_train_samples;
        for (const auto& sample : train_samples) {
            sample_type s;
            for (size_t i = 0; i < sample.size(); ++i) {
                s(i) = sample[i];
            }
            dlib_train_samples.push_back(s);
        }

        decision_function<kernel_type> df = trainer.train(dlib_train_samples, train_labels);

        // 5. Predictions
        std::vector<int> predictions;
        for (const auto& sample : test_samples) {
            sample_type s;
            for (size_t i = 0; i < sample.size(); ++i) {
                s(i) = sample[i];
            }
            predictions.push_back(df(s));
        }

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