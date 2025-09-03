#!/usr/bin/env python3
"""
Flask REST API for User Management
Equivalent to the Spring Boot UserApiApplication
"""

from flask import Flask, request, jsonify
import json
import os
from typing import List, Dict, Optional

app = Flask(__name__)

DATA_FILE = "users.json"


class User:
    """User model class"""

    def __init__(self, user_id: int = None, name: str = None, email: str = None):
        self.id = user_id
        self.name = name
        self.email = email

    def to_dict(self) -> Dict:
        """Convert User object to dictionary"""
        return {
            "id": self.id,
            "name": self.name,
            "email": self.email
        }

    @classmethod
    def from_dict(cls, data: Dict) -> 'User':
        """Create User object from dictionary"""
        return cls(data.get('id'), data.get('name'), data.get('email'))


def load_users() -> List[User]:
    """Load users from JSON file"""
    try:
        if not os.path.exists(DATA_FILE):
            return []

        with open(DATA_FILE, 'r', encoding='utf-8') as file:
            data = json.load(file)
            return [User.from_dict(user_data) for user_data in data]
    except Exception as e:
        print(f"Error loading users: {e}")
        return []


def save_users(users: List[User]) -> None:
    """Save users to JSON file"""
    try:
        with open(DATA_FILE, 'w', encoding='utf-8') as file:
            json.dump([user.to_dict() for user in users], file, indent=2, ensure_ascii=False)
    except Exception as e:
        print(f"Error saving users: {e}")


@app.route('/users', methods=['GET'])
def get_users():
    """GET /users – get all users"""
    users = load_users()
    return jsonify([user.to_dict() for user in users])


@app.route('/users/<int:user_id>', methods=['GET'])
def get_user(user_id: int):
    """GET /users/{id} – get single user"""
    users = load_users()

    for user in users:
        if user.id == user_id:
            return jsonify(user.to_dict())

    return jsonify({"error": "User not found"}), 404


@app.route('/users', methods=['POST'])
def add_user():
    """POST /users – add new user"""
    data = request.get_json()

    if not data or not data.get('name') or not data.get('email'):
        return jsonify({"error": "Missing name or email"}), 400

    users = load_users()

    # Generate new ID
    new_id = max([user.id for user in users], default=0) + 1

    new_user = User(new_id, data['name'], data['email'])
    users.append(new_user)
    save_users(users)

    return jsonify(new_user.to_dict()), 201


@app.route('/users/<int:user_id>', methods=['PUT'])
def update_user(user_id: int):
    """PUT /users/{id} – update user"""
    data = request.get_json()
    users = load_users()

    for user in users:
        if user.id == user_id:
            if data.get('name') is not None:
                user.name = data['name']
            if data.get('email') is not None:
                user.email = data['email']

            save_users(users)
            return jsonify(user.to_dict())

    return jsonify({"error": "User not found"}), 404


@app.route('/users/<int:user_id>', methods=['DELETE'])
def delete_user(user_id: int):
    """DELETE /users/{id} – delete user"""
    users = load_users()

    for i, user in enumerate(users):
        if user.id == user_id:
            users.pop(i)
            save_users(users)
            return jsonify({"message": "User deleted"})

    return jsonify({"error": "User not found"}), 404


@app.errorhandler(404)
def not_found(error):
    """Handle 404 errors"""
    return jsonify({"error": "Endpoint not found"}), 404


@app.errorhandler(500)
def internal_error(error):
    """Handle 500 errors"""
    return jsonify({"error": "Internal server error"}), 500


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)