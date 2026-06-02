const form = document.getElementById("form-dispositivo");
const mensagem = document.getElementById("mensagem");

const API_URL = "http://192.168.12.18:5000";
//"https://monitoramento-de-salas.onrender.com"
//

form.addEventListener("submit", async (e) => {
    e.preventDefault();

    const id = document.getElementById("idSala").value.trim();
    const nome = document.getElementById("nomeSala").value.trim();

    if (!id || !nome) {
        mensagem.textContent = "Preencha todos os campos.";
        mensagem.style.color = "orange";
        return;
    }

    try {
        // Objeto enxuto: Apenas os campos da coleção device_status
        const novoDispositivo = {
            device_id: id,
            name: nome,
            relay_status: false,
            ac_status: false,
            temperature: 21,
            ac_command: 0,
            ac_command_response: 0,
            relay_command: false,
            relay_command_response: false
        };

        const response = await fetch(`${API_URL}/dispositivos`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(novoDispositivo)
        });

        const data = await response.json();

        if (response.ok) {
            mensagem.textContent = `Sala "${nome}" cadastrada com sucesso!`;
            mensagem.style.color = "lightgreen";
            form.reset();
            
            setTimeout(() => {
                window.location.href = "monitoramento.html";
            }, 2000);
        } else {
            mensagem.textContent = data.message || "Erro ao cadastrar sala.";
            mensagem.style.color = "red";
        }

    } catch (error) {
        console.error("Erro ao cadastrar sala:", error);
        mensagem.textContent = "Não foi possível conectar ao servidor.";
        mensagem.style.color = "red";
    }
});