from gevent import monkey
monkey.patch_all()

import os
from flask import Flask
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
    print(f"-- Error connecting to MongoDB: {e}")

db = client["test"] #tcc_iot

presence_collection = db["presence_logs"]
temperature_collection = db["temperature_logs"]
status_collection = db["device_status"]



#---------------log registration---------------
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



#---------------update status---------------
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

@socketio.on('update_ac_status')
def handle_update_ac_status(data):
  try:
    if not data or 'device_id' not in data or 'ac_status' not in data:
      emit('error_relay_update', {"error": "Invalid JSON or missing fields"})
      return
  
    query = {"device_id": data["device_id"]}
    new_value = {"$set": {"ac_status": data['ac_status']}} 
    
    status_collection.update_one(query, new_value, upsert=True)

    print(f"[STATUS] AC updated to: {data['ac_status']}")

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



#---------------front-end commands---------------
@socketio.on('set_ac_command')
def handle_set_ac_command(data):
  try:
    if not data or 'device_id' not in data or 'new_ac_command' not in data:
      emit('error_command', {"error": "Invalid JSON or missing fields"})
      return

    query = {"device_id": data["device_id"]}
    new_value = {"$set": {"ac_command": data['new_ac_command']}}
    
    status_collection.update_one(query, new_value, upsert=True)
    
    print(f"[COMMAND] Nova temperatura alvo para {data['device_id']}: {data['new_ac_command']}°C")

    emit('ac_command', {
      "device_id": data["device_id"],
      "ac_command": data['new_ac_command']
    }, broadcast=True)

  except Exception as e:
    emit('error_command', {"error": str(e)})

@socketio.on('set_relay_command')
def handle_set_relay_command(data):
  try:
    if not data or 'device_id' not in data or 'new_relay_command' not in data:
      emit('error_command', {"error": "Invalid JSON or missing fields"})
      return

    query = {"device_id": data["device_id"]}
    new_value = {"$set": {"relay_command": data['new_relay_command']}}
    
    status_collection.update_one(query, new_value, upsert=True)
    
    print(f"[COMMAND] Novo estado para {data['device_id']}: {data['new_relay_command']}")

    emit('relay_command', {
      "device_id": data["device_id"],
      "relay_command": data['new_relay_command']
    }, broadcast=True)

  except Exception as e:
    emit('error_command', {"error": str(e)})



#---------------back-end response to front-end commands---------------
@socketio.on('ac_command_response')
def handle_ac_response(data):
  try:
    if not data or 'device_id' not in data or 'ac_response' not in data:
      emit('error_ac_response_update', {"error": "Invalid JSON or missing fields"})
      return
  
    query = {"device_id": data["device_id"]}
    new_value = {"$set": {"ac_command_response": data['ac_response']}} 
    
    status_collection.update_one(query, new_value, upsert=True)

    print(f"[RESPONSE] AC response updated to: {data['ac_response']}")

  except Exception as e:
    emit('error_ac_response_update', {"error": str(e)})

@socketio.on('relay_command_response')
def handle_relay_response(data):
  try:
    if not data or 'device_id' not in data or 'relay_response' not in data:
      emit('error_relay_response_update', {"error": "Invalid JSON or missing fields"})
      return
  
    query = {"device_id": data["device_id"]}
    new_value = {"$set": {"relay_command_response": data['relay_response']}} 
    
    status_collection.update_one(query, new_value, upsert=True)

    print(f"[RESPONSE] Relay response updated to: {data['relay_response']}")

  except Exception as e:
    emit('error_relay_response_update', {"error": str(e)})

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
