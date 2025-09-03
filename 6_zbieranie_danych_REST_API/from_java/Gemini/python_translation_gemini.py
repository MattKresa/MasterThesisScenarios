import json
import os
from typing import List, Optional, Dict, Any
from flask import Flask, jsonify, request, Response
from pydantic import BaseModel, Field


# --- Pydantic User Model ---
# This class defines the structure of a User and provides validation.
class User(BaseModel):
    id: Optional[int] = None
    name: str
    email: str


# --- Flask Application Setup ---
app = Flask(__name__)
DATA_FILE = 'users.json'


# --- Utility Functions for Data Persistence ---
def load_users() -> List[User]:
    """Loads a list of User objects from the JSON data file."""
    if not os.path.exists(DATA_FILE):
        return []
    try:
        with open(DATA_FILE, 'r') as f:
            data = json.load(f)
            return [User(**user_data) for user_data in data]
    except (IOError, json.JSONDecodeError) as e:
        print(f"Error loading users: {e}")
        return []


def save_users(users: List[User]):
    """Saves a list of User objects to the JSON data file."""
    try:
        with open(DATA_FILE, 'w') as f:
            # Pydantic's .model_dump() method converts the model to a dictionary
            # suitable for JSON serialization.
            json.dump([user.model_dump() for user in users], f, indent=4)
    except IOError as e:
        print(f"Error saving users: {e}")


# --- API Endpoints ---
@app.route('/users', methods=['GET'])
def get_users() -> Response:
    """GET /users - Returns a list of all users."""
    users = load_users()
    return jsonify([user.model_dump() for user in users])


@app.route('/users/<int:user_id>', methods=['GET'])
def get_user(user_id: int) -> Response:
    """GET /users/{id} - Returns a single user by ID."""
    users = load_users()
    user = next((u for u in users if u.id == user_id), None)
    if user:
        return jsonify(user.model_dump())
    return jsonify({"error": "User not found"}), 404


@app.route('/users', methods=['POST'])
def add_user() -> Response:
    """POST /users - Adds a new user."""
    try:
        user_data = User(**request.json)
    except Exception:
        return jsonify({"error": "Missing name or email"}), 400

    users = load_users()

    # Generate a new ID
    new_id = max((u.id for u in users), default=0) + 1
    user_data.id = new_id

    users.append(user_data)
    save_users(users)

    return jsonify(user_data.model_dump()), 201


@app.route('/users/<int:user_id>', methods=['PUT'])
def update_user(user_id: int) -> Response:
    """PUT /users/{id} - Updates an existing user."""
    users = load_users()
    for user in users:
        if user.id == user_id:
            try:
                update_data = request.json
                if 'name' in update_data:
                    user.name = update_data['name']
                if 'email' in update_data:
                    user.email = update_data['email']
                save_users(users)
                return jsonify(user.model_dump())
            except Exception:
                return jsonify({"error": "Invalid request body"}), 400
    return jsonify({"error": "User not found"}), 404


@app.route('/users/<int:user_id>', methods=['DELETE'])
def delete_user(user_id: int) -> Response:
    """DELETE /users/{id} - Deletes a user by ID."""
    users = load_users()
    initial_count = len(users)
    users = [u for u in users if u.id != user_id]

    if len(users) < initial_count:
        save_users(users)
        return jsonify({"message": "User deleted"})

    return jsonify({"error": "User not found"}), 404


# --- Main Entry Point ---
if __name__ == '__main__':
    app.run(debug=True)