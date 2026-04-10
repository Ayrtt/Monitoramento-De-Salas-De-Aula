import os
from flask import Flask, jsonify, request
from pymongo import MongoClient
from dotenv import load_dotenv, find_dotenv

load_dotenv('.env')

app = Flask(__name__)
connection_string = os.environ.get("MONGO_DB_CONN_STRING")

try:
    client = MongoClient(connection_string)
    client.admin.command('ping')
    print("-- Successfully connected to MongoDB!")
except Exception as e:
    print(f"-- Error connecting to Mongo: {e}")

db = client["MonitoramentoSalaDeAula"]

presence_collection = db["presence_logs"]
temperature_collection = db["temperature_logs"]
status_collection = db["device_status"]

# ==========================================
# ROUTE: Test connection (GET)
# ==========================================
@app.route('/', methods=['GET'])
def index():
    return jsonify({"message": "TCC API running perfectly in the local environment!"}), 200

# ==========================================
# ROUTE: Receive PIR Sensor data (POST)
# ==========================================
@app.route('/api/sensors/presence', methods=['POST'])
def register_presence():
    try:
        data = request.json
        
        if not data or 'device_id' not in data or 'event' not in data:
            return jsonify({"error": "Invalid JSON or missing fields"}), 400
            
        result = presence_collection.insert_one(data)
        
        print(f"New event received: {data['event']} | ID: {result.inserted_id}")
        
        return jsonify({
            "message": "Presence log saved!",
            "mongo_id": str(result.inserted_id)
        }), 201

    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
# ==========================================
# ROUTE FOR DASHBOARD: Update Relay (PATCH)
# ==========================================
@app.route('/api/devices/<device_id>/status', methods=['PATCH'])
def update_status(device_id):
    try:
        data = request.json
        
        # $set: update only the sent fields
        # upsert=True: "If device 0001 doesn't exist, create it now"
        result = status_collection.update_one(
            {"device_id": device_id}, 
            {"$set": data}, 
            upsert=True
        )
        
        print(f"Status updated for device {device_id}")
        return jsonify({"message": "Status updated successfully!"}), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500    
    
# ==========================================
# ROUTE: Receive DHT11 Sensor data (POST)
# ==========================================
@app.route('/api/sensors/temperature', methods=['POST'])
def register_temperature():
    try:
        data = request.json
        
        # Validation: Ensures vital data arrived
        if not data or 'device_id' not in data or 'temperature' not in data:
            return jsonify({"error": "Missing device_id, temperature"}), 400
            
        # Saves the document in the collection
        result = temperature_collection.insert_one(data)
        
        print(f"New temperature data received: {data['temperature']}°C | ID: {result.inserted_id}")
        
        return jsonify({
            "message": "Temperature log saved!",
            "mongo_id": str(result.inserted_id)
        }), 201

    except Exception as e:
        return jsonify({"error": str(e)}), 500

# ==========================================
# ROUTE FOR ESP32: Read Relay (GET)
# ==========================================
@app.route('/api/devices/<device_id>/status', methods=['GET'])
def read_status(device_id):
    try:
        # Searches the DB for the exact document of the requested ESP32
        status = status_collection.find_one({"device_id": device_id})
        
        if status:
            status['_id'] = str(status['_id']) # Converts ID to string
            return jsonify(status), 200
        else:
            return jsonify({"error": "Device not found"}), 404

    except Exception as e:
        return jsonify({"error": str(e)}), 500
