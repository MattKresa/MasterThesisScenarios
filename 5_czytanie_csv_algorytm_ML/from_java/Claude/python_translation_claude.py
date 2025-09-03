import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.neighbors import KNeighborsClassifier
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
from sklearn.preprocessing import LabelEncoder


def main():
    # 1. Load dataset
    data = pd.read_csv("iris.csv")

    # 2. Convert string labels to numerical (if necessary)
    # Assuming the last column is the species/class column
    X = data.iloc[:, :-1]  # All columns except the last one (features)
    y = data.iloc[:, -1]  # Last column (target/class)

    # Convert string labels to numerical if they're not already
    if y.dtype == 'object':
        label_encoder = LabelEncoder()
        y_encoded = label_encoder.fit_transform(y)
        class_names = label_encoder.classes_
    else:
        y_encoded = y
        class_names = np.unique(y)

    # 3. Train-test split (70% train, 30% test) with stratification
    X_train, X_test, y_train, y_test = train_test_split(
        X, y_encoded,
        test_size=0.3,
        random_state=42,  # For reproducibility
        stratify=y_encoded  # Stratified split
    )

    # 4. Train KNN model
    knn = KNeighborsClassifier(n_neighbors=5)  # K=5
    knn.fit(X_train, y_train)

    # 5. Make predictions
    y_pred = knn.predict(X_test)

    # 6. Evaluate model
    overall_accuracy = accuracy_score(y_test, y_pred)
    print(f"Overall accuracy: {overall_accuracy:.2f}")

    # 7. Accuracy per class
    print("\nAccuracy per class:")
    report = classification_report(y_test, y_pred, target_names=class_names, output_dict=True)

    for i, class_name in enumerate(class_names):
        # Using recall as per the original Java code comment
        class_accuracy = report[class_name]['recall']
        print(f"Accuracy for class {class_name}: {class_accuracy:.2f}")

    # 8. Additional detailed metrics (optional)
    print(f"\nDetailed Classification Report:")
    print(classification_report(y_test, y_pred, target_names=class_names))

    print(f"\nConfusion Matrix:")
    print(confusion_matrix(y_test, y_pred))


if __name__ == "__main__":
    main()