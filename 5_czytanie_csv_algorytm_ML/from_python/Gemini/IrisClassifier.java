import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

public class IrisClassifier {

    // --- Nested Class: IrisData ---
    // Represents a single row of Iris data
    public static class IrisData {
        double sepalLength;
        double sepalWidth;
        double petalLength;
        double petalWidth;
        String species; // Original species name

        public IrisData(double sepalLength, double sepalWidth, double petalLength, double petalWidth, String species) {
            this.sepalLength = sepalLength;
            this.sepalWidth = sepalWidth;
            this.petalLength = petalLength;
            this.petalWidth = petalWidth;
            this.species = species;
        }

        public double[] getFeatures() {
            return new double[]{sepalLength, sepalWidth, petalLength, petalWidth};
        }

        public String getSpecies() {
            return species;
        }
    }

    // --- Nested Class: LabelEncoder ---
    // Custom implementation of Scikit-learn's LabelEncoder
    public static class LabelEncoder {
        private Map<String, Integer> labelMap;
        private List<String> classNames; // To store the original class names in order of their integer mapping

        public LabelEncoder() {
            labelMap = new HashMap<>();
            classNames = new ArrayList<>();
        }

        public int[] fitTransform(List<String> labels) {
            int nextId = 0;
            int[] encodedLabels = new int[labels.size()];
            for (int i = 0; i < labels.size(); i++) {
                String label = labels.get(i);
                if (!labelMap.containsKey(label)) {
                    labelMap.put(label, nextId);
                    classNames.add(label); // Store the original name
                    nextId++;
                }
                encodedLabels[i] = labelMap.get(label);
            }
            return encodedLabels;
        }

        public List<String> getClassNames() {
            return classNames;
        }
    }

    // --- Nested Class: KNeighborsClassifier ---
    // Custom implementation of Scikit-learn's KNeighborsClassifier
    public static class KNeighborsClassifier {
        private int nNeighbors;
        private double[][] X_train;
        private int[] y_train;

        public KNeighborsClassifier(int nNeighbors) {
            this.nNeighbors = nNeighbors;
        }

        public void fit(double[][] X_train, int[] y_train) {
            this.X_train = X_train;
            this.y_train = y_train;
        }

        public int[] predict(double[][] X_test) {
            int[] predictions = new int[X_test.length];
            for (int i = 0; i < X_test.length; i++) {
                predictions[i] = predictSingle(X_test[i]);
            }
            return predictions;
        }

        private int predictSingle(double[] testPoint) {
            List<NeighborDistance> distances = new ArrayList<>();
            for (int i = 0; i < X_train.length; i++) {
                double dist = euclideanDistance(testPoint, X_train[i]);
                distances.add(new NeighborDistance(dist, y_train[i]));
            }

            // Sort by distance
            Collections.sort(distances, Comparator.comparingDouble(d -> d.distance));

            // Get k nearest neighbors
            Map<Integer, Integer> labelCounts = new HashMap<>();
            for (int i = 0; i < nNeighbors && i < distances.size(); i++) {
                int label = distances.get(i).label;
                labelCounts.put(label, labelCounts.getOrDefault(label, 0) + 1);
            }

            // Find majority label
            int majorityLabel = -1;
            int maxCount = -1;
            for (Map.Entry<Integer, Integer> entry : labelCounts.entrySet()) {
                if (entry.getValue() > maxCount) {
                    maxCount = entry.getValue();
                    majorityLabel = entry.getKey();
                }
            }
            return majorityLabel;
        }

        private double euclideanDistance(double[] p1, double[] p2) {
            double sumSq = 0;
            for (int i = 0; i < p1.length; i++) {
                sumSq += Math.pow(p1[i] - p2[i], 2);
            }
            return Math.sqrt(sumSq);
        }

        private static class NeighborDistance {
            double distance;
            int label;

            public NeighborDistance(double distance, int label) {
                this.distance = distance;
                this.label = label;
            }
        }
    }


