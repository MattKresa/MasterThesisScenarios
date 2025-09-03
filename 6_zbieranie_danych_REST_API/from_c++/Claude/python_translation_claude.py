from flask import Flask, request, jsonify
import json
import os
from typing import List, Dict, Optional

app = Flask(__name__)

DATA_FILE = "users.json"


def load_users() -> List[Dict]:
    """Load users from JSON file"""
    if not os.path.exists(DATA_FILE):
        return []

    try:
        with open(DATA_FILE, 'r', encoding='utf-8') as file:
            users = json.load(file)
            return users if isinstance(users, list) else []
    except (json.JSONDecodeError, IOError):
        return []


def save_users(users: List[Dict]) -> None:
    """Save users to JSON file"""
    with open(DATA_FILE, 'w', encoding='utf-8') as file:
        json.dump(users, file, indent=4, ensure_ascii=False)


def find_user_by_id(users: List[Dict], user_id: int) -> Optional[Dict]:
    """Find user by ID"""
    for user in users:
        if user.get("id") == user_id:
            return user
    return None


@app.route('/users', methods=['GET'])
def get_users():
    """GET /users – list of users"""
    users = load_users()
    return jsonify(users)


@app.route('/users/<int:user_id>', methods=['GET'])
def get_user(user_id: int):
    """GET /users/<id> – single user"""
    users = load_users()
    user = find_user_by_id(users, user_id)

    if user:
        return jsonify(user)
    else:
        return jsonify({"error": "User not found"}), 404


@app.route('/users', methods=['POST'])
def create_user():
    """POST /users – add user"""
    try:
        data = request.get_json()
        if not data or not isinstance(data, dict):
            return jsonify({"error": "Invalid JSON"}), 400

        if "name" not in data or "email" not in data:
            return jsonify({"error": "Missing name or email"}), 400

        users = load_users()

        # Find the highest ID and increment
        new_id = 0
        for user in users:
            if "id" in user:
                new_id = max(new_id, user["id"])
        new_id += 1

        new_user = {
            "id": new_id,
            "name": data["name"],
            "email": data["email"]
        }

        users.append(new_user)
        save_users(users)

        return jsonify(new_user), 201

    except Exception:
        return jsonify({"error": "Invalid JSON"}), 400


@app.route('/users/<int:user_id>', methods=['PUT'])
def update_user(user_id: int):
    """PUT /users/<id> – update user"""
    try:
        data = request.get_json()
        if not data or not isinstance(data, dict):
            return jsonify({"error": "Invalid JSON"}), 400

        users = load_users()
        user = find_user_by_id(users, user_id)

        if not user:
            return jsonify({"error": "User not found"}), 404

        # Update user fields if provided
        if "name" in data:
            user["name"] = data["name"]
        if "email" in data:
            user["email"] = data["email"]

        save_users(users)
        return jsonify(user)

    except Exception:
        return jsonify({"error": "Invalid JSON"}), 400


@app.route('/users/<int:user_id>', methods=['DELETE'])
def delete_user(user_id: int):
    """DELETE /users/<id> – delete user"""
    users = load_users()
    initial_count = len(users)

    # Remove user with matching ID
    users[:] = [user for user in users if user.get("id") != user_id]

    if len(users) == initial_count:
        return jsonify({"error": "User not found"}), 404

    save_users(users)
    return jsonify({"message": "User deleted"})


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True, threaded=True)