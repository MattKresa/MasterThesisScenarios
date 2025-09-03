import java.io.*;
import java.util.*;
import java.util.stream.Collectors;

class DataPoint {
    public double[] features;
    public int label;
    public String className;

    public DataPoint(double[] features, int label, String className) {
        this.features = features.clone();
        this.label = label;
        this.className = className;
    }
}

class KNNClassifier {
    private List<DataPoint> trainingData;
    private int k;

    public KNNClassifier(int k) {
        this.k = k;
        this.trainingData = new ArrayList<>();
    }

    public void fit(List<DataPoint> trainData) {
        this.trainingData = new ArrayList<>(trainData);
    }

    private double euclideanDistance(double[] a, double[] b) {
        double sum = 0.0;
        for (int i = 0; i < a.length; i++) {
            sum += Math.pow(a[i] - b[i], 2);
        }
        return Math.sqrt(sum);
    }

    public int predict(double[] features) {
        // Calculate distances to all training points
        List<DistanceLabelPair> distances = new ArrayList<>();

        for (DataPoint point : trainingData) {
            double dist = euclideanDistance(features, point.features);
            distances.add(new DistanceLabelPair(dist, point.label));
        }

        // Sort by distance
        distances.sort(Comparator.comparingDouble(d -> d.distance));

        // Count votes from k nearest neighbors
        Map<Integer, Integer> votes = new HashMap<>();
        for (int i = 0; i < Math.min(k, distances.size()); i++) {
            int label = distances.get(i).label;
            votes.put(label, votes.getOrDefault(label, 0) + 1);
        }

        // Return class with most votes
        return votes.entrySet().stream()
                .max(Map.Entry.comparingByValue())
                .map(Map.Entry::getKey)
                .orElse(-1);
    }

    public int[] predict(List<DataPoint> testData) {
        int[] predictions = new int[testData.size()];
        for (int i = 0; i < testData.size(); i++) {
            predictions[i] = predict(testData.get(i).features);
        }
        return predictions;
    }

    private static class DistanceLabelPair {
        double distance;
        int label;

        DistanceLabelPair(double distance, int label) {
            this.distance = distance;
            this.label = label;
        }
    }
}

class LabelEncoder {
    private Map<String, Integer> classToLabel;
    private Map<Integer, String> labelToClass;
    private int nextLabel;

    public LabelEncoder() {
        this.classToLabel = new HashMap<>();
        this.labelToClass = new HashMap<>();
        this.nextLabel = 0;
    }

    public int[] fitTransform(List<String> classes) {
        int[] labels = new int[classes.size()];

        for (int i = 0; i < classes.size(); i++) {
            String cls = classes.get(i);
            if (!classToLabel.containsKey(cls)) {
                classToLabel.put(cls, nextLabel);
                labelToClass.put(nextLabel, cls);
                nextLabel++;
            }
            labels[i] = classToLabel.get(cls);
        }

        return labels;
    }

    public List<String> getClassNames() {
        return labelToClass.values().stream()
                .collect(Collectors.toList());
    }
}

public class KNNIrisClassification {

    public static List<DataPoint> loadIrisDataset(String filename) throws IOException {
        List<DataPoint> data = new ArrayList<>();
        List<String> speciesList = new ArrayList<>();

        try (BufferedReader br = new BufferedReader(new FileReader(filename))) {
            String line = br.readLine();

            // Check if first line is header
            boolean hasHeader = line != null &&
                    (line.toLowerCase().contains("sepal") ||
                            line.toLowerCase().contains("petal") ||
                            line.toLowerCase().contains("species"));

            if (!hasHeader && line != null) {
                // Process first line as data
                processDataLine(line, data, speciesList);
            }

            // Process remaining lines
            while ((line = br.readLine()) != null) {
                processDataLine(line, data, speciesList);
            }
        }

        // Encode labels
        LabelEncoder encoder = new LabelEncoder();
        int[] labels = encoder.fitTransform(speciesList);

        for (int i = 0; i < data.size(); i++) {
            data.get(i).label = labels[i];
        }

        return data;
    }

    private static void processDataLine(String line, List<DataPoint> data, List<String> speciesList) {
        String[] tokens = line.split(",");
        if (tokens.length >= 5) {
            double[] features = new double[4];
            for (int i = 0; i < 4; i++) {
                features[i] = Double.parseDouble(tokens[i].trim());
            }
            String species = tokens[4].trim();

            data.add(new DataPoint(features, -1, species)); // label will be set later
            speciesList.add(species);
        }
    }

    public static void trainTestSplit(List<DataPoint> data,
                                      List<DataPoint> train,
                                      List<DataPoint> test,
                                      double testSize,
                                      long randomState) {

        // Group by class for stratified split
        Map<Integer, List<DataPoint>> classData = new HashMap<>();
        for (DataPoint point : data) {
            classData.computeIfAbsent(point.label, k -> new ArrayList<>()).add(point);
        }

        Random rng = new Random(randomState);

        // Split each class proportionally
        for (List<DataPoint> classPoints : classData.values()) {
            Collections.shuffle(classPoints, rng);

            int testCount = (int) (classPoints.size() * testSize);

            for (int i = 0; i < testCount; i++) {
                test.add(classPoints.get(i));
            }

            for (int i = testCount; i < classPoints.size(); i++) {
                train.add(classPoints.get(i));
            }
        }

        // Shuffle final sets
        Collections.shuffle(train, rng);
        Collections.shuffle(test, rng);
    }

    public static double calculateAccuracy(int[] yTrue, int[] yPred) {
        int correct = 0;
        for (int i = 0; i < yTrue.length; i++) {
            if (yTrue[i] == yPred[i]) {
                correct++;
            }
        }
        return (double) correct / yTrue.length;
    }

    public static void main(String[] args) {
        try {
            // 1. Load the dataset
            List<DataPoint> df = loadIrisDataset("iris.csv");

            if (df.isEmpty()) {
                System.err.println("Error: Could not load iris.csv");
                return;
            }

            // Get unique class names
            List<String> classNames = df.stream()
                    .map(point -> point.className)
                    .distinct()
                    .collect(Collectors.toList());

            // 3. Train-test split
            List<DataPoint> XTrain = new ArrayList<>();
            List<DataPoint> XTest = new ArrayList<>();
            trainTestSplit(df, XTrain, XTest, 0.3, 42);

            // Extract true labels for test set
            int[] yTest = XTest.stream().mapToInt(point -> point.label).toArray();

            // 4. Train KNN model
            KNNClassifier model = new KNNClassifier(5);
            model.fit(XTrain);

            // 5. Predictions
            int[] yPred = model.predict(XTest);

            // 6. Overall accuracy
            double overallAccuracy = calculateAccuracy(yTest, yPred);
            System.out.printf("Overall accuracy: %.4f%n", overallAccuracy);

            // 7. Accuracy per class
            for (int clsIdx = 0; clsIdx < classNames.size(); clsIdx++) {
                int correct = 0;
                int total = 0;

                for (int i = 0; i < yTest.length; i++) {
                    if (yTest[i] == clsIdx) {
                        total++;
                        if (yPred[i] == yTest[i]) {
                            correct++;
                        }
                    }
                }

                if (total > 0) {
                    double classAccuracy = (double) correct / total;
                    System.out.printf("Accuracy for class %s: %.2f%n",
                            classNames.get(clsIdx), classAccuracy);
                }
            }

        } catch (IOException e) {
            System.err.println("Error reading file: " + e.getMessage());
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}