    // --- Data Loading ---
    public static List<IrisData> loadCsv(String filename) throws IOException {
        List<IrisData> dataList = new ArrayList<>();
        try (BufferedReader br = new BufferedReader(new FileReader(filename))) {
            String line;
            // Read header line (and discard for this simple example)
            if ((line = br.readLine()) == null) {
                throw new IOException("CSV file is empty or missing header.");
            }

            while ((line = br.readLine()) != null) {
                String[] parts = line.split(",");
                if (parts.length == 5) { // Assuming 4 features + 1 species
                    try {
                        double sepalLength = Double.parseDouble(parts[0]);
                        double sepalWidth = Double.parseDouble(parts[1]);
                        double petalLength = Double.parseDouble(parts[2]);
                        double petalWidth = Double.parseDouble(parts[3]);
                        String species = parts[4];
                        dataList.add(new IrisData(sepalLength, sepalWidth, petalLength, petalWidth, species));
                    } catch (NumberFormatException e) {
                        System.err.println("Skipping malformed row (number format error): " + line);
                    }
                } else {
                    System.err.println("Skipping malformed row (incorrect number of columns): " + line);
                }
            }
        }
        return dataList;
    }

    // --- Train-Test Split ---
    // This implementation does NOT include 'stratify' logic, which is more complex.
    public static void trainTestSplit(
            double[][] X, int[] y, double testSize, int randomState,
            List<double[]> X_train, List<double[]> X_test,
            List<Integer> y_train, List<Integer> y_test
    ) {
        Random rand = new Random(randomState);
        List<Integer> indices = new ArrayList<>();
        for (int i = 0; i < X.length; i++) {
            indices.add(i);
        }
        Collections.shuffle(indices, rand);

        int numTest = (int) (X.length * testSize);
        int numTrain = X.length - numTest;

        for (int i = 0; i < numTrain; i++) {
            X_train.add(X[indices.get(i)]);
            y_train.add(y[indices.get(i)]);
        }

        for (int i = numTrain; i < X.length; i++) {
            X_test.add(X[indices.get(i)]);
            y_test.add(y[indices.get(i)]);
        }
    }

    // --- Evaluation Metrics ---
    public static double accuracyScore(int[] y_true, int[] y_pred) {
        if (y_true.length == 0) return 0.0;
        int correctCount = 0;
        for (int i = 0; i < y_true.length; i++) {
            if (y_true[i] == y_pred[i]) {
                correctCount++;
            }
        }
        return (double) correctCount / y_true.length;
    }

    public static void main(String[] args) {
        try {
            // 1. Load the dataset
            List<IrisData> df = loadCsv("iris.csv");

            // 2. Separate features and labels
            List<String> speciesList = new ArrayList<>();
            double[][] X_full = new double[df.size()][4]; // 4 features
            for (int i = 0; i < df.size(); i++) {
                X_full[i] = df.get(i).getFeatures();
                speciesList.add(df.get(i).getSpecies());
            }

            LabelEncoder labelEncoder = new LabelEncoder();
            int[] y_full = labelEncoder.fitTransform(speciesList);
            List<String> classNames = labelEncoder.getClassNames();

            // 3. Train-test split
            List<double[]> X_train_list = new ArrayList<>();
            List<double[]> X_test_list = new ArrayList<>();
            List<Integer> y_train_list = new ArrayList<>();
            List<Integer> y_test_list = new ArrayList<>();

            trainTestSplit(X_full, y_full, 0.3, 42, X_train_list, X_test_list, y_train_list, y_test_list);

            // Convert lists back to arrays for the KNN model
            double[][] X_train = X_train_list.toArray(new double[0][]);
            int[] y_train = y_train_list.stream().mapToInt(Integer::intValue).toArray();
            double[][] X_test = X_test_list.toArray(new double[0][]);
            int[] y_test = y_test_list.stream().mapToInt(Integer::intValue).toArray();

            // 4. Train KNN model
            KNeighborsClassifier model = new KNeighborsClassifier(5);
            model.fit(X_train, y_train);

            // 5. Predictions
            int[] y_pred = model.predict(X_test);

            // 6. Overall accuracy
            System.out.println("Overall accuracy: " + String.format("%.2f", accuracyScore(y_test, y_pred)));

            // 7. Accuracy per class
            System.out.println("\nAccuracy per class:");
            for (int i = 0; i < classNames.size(); i++) {
                String cls = classNames.get(i);
                int correct = 0;
                int total = 0;
                for (int j = 0; j < y_test.length; j++) {
                    if (y_test[j] == i) { // Check if the true label matches the current class index
                        total++;
                        if (y_pred[j] == i) { // Check if the prediction also matches
                            correct++;
                        }
                    }
                }
                double accuracyCls = (total == 0) ? 0.0 : (double) correct / total;
                System.out.println(String.format("Accuracy for class %s: %.2f", cls, accuracyCls));
            }

        } catch (IOException e) {
            System.err.println("Error reading CSV file: " + e.getMessage());
        } catch (Exception e) {
            System.err.println("An unexpected error occurred: " + e.getMessage());
            e.printStackTrace();
        }
    }
}