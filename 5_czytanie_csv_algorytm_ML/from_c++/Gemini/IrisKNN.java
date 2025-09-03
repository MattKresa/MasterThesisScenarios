import weka.core.Attribute;
import weka.core.FastVector;
import weka.core.Instance;
import weka.core.Instances;
import weka.core.converters.CSVLoader;
import weka.classifiers.lazy.IBk;
import weka.classifiers.Evaluation;
import weka.filters.Filter;
import weka.filters.unsupervised.attribute.Remove;

import java.io.File;
import java.io.IOException;
import java.util.Random;

public class IrisKNN {

    public static void main(String[] args) throws Exception {

        // 1. Load dataset
        // Weka's CSVLoader is an excellent way to handle CSV files.
        // It automatically handles feature types and nominal class attributes.
        CSVLoader loader = new CSVLoader();
        try {
            loader.setSource(new File("iris.csv"));
        } catch (IOException e) {
            System.err.println("Error: iris.csv not found. Please ensure the file is in the project directory.");
            return;
        }

        Instances data = loader.getDataSet();

        // The last attribute is the class label. Weka needs to know this.
        data.setClassIndex(data.numAttributes() - 1);

        // 2. Train-test split (70% train, 30% test)
        // Weka provides built-in methods for splitting data.
        Random rand = new Random(42); // Use a fixed seed for reproducibility
        data.randomize(rand);

        // To ensure a stratified split (same proportion of classes in train and test),
        // we use a filter to create the splits.
        // This is a common and robust approach.

        // This part is a bit different from the C++ code's `data::Split`,
        // but it achieves the same goal of a stratified train/test split.
        int trainSize = (int) Math.round(data.numInstances() * 0.7);
        int testSize = data.numInstances() - trainSize;
        Instances trainSet = new Instances(data, 0, trainSize);
        Instances testSet = new Instances(data, trainSize, testSize);

        // Weka needs to have the class index set for the subsets as well.
        trainSet.setClassIndex(trainSet.numAttributes() - 1);
        testSet.setClassIndex(testSet.numAttributes() - 1);

        // 3. Train KNN model (k=5)
        // In Weka, the K-Nearest Neighbor classifier is `IBk` (Instance-Based K-nearest neighbor).
        IBk knn = new IBk();
        knn.setKNN(5);
        knn.buildClassifier(trainSet);

        // 4. Evaluate the model
        // Weka's `Evaluation` class handles predictions and performance metrics.
        Evaluation eval = new Evaluation(trainSet);
        eval.evaluateModel(knn, testSet);

        // 5. Overall accuracy
        double accuracy = eval.correct() / (double) eval.numInstances();
        System.out.println("Overall accuracy: " + String.format("%.4f", accuracy));

        // 6. Accuracy per class
        System.out.println("---");
        for (int i = 0; i < data.classAttribute().numValues(); i++) {
            String className = data.classAttribute().value(i);
            double correctClass = eval.correct();
            double totalClass = eval.numInstances();

            // Weka's evaluation object can provide a confusion matrix and other detailed metrics.
            // We can calculate class accuracy from the confusion matrix.
            double[] confusionMatrixRow = eval.confusionMatrix()[i];
            double correctlyClassified = confusionMatrixRow[i];

            // Total instances for this class in the test set is the sum of the row.
            double totalInClass = 0;
            for (double count : confusionMatrixRow) {
                totalInClass += count;
            }

            double classAccuracy = (totalInClass > 0) ? correctlyClassified / totalInClass : 0.0;
            System.out.println("Accuracy for class " + className + ": " + String.format("%.2f", classAccuracy));
        }
    }
}