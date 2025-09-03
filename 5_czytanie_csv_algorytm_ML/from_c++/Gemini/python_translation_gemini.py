import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.neighbors import KNeighborsClassifier
from collections import Counter


def main():
    # 1. Load dataset (iris.csv)
    # The C++ code manually reads the file, but in Python, pandas is a more idiomatic way.
    # Assuming 'iris.csv' has columns 'sepal_length', 'sepal_width', 'petal_length', 'petal_width', 'species'
    try:
        df = pd.read_csv('iris.csv')
    except FileNotFoundError:
        print("Error: 'iris.csv' not found. Please make sure the file is in the same directory.")
        return

    # Separate features (X) and labels (y)
    X = df.iloc[:, :4].values
    y = df.iloc[:, 4].values

    # 2. Train-test split (70% train, 30% test)
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.3, random_state=42, stratify=y)

    # 3. Train KNN model (k=5)
    # The C++ code manually implements the neighbor search and majority vote.
    # In Python, scikit-learn's KNeighborsClassifier handles this efficiently.
    knn = KNeighborsClassifier(n_neighbors=5)
    knn.fit(X_train, y_train)

    # 4. Predict labels for the test set
    y_pred = knn.predict(X_test)

    # 5. Overall accuracy
    accuracy = np.mean(y_pred == y_test)
    print(f"Overall accuracy: {accuracy:.4f}")

    # 6. Accuracy per class
    labels = np.unique(y_test)
    for label in labels:
        # Get indices for the current class in the test set
        label_indices = (y_test == label)

        # Get the true labels and predictions for this class
        true_labels_class = y_test[label_indices]
        pred_labels_class = y_pred[label_indices]

        # Calculate accuracy for the class
        correct_class = np.sum(pred_labels_class == true_labels_class)
        total_class = len(true_labels_class)

        if total_class > 0:
            accuracy_class = correct_class / total_class
            print(f"Accuracy for class {label}: {accuracy_class:.2f}")
        else:
            print(f"Accuracy for class {label}: No samples in test set.")


if __name__ == "__main__":
    main()