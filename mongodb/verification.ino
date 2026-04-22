/* device_status Document:
*_id: id
*device_id: 0001
*relay_status: bool
*temperature: int
*ac_command: int
*ac_command_response: int
*relay_command: bool
*relay_command_response: bool
*/

void verificationAC(int temperature) {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  array.add("verification_ac");

  JsonObject param1 = array.createNestedObject();
  param1["device_id"] = deviceID;
  param1["ac_command_response"] = temperature;

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);

  Serial.print("AC Command Response: ");
  Serial.println(output);
}

void verificationRelay() {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  array.add("verification_relay");

  JsonObject param1 = array.createNestedObject();
  param1["device_id"] = deviceID;
  param1["relay_command_response"] = digitalRead(RELAY_PIN);

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);

  Serial.print("Relay Command Response: ");
  Serial.println(output);
}

// Required function to keep the library Socket.IO working internally
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.println("[WebSocket] Disconnected!");
      break;
    case sIOtype_CONNECT:
      Serial.printf("[WebSocket] Connected! URL: %s\n", payload);
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
    {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("[WebSocket] Error while reading json received: ");
        Serial.println(error.c_str());
        return;
      }

      String eventName = doc[0].as<String>();
      JsonObject data = doc[1];
      String id_received = data["device_id"].as<String>();

      if (id_received == deviceID) {
        if (eventName == "command_target_temperature"){
          target_temperature = data["target_temperature"].as<int>();
          Serial.printf("[COMMAND] Setting temperature to: %d\n", target_temperature);
          if (target_temperature == 0){
            sendCodeACOff();
          }
          else{
            sendCodeAC(target_temperature);
          }
        }
        else if (eventName == "command_relay_status") {
          bool new_relay_status = data["relay_status"].as<bool>();
          Serial.printf("[COMMAND] Setting relay to: %s\n", new_relay_status ? "ON" : "OFF");
          if (new_relay_status) {
            digitalWrite(RELAY_PIN, RELAY_ON);
          } else {
            digitalWrite(RELAY_PIN, RELAY_OFF);
          }
          verificationRelay();
        }
        else {
          Serial.printf("[WebSocket] Server response: %s\n", payload);
        }
      }
      break;
    }
  }
}

void sendCodeAC(int temperature){
  Serial.printf("[IR] Sending %d°C signal\n", temperature);

  int index = temperature - 16;

  if (index >= 0 && index < 16) {
    const uint16_t* signal_data = array_ac_signals_on[index].rawData;
    size_t signal_size = array_ac_signals_on[index].length;

    if (signal_size > 0) {
      sendConvertedRaw(signal_data, signal_size);
      Serial.println("[IR] Code sent.");
    } else {
      Serial.println("[WARNING IR] Empty code!");
    }
  } else {
    Serial.printf("[ERROR IR] Temperature %d outside of range.\n", temperature);
  }

  verificationAC(index+16);
}

void sendCodeACOff() {
  Serial.println("[IR] Turning AC off");

  const uint16_t* signal_data = array_ac_signal_off.rawData;
  size_t signal_size = array_ac_signal_off.length;

  if (signal_size > 0) {
    sendConvertedRaw(signal_data, signal_size);
    Serial.println("[IR] OFF Code sent.");
  } else {
    Serial.println("[WARNING IR] Empty OFF code");
  }

  verificationAC(0);
}
