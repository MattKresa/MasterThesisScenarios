import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder
from sklearn.neighbors import KNeighborsClassifier
from sklearn.metrics import accuracy_score

# 1. Load the dataset
df = pd.read_csv("iris.csv")

# 2. Separate features and labels
X = df.drop("species", axis=1)
y = LabelEncoder().fit_transform(df["species"])
class_names = df["species"].unique()

# 3. Train-test split
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.3, random_state=42, stratify=y
)

# 4. Train KNN model
model = KNeighborsClassifier(n_neighbors=5)
model.fit(X_train, y_train)

# 5. Predictions
y_pred = model.predict(X_test)

# 6. Overall accuracy
print("Overall accuracy:", accuracy_score(y_test, y_pred))

# 7. Accuracy per class
for i, cls in enumerate(class_names):
    correct = sum((y_pred == y_test) & (y_test == i))
    total = sum(y_test == i)
    print(f"Accuracy for class {cls}: {correct / total:.2f}")
