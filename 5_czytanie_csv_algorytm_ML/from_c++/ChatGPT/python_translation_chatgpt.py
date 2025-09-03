import pandas as pd
import numpy as np
from collections import Counter
from sklearn.model_selection import train_test_split
from sklearn.neighbors import NearestNeighbors

# 1. Load dataset (manually)
df = pd.read_csv("iris.csv")

# Assuming columns: sepal_length,sepal_width,petal_length,petal_width,species
features = df.iloc[:, 0:4].values
labels_str = df.iloc[:, 4].values

# Encode labels
label_map = {}
reverse_label_map = {}
current_label = 0
labels_int = []

for lbl in labels_str:
    if lbl not in label_map:
        label_map[lbl] = current_label
        reverse_label_map[current_label] = lbl
        current_label += 1
    labels_int.append(label_map[lbl])

labels_int = np.array(labels_int)

# 2. Train-test split (70% train, 30% test)
X_train, X_test, y_train, y_test = train_test_split(
    features, labels_int, test_size=0.3, random_state=42, shuffle=True
)

# 3. Train KNN model (k=5) — using NearestNeighbors to mimic raw search
k = 5
knn = NearestNeighbors(n_neighbors=k)
knn.fit(X_train)
distances, indices = knn.kneighbors(X_test)

# 4. Majority vote for each test point
y_pred = []
for neighbor_idxs in indices:
    neighbor_labels = y_train[neighbor_idxs]
    most_common = Counter(neighbor_labels).most_common(1)[0][0]
    y_pred.append(most_common)

y_pred = np.array(y_pred)

# 5. Overall accuracy
accuracy = np.mean(y_pred == y_test)
print(f"Overall accuracy: {accuracy:.4f}")

# 6. Accuracy per class
for label_int, class_name in reverse_label_map.items():
    mask = (y_test == label_int)
    correct_class = np.sum((y_pred == label_int) & mask)
    total_class = np.sum(mask)
    class_accuracy = correct_class / total_class if total_class > 0 else 0
    print(f"Accuracy for class {class_name}: {class_accuracy:.2f}")
