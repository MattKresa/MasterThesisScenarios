from flask import Flask, request, jsonify, Response
import json
import os
from typing import List, Dict, Optional

app = Flask(__name__)

DATA_FILE = "users.json"


# Model użytkownika
class User:
    def __init__(self, id: int = None, name: str = None, email: str = None):
        self.id = id
        self.name = name
        self.email = email

    def to_dict(self) -> Dict:
        return {
            "id": self.id,
            "name": self.name,
            "email": self.email
        }

    @classmethod
    def from_dict(cls, data: Dict):
        return cls(data.get("id"), data.get("name"), data.get("email"))


# Załaduj użytkowników z pliku
def load_users() -> List[User]:
    try:
        if not os.path.exists(DATA_FILE):
            return []
        with open(DATA_FILE, "r") as file:
            users_data = json.load(file)
            return [User.from_dict(user_data) for user_data in users_data]
    except Exception as e:
        print(f"Error loading users: {e}")
        return []


# Zapisz użytkowników do pliku
def save_users(users: List[User]) -> None:
    try:
        with open(DATA_FILE, "w") as file:
            json.dump([user.to_dict() for user in users], file, indent=2)
    except Exception as e:
        print(f"Error saving users: {e}")


@app.route('/users', methods=['GET'])
def get_users() -> Response:
    users = load_users()
    return jsonify([user.to_dict() for user in users])


@app.route('/users/<int:id>', methods=['GET'])
def get_user(id: int) -> Response:
    users = load_users()
    user = next((u for u in users if u.id == id), None)
    if user:
        return jsonify(user.to_dict())
    else:
        return jsonify({"error": "User not found"}), 404


@app.route('/users', methods=['POST'])
def add_user() -> Response:
    data = request.get_json()
    if not data or 'name' not in data or 'email' not in data:
        return jsonify({"error": "Missing name or email"}), 400

    users = load_users()
    new_id = max((user.id for user in users), default=0) + 1
    new_user = User(new_id, data['name'], data['email'])
    users.append(new_user)
    save_users(users)
    return jsonify(new_user.to_dict()), 201


@app.route('/users/<int:id>', methods=['PUT'])
def update_user(id: int) -> Response:
    data = request.get_json()
    users = load_users()

    for user in users:
        if user.id == id:
            if 'name' in data:
                user.name = data['name']
            if 'email' in data:
                user.email = data['email']
            save_users(users)
            return jsonify(user.to_dict())

    return jsonify({"error": "User not found"}), 404


@app.route('/users/<int:id>', methods=['DELETE'])
def delete_user(id: int) -> Response:
    users = load_users()
    initial_length = len(users)
    users = [user for user in users if user.id != id]

    if len(users) < initial_length:
        save_users(users)
        return jsonify({"message": "User deleted"})
    else:
        return jsonify({"error": "User not found"}), 404


if __name__ == '__main__':
    app.run(debug=True)