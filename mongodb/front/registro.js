const id = localStorage.getItem("dispositivoIdParaRegistro");
const API_URL = "http://192.168.12.18:5000";

const popupPower = document.getElementById("popupPower");
const popupTemp = document.getElementById("popupTemperatura");
const inputTemp = document.getElementById("inputTemperatura");
let tipoPendente = "";

// Variáveis para armazenar os logs em memória
let logsArAtuais = [];
let logsLuzAtuais = [];

// --- FUNÇÕES DE CARREGAMENTO E LOGS ---

async function carregarDados() {
    try {
        // Busca os logs atualizados nas novas rotas
        const resAr = await fetch(`${API_URL}/dispositivos/${id}/logs/ar`);
        logsArAtuais = await resAr.json();
        
        const tabelaAr = document.getElementById("registros-ar");
        if (tabelaAr) {
            tabelaAr.innerHTML = logsArAtuais.length > 0 
                ? logsArAtuais.map((r, i) => `
                    <tr>
                        <td>${i + 1}</td>
                        <td>${r.timestamp || r.dataHora || "--"}</td>
                        <td style="color:#00ff00">Comando</td>
                        <td>${r.temperature || r.ac_command || "0"}°C</td>
                    </tr>`).join('')
                : `<tr><td colspan="4">Sem registros de ar.</td></tr>`;
        }

        const resLuz = await fetch(`${API_URL}/dispositivos/${id}/logs/luz`);
        logsLuzAtuais = await resLuz.json();

        const tabelaLuz = document.getElementById("registros-luz");
        if (tabelaLuz) {
            tabelaLuz.innerHTML = logsLuzAtuais.length > 0
                ? logsLuzAtuais.map((r, i) => `
                    <tr>
                        <td>${i + 1}</td>
                        <td>${r.timestamp || r.dataHora || "--"}</td>
                        <td style="color:${r.event === 'on' || r.estado ? '#00ff00' : '#ff4444'}">
                            ${r.event === 'on' || r.estado ? "Ligada" : "Desligada"}
                        </td>
                    </tr>`).join('')
                : `<tr><td colspan="3">Sem registros de luz.</td></tr>`;
        }
    } catch (err) { 
        console.error("Erro ao carregar dados principais:", err); 
    }
}

// --- FUNÇÃO DE DOWNLOAD DE CSV ---

function baixarCSV(tipo) {
    const dados = tipo === 'ar' ? logsArAtuais : logsLuzAtuais;
    const nomeArquivo = `historico_${tipo}_sala_${id}`;

    if (!dados || dados.length === 0) {
        return alert("Não há registros para baixar.");
    }

    const chavesRelevantes = Object.keys(dados[0]).filter(k => k !== '_id' && k !== '__v' && k !== 'device_id');
    
    let csvContent = "data:text/csv;charset=utf-8,";
    csvContent += chavesRelevantes.join(";") + "\n";

    dados.forEach(linha => {
        const valores = chavesRelevantes.map(chave => {
            let valor = linha[chave] !== undefined && linha[chave] !== null ? linha[chave] : "";
            return `"${String(valor).replace(/"/g, '""')}"`;
        });
        csvContent += valores.join(";") + "\n";
    });

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    link.setAttribute("download", `${nomeArquivo}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

// Configura os botões de download
document.getElementById("downloadArBtn")?.addEventListener("click", () => baixarCSV('ar'));
document.getElementById("downloadLuzBtn")?.addEventListener("click", () => baixarCSV('luz'));

// --- CONTROLES DOS POPUPS E COMANDOS ---

document.getElementById("powerArBtn").onclick = () => { tipoPendente = "ar"; popupPower.style.display = "flex"; };
document.getElementById("powerLuzBtn").onclick = () => { tipoPendente = "luz"; popupPower.style.display = "flex"; };

document.getElementById("confirmPower").onclick = async () => {
    try {
        const res = await fetch(`${API_URL}/dispositivos/${id}`);
        const disp = await res.json();
        
        let endpoint = "";
        let payload = {};

        if (tipoPendente === 'ar') {
            // Se maior que 0, desliga (0). Se estiver 0, liga em 23.
            const novoComando = disp.ac_command_response > 0 ? 0 : 23;
            endpoint = 'ar';
            payload = { comando: novoComando };
        } else if (tipoPendente === 'luz') {
            const novoEstado = !disp.relay_status;
            endpoint = 'luz';
            payload = { estado: novoEstado };
        }

        await fetch(`${API_URL}/dispositivos/${id}/${endpoint}`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        
        popupPower.style.display = "none";
        carregarDados(); // Recarrega os dados imediatamente
    } catch (err) {
        console.error("Erro ao enviar comando de power:", err);
    }
};

document.getElementById("temperaturaBtn").onclick = () => { popupTemp.style.display = "flex"; };

document.getElementById("confirmTemperatura").onclick = async () => {
    const temp = Number(inputTemp.value);
    
    if (temp < 16 || temp > 31) {
        return alert("Valor inválido. A temperatura deve estar entre 16°C e 31°C.");
    }

    try {
        await fetch(`${API_URL}/dispositivos/${id}/temperatura`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ temperatura: temp })
        });

        popupTemp.style.display = "none";
        carregarDados();
    } catch (err) {
        console.error("Erro ao enviar comando de temperatura:", err);
    }
};

document.getElementById("cancelPower").onclick = () => popupPower.style.display = "none";
document.getElementById("cancelTemperatura").onclick = () => popupTemp.style.display = "none";

// --- INICIALIZAÇÃO ---
carregarDados();
setInterval(carregarDados, 5000); // Atualiza os logs a cada 5 segundos
