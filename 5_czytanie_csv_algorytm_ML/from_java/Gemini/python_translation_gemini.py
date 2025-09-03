import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.neighbors import KNeighborsClassifier
from sklearn.metrics import accuracy_score, classification_report
from sklearn.preprocessing import LabelEncoder
import numpy as np

def iris_knn():
    """
    Translates the Java Weka code for Iris KNN classification to Python using scikit-learn.
    """
    # 1. Load dataset (assuming 'iris.csv' is in the same directory)
    # The Weka code handles string to nominal conversion implicitly, which we'll do explicitly.
    try:
        data = pd.read_csv("iris.csv")
    except FileNotFoundError:
        print("Error: 'iris.csv' not found. Please ensure the file is in the same directory.")
        return

    # 2. Convert string to nominal (if necessary)
    # Weka's `StringToNominal` filter is equivalent to scikit-learn's `LabelEncoder` for the class attribute.
    # The code assumes the last column is the class attribute.
    class_column = data.columns[-1]
    le = LabelEncoder()
    data[class_column] = le.fit_transform(data[class_column])

    # 3. Separate features (X) and labels (y)
    X = data.iloc[:, :-1]
    y = data.iloc[:, -1]

    # 4. Train-test split (70% train, 30% test)
    # Weka's `trainCV` and `testCV` with `randomize` and `stratify` is equivalent to
    # scikit-learn's `train_test_split` with `random_state` and `stratify`.
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.3, random_state=42, stratify=y
    )

    # 5. Train KNN model
    # Weka's `IBk(5)` is equivalent to scikit-learn's `KNeighborsClassifier(n_neighbors=5)`.
    knn = KNeighborsClassifier(n_neighbors=5)
    knn.fit(X_train, y_train)

    # 6. Evaluate model
    # Weka's `evaluateModel` is equivalent to scikit-learn's prediction and reporting functions.
    y_pred = knn.predict(X_test)

    # 7. Overall accuracy
    # Weka's `eval.pctCorrect() / 100` is equivalent to `accuracy_score(y_test, y_pred)`.
    overall_accuracy = accuracy_score(y_test, y_pred)
    print(f"Overall accuracy: {overall_accuracy:.2f}")

    # 8. Accuracy per class
    # Weka's `eval.recall(i)` is part of scikit-learn's `classification_report`.
    class_names = le.inverse_transform(np.arange(len(le.classes_)))
    report = classification_report(y_test, y_pred, target_names=class_names)
    print("\nAccuracy per class (Recall):")
    print(report)

if __name__ == "__main__":
    iris_knn()