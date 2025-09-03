import weka.core.Instances;
import weka.core.converters.ConverterUtils.DataSource;
import weka.filters.Filter;
import weka.filters.unsupervised.attribute.StringToNominal;
import weka.classifiers.lazy.IBk;
import weka.classifiers.Evaluation;

import java.util.Random;

public class IrisKNN {

    public static void main(String[] args) throws Exception {
        // 1. Load dataset
        DataSource source = new DataSource("iris.csv");
        Instances data = source.getDataSet();

        // 2. Convert string to nominal (if necessary)
        StringToNominal filter = new StringToNominal();
        filter.setAttributeRange("last"); // assuming species is the last column
        filter.setInputFormat(data);
        data = Filter.useFilter(data, filter);

        // 3. Set class index to the label attribute
        if (data.classIndex() == -1) {
            data.setClassIndex(data.numAttributes() - 1);
        }

        // 4. Train-test split (70% train, 30% test)
        Random rand = new Random(42); // reproducibility
        Instances randData = new Instances(data);
        randData.randomize(rand);
        randData.stratify(10); // Weka stratifies based on folds, so use 10

        Instances train = randData.trainCV(10, 0, rand);
        Instances test = randData.testCV(10, 0);

        // 5. Train KNN model
        IBk knn = new IBk(5); // K=5
        knn.buildClassifier(train);

        // 6. Evaluate model
        Evaluation eval = new Evaluation(train);
        eval.evaluateModel(knn, test);

        // 7. Overall accuracy
        System.out.println("Overall accuracy: " + String.format("%.2f", eval.pctCorrect() / 100));

        // 8. Accuracy per class
        for (int i = 0; i < data.numClasses(); i++) {
            String className = data.classAttribute().value(i);
            double classPrecision = eval.recall(i); // recall = correctly classified / total actual
            System.out.printf("Accuracy for class %s: %.2f%n", className, classPrecision);
        }
    }
}
