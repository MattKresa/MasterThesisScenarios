import weka.core.Instances;
import weka.core.converters.ConverterUtils.DataSource;
import weka.classifiers.lazy.IBk;

public class IrisKNN {
    public static void main(String[] args) throws Exception {
        // 1. Load the dataset
        DataSource source = new DataSource("iris.csv");
        Instances data = source.getDataSet();

        // Set class index (assuming species is the last attribute)
        if (data.classIndex() == -1) {
            data.setClassIndex(data.numAttributes() - 1);
        }

        // 2. Separate features and labels (handled automatically in Weka)

        // 3. Train-test split (80-20 split with stratification)
        data.randomize(new java.util.Random(42));
        Instances train = data.trainCV(3, 0); // 2/3 for training
        Instances test = data.testCV(3, 0);   // 1/3 for testing

        // 4. Train KNN model (IBk is Weka's KNN implementation)
        IBk model = new IBk(5); // k=5
        model.buildClassifier(train);

        // 5. Predictions and evaluation
        double correct = 0;
        double[] classCorrect = new double[data.numClasses()];
        double[] classTotal = new double[data.numClasses()];

        for (int i = 0; i < test.numInstances(); i++) {
            double actual = test.instance(i).classValue();
            double predicted = model.classifyInstance(test.instance(i));

            classTotal[(int) actual]++;
            if (actual == predicted) {
                correct++;
                classCorrect[(int) actual]++;
            }
        }

        // 6. Overall accuracy
        System.out.printf("Overall accuracy: %.2f%%\n",
                (correct / test.numInstances()) * 100);

        // 7. Accuracy per class
        for (int i = 0; i < data.numClasses(); i++) {
            String className = data.classAttribute().value(i);
            double accuracy = (classCorrect[i] / classTotal[i]) * 100;
            System.out.printf("Accuracy for class %s: %.2f%%\n",
                    className, accuracy);
        }
    }
}