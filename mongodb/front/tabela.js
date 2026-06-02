const corpoTabela = document.getElementById("tabela-dados");
const adminButtons = document.getElementById("adminButtons");
const thDeletar = document.getElementById("th-deletar");
const logoutBtn = document.getElementById("logout-btn");

const API_URL = "http://192.168.12.18:5000";
//"https://monitoramento-de-salas.onrender.com"
//

const tipoUsuario = localStorage.getItem("tipoUsuario");

if (tipoUsuario === "1") {
    if (adminButtons) adminButtons.style.display = "flex";
    if (thDeletar) thDeletar.style.display = "table-cell";
}

logoutBtn.onclick = () => {
    localStorage.clear();
    window.location.href = "index.html";
};

async function carregarDispositivos() {
    try {
        const response = await fetch(`${API_URL}/dispositivos`);
        if (!response.ok) throw new Error("Falha na rede");
        
        const dispositivos = await response.json();
        renderizarTabela(dispositivos);
    } catch (error) {
        console.error("Erro:", error);
        corpoTabela.innerHTML = `<tr><td colspan="6" style="text-align:center; color:red;">Erro ao carregar dados.</td></tr>`;
    }
}

function renderizarTabela(dispositivos) {
    if (!corpoTabela) return;
    corpoTabela.innerHTML = "";

    dispositivos.forEach((disp) => {
        const tr = document.createElement("tr");
        
        // --- STATUS BASEADO NO NOVO BANCO ---
        // Luz: Baseado em relay_status (true = ligada, false = desligada)
        const luzLigada = disp.relay_status;
        const corLuz = luzLigada ? "#00ff00" : "#ff4444";
        
        // Ar: Baseado em ac_command_response (0 = desligado, > 0 = temperatura)
        const acLigado = disp.ac_command_response > 0;
        const corAr = acLigado ? "#00ff00" : "#ff4444";

        tr.innerHTML = `
            <td>${disp.device_id}</td>
            <td>${disp.name || "Sala " + disp.device_id}</td>
            <td style="color: ${corLuz}; font-weight: bold;">${luzLigada ? "Ligada" : "Desligada"}</td>
            <td style="color: ${corAr}; font-weight: bold;">${acLigado ? "Ligado" : "Desligado"}</td>
            <td>${acLigado ? disp.ac_command_response + "°C" : "--"}</td>
            ${tipoUsuario === "1" ? `<td><button class="deletar-btn" data-id="${disp.device_id}">🗑️</button></td>` : ""}
        `;

        tr.onclick = (e) => {
            if (!e.target.classList.contains('deletar-btn')) {
                // Alterado para device_id
                localStorage.setItem("dispositivoIdParaRegistro", disp.device_id);
                window.location.href = "registro.html";
            }
        };

        corpoTabela.appendChild(tr);
    });

    configurarBotoesDeletar();
}

function configurarBotoesDeletar() {
    document.querySelectorAll(".deletar-btn").forEach(btn => {
        btn.onclick = async (e) => {
            e.stopPropagation();
            const id = btn.getAttribute("data-id"); // Puxando o device_id
            if (confirm(`Excluir a sala ${id}?`)) {
                try {
                    await fetch(`${API_URL}/dispositivos/${id}`, { method: 'DELETE' });
                    carregarDispositivos();
                } catch (err) {
                    console.error("Erro ao deletar:", err);
                }
            }
        };
    });
}

// Inicialização
carregarDispositivos();
setInterval(carregarDispositivos, 10000);
window.addEventListener('focus', carregarDispositivos);