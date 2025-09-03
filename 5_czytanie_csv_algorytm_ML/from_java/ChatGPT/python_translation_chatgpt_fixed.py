import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
from sklearn.neighbors import KNeighborsClassifier
from sklearn.metrics import accuracy_score, recall_score

# 1. Load dataset
data = pd.read_csv("iris.csv")

# 2. Convert string labels to numeric (like Weka's StringToNominal)
# Assuming the last column is the species/label
label_encoder = LabelEncoder()
# data.iloc[:, -1] = label_encoder.fit_transform(data.iloc[:, -1])

# 3. Split features and target
X = data.iloc[:, :-1]
y = label_encoder.fit_transform(data.iloc[:, -1])

# 4. Train-test split (70% train, 30% test, stratified, reproducible)
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.3, random_state=42, stratify=y
)

# 5. Train KNN model (K=5)
knn = KNeighborsClassifier(n_neighbors=5)
knn.fit(X_train, y_train)

# 6. Predictions
y_pred = knn.predict(X_test)

# 7. Overall accuracy
overall_accuracy = accuracy_score(y_test, y_pred)
print(f"Overall accuracy: {overall_accuracy:.2f}")

# 8. Accuracy per class (recall per class)
recalls = recall_score(y_test, y_pred, average=None)
for class_idx, recall_val in enumerate(recalls):
    class_name = label_encoder.inverse_transform([class_idx])[0]
    print(f"Accuracy for class {class_name}: {recall_val:.2f}")
