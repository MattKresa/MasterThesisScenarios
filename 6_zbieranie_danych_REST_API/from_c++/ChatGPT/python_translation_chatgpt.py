from flask import Flask, request, jsonify
import json
import os

DATA_FILE = "users.json"
app = Flask(__name__)

# Load users from file
def load_users():
    if not os.path.exists(DATA_FILE):
        return []
    with open(DATA_FILE, "r", encoding="utf-8") as f:
        return json.load(f)

# Save users to file
def save_users(users):
    with open(DATA_FILE, "w", encoding="utf-8") as f:
        json.dump(users, f, indent=4)

# GET /users – list all users
@app.route("/users", methods=["GET"])
def get_users():
    users = load_users()
    return jsonify(users)

# GET /users/<id> – get a single user
@app.route("/users/<int:user_id>", methods=["GET"])
def get_user(user_id):
    users = load_users()
    user = next((u for u in users if u["id"] == user_id), None)
    if user:
        return jsonify(user)
    return jsonify({"error": "User not found"}), 404

# POST /users – add a new user
@app.route("/users", methods=["POST"])
def create_user():
    data = request.get_json()
    if not data or "name" not in data or "email" not in data:
        return jsonify({"error": "Missing name or email"}), 400

    users = load_users()
    new_id = max((u["id"] for u in users), default=0) + 1
    new_user = {"id": new_id, "name": data["name"], "email": data["email"]}
    users.append(new_user)
    save_users(users)
    return jsonify(new_user), 201

# PUT /users/<id> – update user
@app.route("/users/<int:user_id>", methods=["PUT"])
def update_user(user_id):
    data = request.get_json()
    if not data:
        return jsonify({"error": "Invalid JSON"}), 400

    users = load_users()
    for u in users:
        if u["id"] == user_id:
            if "name" in data:
                u["name"] = data["name"]
            if "email" in data:
                u["email"] = data["email"]
            save_users(users)
            return jsonify(u)
    return jsonify({"error": "User not found"}), 404

# DELETE /users/<id> – delete user
@app.route("/users/<int:user_id>", methods=["DELETE"])
def delete_user(user_id):
    users = load_users()
    before = len(users)
    users = [u for u in users if u["id"] != user_id]
    if len(users) == before:
        return jsonify({"error": "User not found"}), 404

    save_users(users)
    return jsonify({"message": "User deleted"})

if __name__ == "__main__":
    app.run(port=5000, threaded=True)
