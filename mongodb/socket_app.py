from gevent import monkey
monkey.patch_all()

import os
from flask import Flask, render_template
from pymongo import MongoClient
from dotenv import load_dotenv
from flask_socketio import SocketIO, emit

load_dotenv('.env')

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
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
    if not data or 'device_id' not in data or 'event' not in data or 'timestamp' not in data:
      # Emits an event back to the sender informing something went wrong
      emit('error_presence', {"error": "Invalid JSON or missing fields"})
      return
            
    result = presence_collection.insert_one(data)
        
    print(f"[PIR_log] {data['event']} | ID: {result.inserted_id} | {data['timestamp']}")
        
    emit('presence_registered_successfully', {
      "mongo_id": str(result.inserted_id)
    })

  except Exception as e:
    emit('error_presence', {"error": str(e)})

@socketio.on('register_temperature')
def handle_register_temperature(data):
  try:
    if not data or 'device_id' not in data or 'temperature' not in data or 'timestamp' not in data:
      # Emits an event back to the sender informing something went wrong
      emit('error_temperature', {"error": "Invalid JSON or missing fields"})
      return
            
    result = temperature_collection.insert_one(data)
        
    print(f"[DHT_log] {data['temperature']} | ID: {result.inserted_id} | {data['timestamp']}")
        
    emit('temperature_registered_successfully', {
      "mongo_id": str(result.inserted_id)
    })

  except Exception as e:
    emit('error_temperature', {"error": str(e)})

@socketio.on('update_relay')
def handle_update_relay(data):
  try:
    if not data or 'device_id' not in data or 'relay_status' not in data:
      emit('error_relay_update', {"error": "Invalid JSON or missing fields"})
      return
  
    query = {"device_id": data["device_id"]}
    new_value = {"$set": {"relay_status": data['relay_status']}} 
    
    status_collection.update_one(query, new_value, upsert=True)

    print(f"[STATUS] Relay updated to: {data['relay_status']}")

  except Exception as e:
    emit('error_status_update', {"error": str(e)})

@socketio.on('update_temperature')
def handle_update_temperature(data):
  try:
    if not data or 'device_id' not in data or 'temperature' not in data:
      emit('error_temperature_update', {"error": "Invalid JSON or missing fields"})
      return
  
    query = {"device_id": data["device_id"]}
    new_value = {"$set": {"temperature": data['temperature']}} 
    
    status_collection.update_one(query, new_value, upsert=True)

    print(f"[STATUS] Temperature updated to: {data['temperature']}")

  except Exception as e:
    emit('error_temperature_update', {"error": str(e)})

@socketio.on('set_target_temperature')
def handle_set_target_temperature(data):
  try:
    if not data or 'device_id' not in data or 'target_temperature' not in data:
      emit('error_command', {"error": "Invalid JSON or missing fields"})
      return

    query = {"device_id": data["device_id"]}
    new_value = {"$set": {"target_temperature": data['target_temperature']}}
    
    status_collection.update_one(query, new_value, upsert=True)
    
    print(f"[COMANDO] Nova temperatura alvo para {data['device_id']}: {data['target_temperature']}°C")

    emit('command_target_temperature', {
      "device_id": data["device_id"],
      "target_temperature": data['target_temperature']
    }, broadcast=True)

  except Exception as e:
    emit('error_command', {"error": str(e)})


if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
