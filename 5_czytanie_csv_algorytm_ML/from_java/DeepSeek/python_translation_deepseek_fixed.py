import pandas as pd
from sklearn.preprocessing import LabelEncoder
from sklearn.model_selection import train_test_split
from sklearn.neighbors import KNeighborsClassifier
from sklearn.metrics import accuracy_score, recall_score


def iris_knn():
    # 1. Load dataset
    data = pd.read_csv("iris.csv")

    # 2. Convert string to nominal (if necessary)
    # Assuming species is the last column
    label_encoder = LabelEncoder()
    #data.iloc[:, -1] = label_encoder.fit_transform(data.iloc[:, -1])

    # 3. Set class index to the label attribute
    X = data.iloc[:, :-1]  # features
    y = label_encoder.fit_transform(data.iloc[:, -1])  # target

    # 4. Train-test split (70% train, 30% test)
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.3, random_state=42, stratify=y
    )

    # 5. Train KNN model
    knn = KNeighborsClassifier(n_neighbors=5)  # K=5
    knn.fit(X_train, y_train)

    # 6. Evaluate model
    y_pred = knn.predict(X_test)

    # 7. Overall accuracy
    overall_accuracy = accuracy_score(y_test, y_pred)
    print(f"Overall accuracy: {overall_accuracy:.2f}")

    # 8. Accuracy per class (recall)
    for i, class_name in enumerate(label_encoder.classes_):
        class_recall = recall_score(y_test, y_pred, average=None)[i]
        print(f"Accuracy for class {class_name}: {class_recall:.2f}")


if __name__ == "__main__":
    iris_knn()