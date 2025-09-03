import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.neighbors import KNeighborsClassifier
from sklearn.preprocessing import LabelEncoder
from collections import Counter


def main():
    # 1. Load dataset (features and labels)
    df = pd.read_csv("iris.csv")

    # Extract features (first 4 columns) and labels (last column)
    X = df.iloc[:, :4].values  # Features
    y_strings = df.iloc[:, -1].values  # Labels as strings

    # Encode labels to numeric values
    label_encoder = LabelEncoder()
    y = label_encoder.fit_transform(y_strings)

    # Create reverse mapping for class names
    label_classes = label_encoder.classes_

    # 2. Train-test split (70% train, 30% test)
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.3, random_state=42, stratify=y
    )

    # 3. Train KNN model (k=5)
    knn = KNeighborsClassifier(n_neighbors=5)
    knn.fit(X_train, y_train)

    # 4. Make predictions
    y_pred = knn.predict(X_test)

    # 5. Overall accuracy
    correct = np.sum(y_pred == y_test)
    accuracy = correct / len(y_test)
    print(f"Overall accuracy: {accuracy}")

    # 6. Accuracy per class
    for label_idx, class_name in enumerate(label_classes):
        # Find indices where true label equals current class
        class_mask = (y_test == label_idx)
        total_class = np.sum(class_mask)

        if total_class > 0:
            # Find correct predictions for this class
            correct_class = np.sum((y_test == label_idx) & (y_pred == label_idx))
            class_accuracy = correct_class / total_class
            print(f"Accuracy for class {class_name}: {class_accuracy:.2f}")


if __name__ == "__main__":
    main()