# --- All-in-One Flask Application (app.py) ---
from flask import Flask, jsonify, request
import json
import os

app = Flask(__name__)

DATA_FILE = "users.json"


# User model (as dicts in Python)
class User:
    def __init__(self, id, name, email):
        self.id = id
        self.name = name
        self.email = email

    def to_dict(self):
        return {"id": self.id, "name": self.name, "email": self.email}


# Load users from file
def load_users():
    if not os.path.exists(DATA_FILE):
        return []
    try:
        with open(DATA_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception as e:
        print(f"Error loading users: {e}")
        return []


# Save users to file
def save_users(users):
    try:
        with open(DATA_FILE, "w", encoding="utf-8") as f:
            json.dump(users, f, indent=4)
    except Exception as e:
        print(f"Error saving users: {e}")


# GET /users – list
@app.route("/users", methods=["GET"])
def get_users():
    return jsonify(load_users())


# GET /users/<id> – single user
@app.route("/users/<int:user_id>", methods=["GET"])
def get_user(user_id):
    users = load_users()
    for user in users:
        if user["id"] == user_id:
            return jsonify(user), 200
    return jsonify({"error": "User not found"}), 404


# POST /users – add
@app.route("/users", methods=["POST"])
def add_user():
    data = request.get_json()
    if not data or "name" not in data or "email" not in data:
        return jsonify({"error": "Missing name or email"}), 400

    users = load_users()
    new_id = max((u["id"] for u in users), default=0) + 1
    new_user = {"id": new_id, "name": data["name"], "email": data["email"]}
    users.append(new_user)
    save_users(users)
    return jsonify(new_user), 201


# PUT /users/<id> – edit
@app.route("/users/<int:user_id>", methods=["PUT"])
def update_user(user_id):
    data = request.get_json()
    users = load_users()

    for user in users:
        if user["id"] == user_id:
            if "name" in data:
                user["name"] = data["name"]
            if "email" in data:
                user["email"] = data["email"]
            save_users(users)
            return jsonify(user), 200

    return jsonify({"error": "User not found"}), 404


# DELETE /users/<id> – delete
@app.route("/users/<int:user_id>", methods=["DELETE"])
def delete_user(user_id):
    users = load_users()
    new_users = [u for u in users if u["id"] != user_id]

    if len(new_users) != len(users):
        save_users(new_users)
        return jsonify({"message": "User deleted"}), 200
    else:
        return jsonify({"error": "User not found"}), 404


if __name__ == "__main__":
    app.run(debug=True)
