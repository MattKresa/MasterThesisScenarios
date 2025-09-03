import weka.core.Instances;
import weka.core.converters.ConverterUtils.DataSource;
import weka.classifiers.lazy.IBk;
import weka.classifiers.Evaluation;
import weka.filters.Filter;
import weka.filters.unsupervised.attribute.StringToNominal;
import weka.filters.supervised.instance.StratifiedRemoveFolds;

public class IrisKNN {
    public static void main(String[] args) throws Exception {
        // 1. Load dataset (.csv) and convert species to nominal
        DataSource source = new DataSource("iris.csv");
        Instances data = source.getDataSet();
        // Convert 'species' column (last attribute) from String to Nominal
        StringToNominal str2nom = new StringToNominal();
        str2nom.setAttributeRange("last");
        str2nom.setInputFormat(data);
        data = Filter.useFilter(data, str2nom);

        // Set class index to the species attribute (last attribute)
        data.setClassIndex(data.numAttributes() - 1);

        // 2. Split into train (70%) and test (30%) with stratified sampling
        StratifiedRemoveFolds filter = new StratifiedRemoveFolds();
        filter.setNumFolds(10);               // Use 10 folds
        filter.setSeed(42);
        filter.setFold(1);                   // Fold 1 as test (~10%); approximate to ~30%
        filter.setInvertSelection(true);     // Keep fold 1 as test
        filter.setInputFormat(data);
        Instances test = Filter.useFilter(data, filter);

        filter.setInvertSelection(false);    // Remaining 9 folds as train
        filter.setInputFormat(data);
        Instances train = Filter.useFilter(data, filter);

        // 3. Build KNN classifier (k = 5)
        IBk knn = new IBk(5);
        knn.buildClassifier(train);

        // 4. Evaluate on test set
        Evaluation eval = new Evaluation(train);
        eval.evaluateModel(knn, test);

        // 5. Print overall accuracy
        double accuracy = eval.pctCorrect() / 100.0;
        System.out.printf("Overall accuracy: %.2f%n", accuracy);

        // 6. Accuracy per class
        for (int i = 0; i < data.numClasses(); i++) {
            String clsName = data.classAttribute().value(i);
            double classPrecision = eval.precision(i);
            // Alternatively: accuracy per class = correct / total
            System.out.printf("Accuracy for class %s: %.2f%n", clsName, classPrecision);
        }
    }
}
