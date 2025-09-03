import java.io.*;
import java.util.*;

public class KNNIrisClassifier {

    static class DataPoint {
        double[] features;
        int label;

        DataPoint(double[] features, int label) {
            this.features = features.clone();
            this.label = label;
        }
    }

    static class KNN {
        List<DataPoint> trainingData;
        int k;

        KNN(List<DataPoint> trainingData, int k) {
            this.trainingData = trainingData;
            this.k = k;
        }

        int predict(double[] testPoint) {
            // Calculate distances to all training points
            List<DistanceLabel> distances = new ArrayList<>();

            for (DataPoint trainPoint : trainingData) {
                double distance = euclideanDistance(testPoint, trainPoint.features);
                distances.add(new DistanceLabel(distance, trainPoint.label));
            }

            // Sort by distance and take k nearest neighbors
            Collections.sort(distances);

            // Count votes from k nearest neighbors
            Map<Integer, Integer> votes = new HashMap<>();
            for (int i = 0; i < k && i < distances.size(); i++) {
                int label = distances.get(i).label;
                votes.put(label, votes.getOrDefault(label, 0) + 1);
            }

            // Return majority vote
            int bestLabel = -1;
            int maxVotes = 0;
            for (Map.Entry<Integer, Integer> entry : votes.entrySet()) {
                if (entry.getValue() > maxVotes) {
                    maxVotes = entry.getValue();
                    bestLabel = entry.getKey();
                }
            }

            return bestLabel;
        }

        double euclideanDistance(double[] a, double[] b) {
            double sum = 0.0;
            for (int i = 0; i < a.length; i++) {
                double diff = a[i] - b[i];
                sum += diff * diff;
            }
            return Math.sqrt(sum);
        }
    }

    static class DistanceLabel implements Comparable<DistanceLabel> {
        double distance;
        int label;

        DistanceLabel(double distance, int label) {
            this.distance = distance;
            this.label = label;
        }

        @Override
        public int compareTo(DistanceLabel other) {
            return Double.compare(this.distance, other.distance);
        }
    }

    public static void main(String[] args) {
        try {
            // 1. Load dataset
            List<double[]> featureRows = new ArrayList<>();
            List<String> labelStrings = new ArrayList<>();

            BufferedReader reader = new BufferedReader(new FileReader("iris.csv"));
            String line = reader.readLine(); // Skip header

            while ((line = reader.readLine()) != null) {
                String[] parts = line.split(",");
                double[] features = new double[4];
                for (int i = 0; i < 4; i++) {
                    features[i] = Double.parseDouble(parts[i]);
                }
                featureRows.add(features);
                labelStrings.add(parts[4]);
            }
            reader.close();

            // Encode labels
            Map<String, Integer> labelMap = new HashMap<>();
            Map<Integer, String> reverseLabelMap = new HashMap<>();
            int currentLabel = 0;

            List<DataPoint> allData = new ArrayList<>();
            for (int i = 0; i < featureRows.size(); i++) {
                String labelStr = labelStrings.get(i);
                if (!labelMap.containsKey(labelStr)) {
                    labelMap.put(labelStr, currentLabel);
                    reverseLabelMap.put(currentLabel, labelStr);
                    currentLabel++;
                }
                int label = labelMap.get(labelStr);
                allData.add(new DataPoint(featureRows.get(i), label));
            }

            // 2. Train-test split (70% train, 30% test)
            Collections.shuffle(allData, new Random(42)); // For reproducibility
            int trainSize = (int) (allData.size() * 0.7);

            List<DataPoint> trainData = allData.subList(0, trainSize);
            List<DataPoint> testData = allData.subList(trainSize, allData.size());

            // 3. Train KNN model (k=5)
            KNN knn = new KNN(trainData, 5);

            // 4. Make predictions
            int[] predictions = new int[testData.size()];
            int[] trueLabels = new int[testData.size()];

            for (int i = 0; i < testData.size(); i++) {
                predictions[i] = knn.predict(testData.get(i).features);
                trueLabels[i] = testData.get(i).label;
            }

            // 5. Overall accuracy
            int correct = 0;
            for (int i = 0; i < predictions.length; i++) {
                if (predictions[i] == trueLabels[i]) {
                    correct++;
                }
            }
            double accuracy = (double) correct / predictions.length;
            System.out.println("Overall accuracy: " + accuracy);

            // 6. Accuracy per class
            for (Map.Entry<Integer, String> entry : reverseLabelMap.entrySet()) {
                int label = entry.getKey();
                String className = entry.getValue();

                int totalClass = 0;
                int correctClass = 0;

                for (int i = 0; i < trueLabels.length; i++) {
                    if (trueLabels[i] == label) {
                        totalClass++;
                        if (predictions[i] == label) {
                            correctClass++;
                        }
                    }
                }

                if (totalClass > 0) {
                    double classAccuracy = (double) correctClass / totalClass;
                    System.out.printf("Accuracy for class %s: %.2f%n", className, classAccuracy);
                }
            }

        } catch (IOException e) {
            System.err.println("Error reading file: " + e.getMessage());
        }
    }
}