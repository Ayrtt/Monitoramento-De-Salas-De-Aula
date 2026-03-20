import os
from flask import Flask, jsonify, request # Adicionamos o 'request' aqui!
from pymongo import MongoClient
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__)
string_conexao = os.getenv("MONGO_DB_CONN_STRING")

try:
    cliente = MongoClient(string_conexao)
    cliente.admin.command('ping')
    print("--Conectado ao MongoDB Local com sucesso!")
except Exception as e:
    print(f"--Erro ao conectar no Mongo: {e}")

db = cliente["tcc_iot"]
colecao_presenca = db["logs_presenca"]
colecao_temperatura = db["logs_temperatura"]
colecao_status = db["status_dispositivos"]

# Rota de teste antiga (GET)
@app.route('/', methods=['GET'])
def index():
    return jsonify({"mensagem": "API do TCC rodando perfeitamente no ambiente local!"}), 200

# ==========================================
# ROTA: Receber dados do Sensor PIR (POST)
# ==========================================
@app.route('/api/sensores/presenca', methods=['POST'])
def registrar_presenca():
    try:
        dados = request.json
        
        if not dados or 'device_id' not in dados or 'evento' not in dados:
            return jsonify({"erro": "JSON inválido ou faltando campos"}), 400
            
        resultado = colecao_presenca.insert_one(dados)
        
        print(f"Novo evento recebido: {dados['evento']} | ID: {resultado.inserted_id}")
        
        return jsonify({
            "mensagem": "Log de presença salvo!",
            "id_mongo": str(resultado.inserted_id)
        }), 201

    except Exception as e:
        return jsonify({"erro": str(e)}), 500
    
# ==========================================
# NOVA ROTA: Receber dados do Sensor DHT11 (POST)
# ==========================================
@app.route('/api/sensores/clima', methods=['POST'])
def registrar_clima():
    try:
        dados = request.json
        
        # Validação: Garante que os dados vitais chegaram
        if not dados or 'device_id' not in dados or 'temperatura' not in dados or 'umidade' not in dados:
            return jsonify({"erro": "Faltando device_id, temperatura ou umidade"}), 400
            
        # Salva o documento na coleção
        resultado = colecao_temperatura.insert_one(dados)
        
        print(f"🌡️ Novo clima recebido: {dados['temperatura']}°C | ID: {resultado.inserted_id}")
        
        return jsonify({
            "mensagem": "Log de clima salvo!",
            "id_mongo": str(resultado.inserted_id)
        }), 201

    except Exception as e:
        return jsonify({"erro": str(e)}), 500
    
# ==========================================
# ROTA PARA O DASHBOARD: Atualizar o Relé (PUT)
# ==========================================
@app.route('/api/dispositivos/<device_id>/status', methods=['PUT'])
def atualizar_status(device_id):
    try:
        dados = request.json
        
        # $set: atualizar apenas os campos enviados 
        # upsert=True: "Se o dispositivo 0001 não existir, crie ele agora"
        resultado = colecao_status.update_one(
            {"device_id": device_id}, 
            {"$set": dados}, 
            upsert=True
        )
        
        print(f"💡 Status atualizado para o dispositivo {device_id}")
        return jsonify({"mensagem": "Status atualizado com sucesso!"}), 200

    except Exception as e:
        return jsonify({"erro": str(e)}), 500

# ==========================================
# ROTA PARA O ESP32: Ler o Relé (GET)
# ==========================================
@app.route('/api/dispositivos/<device_id>/status', methods=['GET'])
def ler_status(device_id):
    try:
        # Busca no banco o documento exato do ESP32 solicitado
        status = colecao_status.find_one({"device_id": device_id})
        
        if status:
            status['_id'] = str(status['_id']) # Converte o ID para texto
            return jsonify(status), 200
        else:
            return jsonify({"erro": "Dispositivo não encontrado"}), 404

    except Exception as e:
        return jsonify({"erro": str(e)}), 500