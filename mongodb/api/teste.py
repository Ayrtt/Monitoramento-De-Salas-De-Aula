import requests
import time

URL_BASE = "http://127.0.0.1:5000"

def testar_envio_presenca():
    print("\n--- TESTANDO ENVIO DO PIR (POST) ---")
    dados = {
        "device_id": "0001",
        "evento": "start",
        "timestamp": int(time.time())
    }
    resposta = requests.post(f"{URL_BASE}/api/sensores/presenca", json=dados)
    print(f"Status: {resposta.status_code} | Resposta: {resposta.json()}")

def testar_leitura_status():
    print("\n--- TESTANDO LEITURA DO RELÉ (GET) ---")
    resposta = requests.get(f"{URL_BASE}/api/dispositivos/0001/status")
    
    if resposta.status_code == 200:
        dados = resposta.json()
        estado = "LIGADO" if dados.get("status_rele") else "DESLIGADO"
        print(f"💡 O ESP32 deve deixar o relé: {estado}")
    else:
        print(f"Status: {resposta.status_code} | Erro: {resposta.text}")

# Garanta que a sua API Flask esteja rodando em outro terminal antes de executar!
testar_envio_presenca()
testar_leitura_status()