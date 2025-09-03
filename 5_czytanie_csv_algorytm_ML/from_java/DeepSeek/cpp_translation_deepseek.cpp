#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <map>
#include <random>
#include <iomanip>

using namespace std;

// Structure to hold a data point
struct DataPoint {
    vector<double> features;
    string label;
    
    double distance(const DataPoint& other) const {
        double sum = 0.0;
        for (size_t i = 0; i < features.size(); i++) {
            sum += pow(features[i] - other.features[i], 2);
        }
        return sqrt(sum);
    }
};

// KNN Classifier class
class KNNClassifier {
private:
    vector<DataPoint> trainingData;
    int k;
    
public:
    KNNClassifier(int k) : k(k) {}
    
    void train(const vector<DataPoint>& data) {
        trainingData = data;
    }
    
    string predict(const DataPoint& testPoint) const {
        // Calculate distances to all training points
        vector<pair<double, string>> distances;
        for (const auto& trainPoint : trainingData) {
            distances.emplace_back(trainPoint.distance(testPoint), trainPoint.label);
        }
        
        // Sort by distance
        sort(distances.begin(), distances.end(), 
            [](const auto& a, const auto& b) { return a.first < b.first; });
        
        // Count votes among k nearest neighbors
        map<string, int> votes;
        for (int i = 0; i < k && i < distances.size(); i++) {
            votes[distances[i].second]++;
        }
        
        // Return the label with most votes
        return max_element(votes.begin(), votes.end(), 
            [](const auto& a, const auto& b) { return a.second < b.second; })->first;
    }
};

// Function to load CSV data
vector<DataPoint> loadCSV(const string& filename) {
    ifstream file(filename);
    vector<DataPoint> dataset;
    string line;
    
    while (getline(file, line)) {
        DataPoint point;
        size_t pos = 0;
        string token;
        bool isLabel = false;
        
        while ((pos = line.find(',')) != string::npos) {
            token = line.substr(0, pos);
            line.erase(0, pos + 1);
            
            if (line.find(',') == string::npos) { // Last element is the label
                point.label = line;
                break;
            }
            
            point.features.push_back(stod(token));
        }
        dataset.push_back(point);
    }
    
    return dataset;
}

// Function to split dataset into train and test
void trainTestSplit(const vector<DataPoint>& dataset, 
                    vector<DataPoint>& train, 
                    vector<DataPoint>& test, 
                    double testSize = 0.3, 
                    int randomSeed = 42) {
    vector<DataPoint> shuffled = dataset;
    shuffle(shuffled.begin(), shuffled.end(), default_random_engine(randomSeed));
    
    size_t split = static_cast<size_t>(dataset.size() * (1 - testSize));
    train = vector<DataPoint>(shuffled.begin(), shuffled.begin() + split);
    test = vector<DataPoint>(shuffled.begin() + split, shuffled.end());
}

// Function to evaluate the classifier
void evaluate(const KNNClassifier& knn, 
              const vector<DataPoint>& testData, 
              const vector<string>& classes) {
    map<string, int> correct;
    map<string, int> total;
    
    for (const auto& cls : classes) {
        correct[cls] = 0;
        total[cls] = 0;
    }
    
    int overallCorrect = 0;
    
    for (const auto& point : testData) {
        string prediction = knn.predict(point);
        total[point.label]++;
        if (prediction == point.label) {
            correct[point.label]++;
            overallCorrect++;
        }
    }
    
    // Print overall accuracy
    double accuracy = static_cast<double>(overallCorrect) / testData.size();
    cout << "Overall accuracy: " << fixed << setprecision(2) << accuracy << endl;
    
    // Print accuracy per class
    for (const auto& cls : classes) {
        if (total[cls] > 0) {
            double classAccuracy = static_cast<double>(correct[cls]) / total[cls];
            cout << "Accuracy for class " << cls << ": " 
                 << fixed << setprecision(2) << classAccuracy << endl;
        }
    }
}

int main() {
    // 1. Load dataset
    vector<DataPoint> dataset = loadCSV("iris.csv");
    
    // Get unique classes
    vector<string> classes;
    for (const auto& point : dataset) {
        if (find(classes.begin(), classes.end(), point.label) == classes.end()) {
            classes.push_back(point.label);
        }
    }
    
    // 2. Convert string labels to numeric (not needed as we're keeping them as strings)
    
    // 3. Train-test split (70% train, 30% test)
    vector<DataPoint> train, test;
    trainTestSplit(dataset, train, test, 0.3, 42);
    
    // 4. Train KNN model (K=5)
    KNNClassifier knn(5);
    knn.train(train);
    
    // 5. Evaluate model
    evaluate(knn, test, classes);
    
    return 0;
}