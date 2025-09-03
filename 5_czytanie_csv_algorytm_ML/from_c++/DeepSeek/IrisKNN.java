import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

import weka.classifiers.lazy.IBk;
import weka.core.Attribute;
import weka.core.DenseInstance;
import weka.core.Instance;
import weka.core.Instances;
import weka.core.converters.ConverterUtils.DataSource;
import weka.filters.Filter;
import weka.filters.unsupervised.instance.RemovePercentage;

public class IrisKNN {

    public static void main(String[] args) throws Exception {
        // 1. Load dataset
        List<double[]> features = new ArrayList<>();
        List<String> labels = new ArrayList<>();
        
        try (BufferedReader br = new BufferedReader(new FileReader("iris.csv"))) {
            // Skip header
            br.readLine();
            
            String line;
            while ((line = br.readLine()) != null) {
                String[] values = line.split(",");
                double[] featureRow = new double[4];
                for (int i = 0; i < 4; i++) {
                    featureRow[i] = Double.parseDouble(values[i]);
                }
                features.add(featureRow);
                labels.add(values[4]);
            }
        } catch (IOException e) {
            e.printStackTrace();
            return;
        }

        // Create Weka dataset
        ArrayList<Attribute> attributes = new ArrayList<>();
        for (int i = 0; i < 4; i++) {
            attributes.add(new Attribute("feature" + i));
        }
        
        // Encode labels
        List<String> classValues = new ArrayList<>(new ArrayList<>(labels.stream().distinct().toList()));
        attributes.add(new Attribute("class", classValues));
        
        Instances dataset = new Instances("Iris", attributes, features.size());
        dataset.setClassIndex(4); // Set class attribute
        
        Map<String, Integer> labelMap = new HashMap<>();
        Map<Integer, String> reverseLabelMap = new HashMap<>();
        int currentLabel = 0;
        
        for (int i = 0; i < features.size(); i++) {
            double[] featureRow = features.get(i);
            Instance instance = new DenseInstance(5);
            for (int j = 0; j < 4; j++) {
                instance.setValue(j, featureRow[j]);
            }
            
            String label = labels.get(i);
            if (!labelMap.containsKey(label)) {
                labelMap.put(label, currentLabel);
                reverseLabelMap.put(currentLabel, label);
                currentLabel++;
            }
            
            instance.setValue(4, label);
            dataset.add(instance);
        }

        // 2. Train-test split (70% train, 30% test)
        RemovePercentage filter = new RemovePercentage();
        filter.setPercentage(30);
        filter.setInputFormat(dataset);
        
        Instances trainData = Filter.useFilter(dataset, filter);
        filter.setInvertSelection(true);
        Instances testData = Filter.useFilter(dataset, filter);

        // 3. Train KNN model (k=5)
        IBk knn = new IBk(5);
        knn.buildClassifier(trainData);

        // 4. Predict on test set
        double[] predictions = new double[testData.size()];
        for (int i = 0; i < testData.size(); i++) {
            predictions[i] = knn.classifyInstance(testData.get(i));
        }

        // 5. Overall accuracy
        int correct = 0;
        for (int i = 0; i < testData.size(); i++) {
            if (predictions[i] == testData.get(i).classValue()) {
                correct++;
            }
        }
        double accuracy = (double) correct / testData.size();
        System.out.printf("Overall accuracy: %.4f%n", accuracy);

        // 6. Accuracy per class
        for (Map.Entry<Integer, String> entry : reverseLabelMap.entrySet()) {
            int label = entry.getKey();
            String className = entry.getValue();
            
            int total = 0;
            int correctClass = 0;
            
            for (int i = 0; i < testData.size(); i++) {
                if (testData.get(i).classValue() == label) {
                    total++;
                    if (predictions[i] == label) {
                        correctClass++;
                    }
                }
            }
            
            if (total > 0) {
                System.out.printf("Accuracy for class %s: %.2f%n", 
                    className, (double) correctClass / total);
            }
        }
    }
}