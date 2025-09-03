import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.neighbors import KNeighborsClassifier
from sklearn.metrics import accuracy_score, classification_report


def main():
    # 1. Load dataset (using pandas for simplicity)
    df = pd.read_csv('iris.csv')

    # Separate features and labels
    X = df.iloc[:, :4].values.T  # Transpose to match mlpack's (features x samples) format
    y_strings = df.iloc[:, 4].values

    # Encode labels
    label_map = {label: idx for idx, label in enumerate(np.unique(y_strings))}
    reverse_label_map = {idx: label for label, idx in label_map.items()}
    y = np.array([label_map[label] for label in y_strings])

    # 2. Train-test split (70% train, 30% test)
    X_train, X_test, y_train, y_test = train_test_split(
        X.T, y, test_size=0.3, random_state=42
    )

    # Note: We transpose back to (samples x features) for scikit-learn

    # 3. Train KNN model (k=5)
    knn = KNeighborsClassifier(n_neighbors=5)
    knn.fit(X_train, y_train)

    # 4. Predict on test set (scikit-learn handles majority voting internally)
    y_pred = knn.predict(X_test)

    # 5. Overall accuracy
    accuracy = accuracy_score(y_test, y_pred)
    print(f"Overall accuracy: {accuracy:.4f}")

    # 6. Accuracy per class (using classification report)
    print("\nClassification Report:")
    print(classification_report(
        y_test, y_pred,
        target_names=[reverse_label_map[i] for i in sorted(reverse_label_map.keys())]
    ))


if __name__ == "__main__":
    main()