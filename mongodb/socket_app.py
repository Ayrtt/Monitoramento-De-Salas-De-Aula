from gevent import monkey
monkey.patch_all()

import os
from flask import Flask, render_template
from pymongo import MongoClient
from dotenv import load_dotenv
from flask_socketio import SocketIO, emit

load_dotenv('.env')

app = Flask(__name__)
app.config['SECRET_KEY'] = ''
socketio = SocketIO(app, cors_allowed_origins="*")
connection_string = os.environ.get("MONGO_DB_CONN_STRING")

try:
    client = MongoClient(connection_string)
    client.admin.command('ping')
    print("-- Successfully connected to MongoDB!")
except Exception as e:
    print(f"-- Error connecting to Mongo: {e}")

db = client["te"] #tcc_iot

presence_collection = db["presence_logs"]
temperature_collection = db["temperature_logs"]
status_collection = db["device_status"]

@socketio.on('register_presence')
def handle_register_presence(data):
    try:
        if not data or 'device_id' not in data or 'event' not in data:
            # Emits an event back to the sender informing something went wrong
            emit('error_presence', {"error": "Invalid JSON or missing fields"})
            return
            
        result = presence_collection.insert_one(data)
        
        print(f"New event received: {data['event']} | ID: {result.inserted_id}")
        
        emit('presence_registered_successfully', {
            "message": "Presence log saved!",
            "mongo_id": str(result.inserted_id)
        })

    except Exception as e:
        emit('error_presence', {"error": str(e)})
    
if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
