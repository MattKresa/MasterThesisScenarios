from flask import Flask, request, jsonify
import json
import os

app = Flask(__name__)
DATA_FILE = "users.json"


# Help functions
def load_users():
    if not os.path.exists(DATA_FILE):
        return []
    with open(DATA_FILE, "r") as f:
        return json.load(f)


def save_users(users):
    with open(DATA_FILE, "w") as f:
        json.dump(users, f, indent=4)


# Endpoint GET - user list
@app.route("/users", methods=["GET"])
def get_users():
    users = load_users()
    return jsonify(users), 200


# Endpoint GET – single user
@app.route("/users/<int:user_id>", methods=["GET"])
def get_user(user_id):
    users = load_users()
    user = next((u for u in users if u["id"] == user_id), None)
    if user:
        return jsonify(user), 200
    return jsonify({"error": "User not found"}), 404


# Endpoint POST – add users
@app.route("/users", methods=["POST"])
def add_user():
    data = request.get_json()
    if not data or "name" not in data or "email" not in data:
        return jsonify({"error": "Missing name or email"}), 400

    users = load_users()
    new_id = max([u["id"] for u in users], default=0) + 1
    new_user = {"id": new_id, "name": data["name"], "email": data["email"]}
    users.append(new_user)
    save_users(users)

    return jsonify(new_user), 201


# Endpoint PUT – edit user
@app.route("/users/<int:user_id>", methods=["PUT"])
def update_user(user_id):
    data = request.get_json()
    users = load_users()
    for u in users:
        if u["id"] == user_id:
            u["name"] = data.get("name", u["name"])
            u["email"] = data.get("email", u["email"])
            save_users(users)
            return jsonify(u), 200

    return jsonify({"error": "User not found"}), 404


# Endpoint DELETE – delete user
@app.route("/users/<int:user_id>", methods=["DELETE"])
def delete_user(user_id):
    users = load_users()
    new_users = [u for u in users if u["id"] != user_id]
    if len(new_users) == len(users):
        return jsonify({"error": "User not found"}), 404

    save_users(new_users)
    return jsonify({"message": "User deleted"}), 200


if __name__ == "__main__":
    app.run(debug=True)
