import com.opencsv.CSVReader;
import smile.classification.KNN;
import smile.data.Dataset;
import smile.data.formula.Formula;
import smile.data.vector.DoubleVector;
import smile.data.vector.IntVector;
import smile.data.DataFrame;
import smile.data.vector.BaseVector;
import smile.data.vector.StringVector;
import smile.data.measure.NominalScale;
import smile.validation.ClassificationMetrics;

import java.io.FileReader;
import java.util.*;
import java.util.stream.Collectors;

public class IrisKNN {
    public static void main(String[] args) throws Exception {
        String csvPath = "iris.csv";

        // 1. Load dataset
        List<double[]> featureList = new ArrayList<>();
        List<String> labelList = new ArrayList<>();

        try (CSVReader reader = new CSVReader(new FileReader(csvPath))) {
            String[] line;
            reader.readNext(); // skip header
            while ((line = reader.readNext()) != null) {
                double[] features = new double[4];
                for (int i = 0; i < 4; i++) {
                    features[i] = Double.parseDouble(line[i]);
                }
                featureList.add(features);
                labelList.add(line[4]);
            }
        }

        int nSamples = featureList.size();
        int nFeatures = 4;

        // Encode labels
        Map<String, Integer> labelMap = new HashMap<>();
        Map<Integer, String> reverseLabelMap = new HashMap<>();
        int currentLabel = 0;
        int[] y = new int[nSamples];
        double[][] X = new double[nSamples][nFeatures];

        for (int i = 0; i < nSamples; i++) {
            System.arraycopy(featureList.get(i), 0, X[i], 0, nFeatures);
            String label = labelList.get(i);
            if (!labelMap.containsKey(label)) {
                labelMap.put(label, currentLabel);
                reverseLabelMap.put(currentLabel, label);
                currentLabel++;
            }
            y[i] = labelMap.get(label);
        }

        // 2. Train-test split (70% train, 30% test)
        double testRatio = 0.3;
        int testSize = (int) (nSamples * testRatio);
        List<Integer> indices = new ArrayList<>();
        for (int i = 0; i < nSamples; i++) indices.add(i);
        Collections.shuffle(indices, new Random(42));

        double[][] X_train = new double[nSamples - testSize][nFeatures];
        int[] y_train = new int[nSamples - testSize];
        double[][] X_test = new double[testSize][nFeatures];
        int[] y_test = new int[testSize];

        for (int i = 0; i < nSamples; i++) {
            int idx = indices.get(i);
            if (i < nSamples - testSize) {
                X_train[i] = X[idx];
                y_train[i] = y[idx];
            } else {
                X_test[i - (nSamples - testSize)] = X[idx];
                y_test[i - (nSamples - testSize)] = y[idx];
            }
        }

        // 3. Train KNN model (k=5)
        KNN<double[]> knn = KNN.learn(X_train, y_train, 5);

        // 4. Predict
        int[] y_pred = new int[testSize];
        for (int i = 0; i < testSize; i++) {
            y_pred[i] = knn.predict(X_test[i]);
        }

        // 5. Overall accuracy
        int correct = 0;
        for (int i = 0; i < testSize; i++) {
            if (y_pred[i] == y_test[i]) correct++;
        }
        double accuracy = (double) correct / testSize;
        System.out.println("Overall accuracy: " + accuracy);

        // 6. Accuracy per class
        for (Map.Entry<Integer, String> entry : reverseLabelMap.entrySet()) {
            int label = entry.getKey();
            String className = entry.getValue();
            int total = 0, correctClass = 0;
            for (int i = 0; i < testSize; i++) {
                if (y_test[i] == label) {
                    total++;
                    if (y_pred[i] == label) correctClass++;
                }
            }
            System.out.printf("Accuracy for class %s: %.2f%n", className, (double) correctClass / total);
        }
    }
}
