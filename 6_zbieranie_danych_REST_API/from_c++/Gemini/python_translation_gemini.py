import json
import os
from flask import Flask, jsonify, request

# The name of the JSON file for user data.
DATA_FILE = "users.json"

# Initialize the Flask application.
app = Flask(__name__)


def load_users():
    """
    Loads user data from the DATA_FILE.

    If the file does not exist, it returns an empty list, mirroring the C++ code.
    This prevents file not found errors when the application starts for the first time.

    Returns:
        list: A list of user dictionaries.
    """
    if not os.path.exists(DATA_FILE):
        return []
    with open(DATA_FILE, 'r') as file:
        try:
            return json.load(file)
        except json.JSONDecodeError:
            # Handle empty or invalid JSON file by returning an empty list.
            return []


def save_users(users):
    """
    Saves the list of users to the DATA_FILE.

    Args:
        users (list): The list of user dictionaries to save.
    """
    with open(DATA_FILE, 'w') as file:
        # Use a high indentation level for readability.
        json.dump(users, file, indent=4)


@app.route("/users", methods=["GET"])
def get_all_users():
    """
    GET /users
    Returns a list of all users.
    """
    users = load_users()
    return jsonify(users)


@app.route("/users/<int:user_id>", methods=["GET"])
def get_user_by_id(user_id):
    """
    GET /users/<id>
    Returns a single user by their ID.

    Args:
        user_id (int): The ID of the user to retrieve.
    """
    users = load_users()
    # Find the user using a list comprehension for clarity.
    user = next((u for u in users if u["id"] == user_id), None)
    if user:
        return jsonify(user)
    # If the user is not found, return a 404 Not Found response.
    return jsonify({"error": "User not found"}), 404


@app.route("/users", methods=["POST"])
def add_user():
    """
    POST /users
    Adds a new user. The request body must contain 'name' and 'email'.
    """
    data = request.get_json()
    if not data or 'name' not in data or 'email' not in data:
        # Return a 400 Bad Request error if required fields are missing.
        return jsonify({"error": "Missing name or email"}), 400

    users = load_users()

    # Determine the next available ID.
    new_id = 1
    if users:
        new_id = max(u['id'] for u in users) + 1

    new_user = {
        "id": new_id,
        "name": data["name"],
        "email": data["email"]
    }

    users.append(new_user)
    save_users(users)

    # Return a 201 Created status code.
    return jsonify(new_user), 201


@app.route("/users/<int:user_id>", methods=["PUT"])
def update_user(user_id):
    """
    PUT /users/<id>
    Updates an existing user.

    Args:
        user_id (int): The ID of the user to update.
    """
    data = request.get_json()
    if not isinstance(data, dict):
        return jsonify({"error": "Invalid JSON"}), 400

    users = load_users()

    for user in users:
        if user["id"] == user_id:
            # Update name and/or email if they are present in the request data.
            if 'name' in data:
                user['name'] = data['name']
            if 'email' in data:
                user['email'] = data['email']
            save_users(users)
            return jsonify(user)

    # Return a 404 Not Found if the user doesn't exist.
    return jsonify({"error": "User not found"}), 404


@app.route("/users/<int:user_id>", methods=["DELETE"])
def delete_user(user_id):
    """
    DELETE /users/<id>
    Deletes a user by their ID.

    Args:
        user_id (int): The ID of the user to delete.
    """
    users = load_users()

    initial_count = len(users)

    # Filter the list to remove the user with the specified ID.
    users = [u for u in users if u["id"] != user_id]

    if len(users) == initial_count:
        # If the user list size is unchanged, the user was not found.
        return jsonify({"error": "User not found"}), 404

    save_users(users)

    return jsonify({"message": "User deleted"})


if __name__ == "__main__":
    # Run the application on port 5000.
    app.run(port=5000)
