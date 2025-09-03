from flask import Flask, request, jsonify, make_response
import json
import os
from typing import List, Dict, Any

app = Flask(__name__)
DATA_FILE = "users.json"


def load_users() -> List[Dict[str, Any]]:
    if not os.path.exists(DATA_FILE):
        return []
    with open(DATA_FILE, 'r') as file:
        try:
            return json.load(file)
        except json.JSONDecodeError:
            return []


def save_users(users: List[Dict[str, Any]]) -> None:
    with open(DATA_FILE, 'w') as file:
        json.dump(users, file, indent=4)


@app.route('/users', methods=['GET'])
def get_users():
    users = load_users()
    return jsonify(users)


@app.route('/users/<int:user_id>', methods=['GET'])
def get_user(user_id: int):
    users = load_users()
    user = next((u for u in users if u.get('id') == user_id), None)
    if user:
        return jsonify(user)
    return make_response(jsonify({"error": "User not found"}), 404)


@app.route('/users', methods=['POST'])
def add_user():
    data = request.get_json()
    if not data or 'name' not in data or 'email' not in data:
        return make_response(jsonify({"error": "Missing name or email"}), 400)

    users = load_users()
    new_id = max((u.get('id', 0) for u in users), default=0) + 1
    new_user = {
        "id": new_id,
        "name": data['name'],
        "email": data['email']
    }
    users.append(new_user)
    save_users(users)

    return make_response(jsonify(new_user), 201)


@app.route('/users/<int:user_id>', methods=['PUT'])
def update_user(user_id: int):
    data = request.get_json()
    if not data:
        return make_response(jsonify({"error": "Invalid JSON"}), 400)

    users = load_users()
    for user in users:
        if user.get('id') == user_id:
            if 'name' in data:
                user['name'] = data['name']
            if 'email' in data:
                user['email'] = data['email']
            save_users(users)
            return jsonify(user)

    return make_response(jsonify({"error": "User not found"}), 404)


@app.route('/users/<int:user_id>', methods=['DELETE'])
def delete_user(user_id: int):
    users = load_users()
    initial_length = len(users)
    users = [u for u in users if u.get('id') != user_id]

    if len(users) == initial_length:
        return make_response(jsonify({"error": "User not found"}), 404)

    save_users(users)
    return jsonify({"message": "User deleted"})


if __name__ == '__main__':
    app.run(port=5000, threaded=True)