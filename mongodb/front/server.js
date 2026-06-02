const dotenv = require('dotenv').config();
const express = require('express');
const mongoose = require('mongoose');
const cors = require('cors');
const http = require('http'); // <-- NOVO: Módulo nativo do Node
const { Server } = require('socket.io'); // <-- NOVO: Importação do Socket.IO

const app = express();

// <-- NOVO: Criação do servidor HTTP embrulhando o Express
const server = http.createServer(app); 

// <-- NOVO: Inicialização do Socket.IO liberando o CORS
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST", "PATCH", "DELETE"]
    }
});

app.use(express.json());
app.use(cors());

// --- CONEXÃO COM MONGODB ATLAS ---
const ATLAS_URI = process.env.ATLAS_URI;

//process.env.MONGODB_URI;
mongoose.connect(ATLAS_URI)
    .then(() => console.log("Servidor conectado ao MongoDB Atlas"))
    .catch(err => console.error("Erro de conexão:", err));

// --- MODELOS ---

const User = mongoose.model('User', new mongoose.Schema({
    matricula: { type: String, required: true, unique: true },
    email: String,
    senha: { type: String, required: true },
    tipo: Number
}));

// Coleção principal: Status em tempo real e Comandos
const DeviceStatus = mongoose.model('DeviceStatus', new mongoose.Schema({
    device_id: { type: String, required: true, unique: true },
    name: String,
    relay_status: Boolean,
    ac_status: Boolean, // <-- NOVO
    temperature: Number,
    ac_command: Number,
    ac_command_response: Number,
    relay_command: Boolean,
    relay_command_response: Boolean
}, { collection: 'device_status' }));

// Coleção de Logs (Apenas leitura no servidor Web, escritos pela ESP)
const PresenceLog = mongoose.model('PresenceLog', new mongoose.Schema({
    device_id: String,
    event: String,
    timestamp: String
}, { collection: 'presence_logs' }));

const TemperatureLog = mongoose.model('TemperatureLog', new mongoose.Schema({
    device_id: String,
    temperature: Number,
    timestamp: String
}, { collection: 'temperature_logs' }));

// --- ROTAS DE USUÁRIO ---

app.post('/login', async (req, res) => {
    const { matricula, password } = req.body;
    try {
        const user = await User.findOne({ matricula });
        if (!user) return res.status(404).json({ message: "Matrícula não encontrada." });

        if (user.senha === password) {
            res.json({ matricula: user.matricula, tipo: user.tipo });
        } else {
            res.status(401).json({ message: "Senha incorreta." });
        }
    } catch (err) {
        res.status(500).json({ message: "Erro no servidor." });
    }
});

app.post('/usuarios', async (req, res) => {
    try {
        const { matricula, email } = req.body;
        const existe = await User.findOne({ $or: [{ matricula }, { email }] });
        if (existe) return res.status(400).json({ message: "Matrícula ou E-mail já cadastrados." });

        const novoUsuario = new User(req.body);
        await novoUsuario.save();
        res.status(201).json({ message: "Usuário criado!" });
    } catch (err) {
        res.status(500).json({ message: "Erro ao salvar usuário." });
    }
});

app.delete('/usuarios/:matricula', async (req, res) => {
    try {
        const resultado = await User.findOneAndDelete({ matricula: req.params.matricula });
        if (!resultado) return res.status(404).json({ message: "Usuário não encontrado." });
        res.json({ message: "Usuário removido." });
    } catch (err) {
        res.status(500).json({ message: "Erro ao deletar." });
    }
});

// --- ROTAS DE DISPOSITIVOS ---

app.get('/dispositivos', async (req, res) => {
    try {
        const dispositivos = await DeviceStatus.find();
        res.json(dispositivos);
    } catch (err) {
        res.status(500).json({ message: "Erro ao buscar dispositivos." });
    }
});

app.get('/dispositivos/:id', async (req, res) => {
    try {
        const disp = await DeviceStatus.findOne({ device_id: req.params.id });
        if (!disp) return res.status(404).json({ message: "Sala não encontrada" });
        res.json(disp);
    } catch (err) {
        res.status(500).json({ message: "Erro ao buscar sala." });
    }
});

app.post('/dispositivos', async (req, res) => {
    try {
        const { device_id } = req.body;
        const existe = await DeviceStatus.findOne({ device_id });
        if (existe) return res.status(400).json({ message: "ID já existe." });

        const novo = new DeviceStatus(req.body);
        await novo.save();
        res.status(201).json(novo);
    } catch (err) {
        console.error("ERRO NO MONGO:", err); 
        res.status(500).json({ message: `Erro do Banco: ${err.message}` });
    }
});

app.delete('/dispositivos/:id', async (req, res) => {
    try {
        await DeviceStatus.findOneAndDelete({ device_id: req.params.id });
        
        // Opcional: Se quiser que ao deletar a sala os logs também sumam, descomente abaixo
        // await PresenceLog.deleteMany({ device_id: req.params.id });
        // await TemperatureLog.deleteMany({ device_id: req.params.id });

        res.json({ message: "Deletado com sucesso" });
    } catch (err) {
        res.status(500).send(err);
    }
});

// --- ROTAS DE CONTROLE (SINALIZAÇÃO PARA A ESP) ---

app.patch('/dispositivos/:id/ar', async (req, res) => {
    try {
        const { comando } = req.body; 
        const atualizado = await DeviceStatus.findOneAndUpdate(
            { device_id: req.params.id },
            { $set: { ac_command: comando } },
            //{ new: true }
            { returnDocument: 'after' } // <-- NOVO CORRIGIDO AQUI
        );

        // NOVO: Dispara o evento via WebSocket diretamente para o ESP32!
        io.emit('ac_command', { 
            device_id: req.params.id, 
            ac_command: comando 
        });

        res.json(atualizado);
    } catch (err) {
        res.status(500).json({ message: "Erro ao atualizar comando de Ar." });
    }
});

app.patch('/dispositivos/:id/luz', async (req, res) => {
    try {
        const { estado } = req.body;
        const atualizado = await DeviceStatus.findOneAndUpdate(
            { device_id: req.params.id },
            { $set: { relay_command: estado } },
            { returnDocument: 'after' } // <-- NOVO CORRIGIDO AQUI
        );

        // NOVO: Dispara o evento via WebSocket diretamente para o ESP32!
        io.emit('relay_command', { 
            device_id: req.params.id, 
            relay_command: estado 
        });

        res.json(atualizado);
    } catch (err) {
        res.status(500).json({ message: "Erro ao atualizar Luz." });
    }
});

app.patch('/dispositivos/:id/temperatura', async (req, res) => {
    try {
        const { temperatura } = req.body;
        const atualizado = await DeviceStatus.findOneAndUpdate(
            { device_id: req.params.id },
            { $set: { ac_command: temperatura } }, 
            { returnDocument: 'after' } // <-- NOVO CORRIGIDO AQUI
        );

        // NOVO: Mesma lógica de aviso para a mudança de temperatura do AC
        io.emit('ac_command', { 
            device_id: req.params.id, 
            ac_command: temperatura 
        });

        res.json({ message: "Comando de temperatura atualizado", data: atualizado });
    } catch (err) {
        res.status(500).json({ message: "Erro ao atualizar temperatura." });
    }
});

// --- ROTAS DE BUSCA DE LOGS ---

// Buscar logs de Ar Condicionado (Temperatura)
app.get('/dispositivos/:id/logs/ar', async (req, res) => {
    try {
        // Busca os logs do dispositivo, ordena do mais novo pro mais velho e limita aos últimos 30
        const logs = await TemperatureLog.find({ device_id: req.params.id })
                                         .sort({ _id: -1 })
                                         .limit(30); 
        res.json(logs);
    } catch (err) {
        res.status(500).json({ message: "Erro ao buscar logs de ar." });
    }
});

// Buscar logs de Luz (Presença/Relé)
app.get('/dispositivos/:id/logs/luz', async (req, res) => {
    try {
        const logs = await PresenceLog.find({ device_id: req.params.id })
                                      .sort({ _id: -1 })
                                      .limit(30);
        res.json(logs);
    } catch (err) {
        res.status(500).json({ message: "Erro ao buscar logs de luz." });
    }
});

// <-- NOVO
// =========================================================
// --- WEBSOCKETS: COMUNICAÇÃO EM TEMPO REAL COM O ESP32 ---
// =========================================================

io.on('connection', (socket) => {
    console.log(`[WebSocket] Placa/Cliente conectado! ID: ${socket.id}`);

    // 1. Escutando o sensor PIR (Presença) e o Relé
    socket.on('register_presence', async (payload) => {
        console.log(`[PIR] Presença detectada:`, payload);
        try {
            // Usa o molde do Mongoose para criar o documento
            const novoLog = new PresenceLog({
                device_id: payload.device_id,
                event: payload.event,
                timestamp: payload.timestamp
            });
            await novoLog.save(); // Salva no Atlas!
            console.log("Log de presença salvo no banco!");
            
            // Megafone: avisa o Front-end para atualizar a interface
            io.emit('atualizacao_presenca', payload); 
        } catch (erro) {
            console.error("Erro ao salvar log do PIR:", erro);
        }
    });

    // 2. Escutando o sensor DHT11 (Temperatura)
    socket.on('register_temperature', async (payload) => {
        console.log(`[DHT11] Nova temperatura:`, payload);
        try {
            // Usa o molde do Mongoose para o Ar Condicionado
            const novoLog = new TemperatureLog({
                device_id: payload.device_id,
                temperature: payload.temperature,
                timestamp: payload.timestamp
            });
            await novoLog.save(); // Salva no Atlas!
            console.log("Temperatura salva no banco!");
            
            // Megafone: avisa o Front-end para atualizar o gráfico
            io.emit('atualizacao_temperatura', payload);
        } catch (erro) {
            console.error("Erro ao salvar log de Temperatura:", erro);
        }
    });

    // 3. Status Voláteis (Não salvam no histórico, apenas atualizam a tela)
    socket.on('update_relay', async (payload) => {
        try {
            // Atualiza a "verdade física" do relé no banco de dados
            await DeviceStatus.findOneAndUpdate(
                { device_id: payload.device_id },
                { $set: { relay_status: payload.relay_status } }
            );
        } catch (err) {
            console.error("Erro ao atualizar status do relé no banco:", err);
        }
        
        // Repassa para a tela do navegador
        io.emit('status_rele_alterado', payload);
    });

    socket.on('update_ac_status', async (payload) => {
        try {
            // Atualiza o status do ar-condicionado no banco de dados
            await DeviceStatus.findOneAndUpdate(
                { device_id: payload.device_id },
                { $set: { ac_status: payload.ac_status } }
            );
        } catch (err) {
            console.error("Erro ao atualizar o estado do ar-condicionado no banco:", err);
        }
        
        // Repassa para a tela do navegador
        io.emit('estado_ar_alterado', payload);
    });

    socket.on('update_temperature', async (payload) => {
        try {
            // Atualiza a temperatura no banco de dados
            await DeviceStatus.findOneAndUpdate(
                { device_id: payload.device_id },
                { $set: { temperature: payload.temperature } }
            );
        } catch (err) {
            console.error("Erro ao atualizar a temperatura no banco:", err);
        }
        
        // Repassa para a tela do navegador
        io.emit('temperatura_alterada', payload);
    });

    // --- ESCUTANDO OS EVENTOS DE VERIFICAÇÃO/CONFIRMAÇÃO DO HARDWARE ---

    socket.on('relay_command_response', async (payload) => {
        console.log(`[VERIFICAÇÃO] Resposta do comando do Relé:`, payload);
        try {
            // Atualiza o estado de confirmação no documento da sala
            await DeviceStatus.findOneAndUpdate(
                { device_id: payload.device_id },
                { $set: { relay_command_response: payload.relay_command_response } }
            );
            console.log(`Confirmação do Relé salva em device_status: ${payload.relay_command_response}`);
            
            // Opcional: Avisa o front-end em tempo real que o hardware confirmou a ação
            io.emit('status_rele_confirmado', payload);
        } catch (err) {
            console.error("Erro ao salvar verificação do relé no banco:", err);
        }
    });

    socket.on('ac_command_response', async (payload) => {
        console.log(`[VERIFICAÇÃO] Resposta do comando do AC:`, payload);
        try {
            // Atualiza o estado de confirmação da temperatura no documento da sala
            await DeviceStatus.findOneAndUpdate(
                { device_id: payload.device_id },
                { $set: { ac_command_response: payload.ac_command_response } }
            );
            console.log(`Confirmação do AC salva em device_status: ${payload.ac_command_response}°C`);
            
            // Opcional: Avisa o front-end em tempo real que o hardware confirmou a ação
            io.emit('status_ac_confirmado', payload);
        } catch (err) {
            console.error("Erro ao salvar verificação do ar condicionado no banco:", err);
        }
    });


    socket.on('disconnect', () => {
        console.log(`[WebSocket] Cliente desconectado. ID: ${socket.id}`);
    });
});


const PORT = process.env.PORT || 5000; // Mantendo a 5000 para não ter que mexer no ESP32 agora

// Exportando o 'io' para podermos usar nas rotas depois, se necessário
app.set('io', io); 

server.listen(PORT, '0.0.0.0', () => {
    console.log(`Servidor Híbrido (Express + Socket.IO) rodando na porta ${PORT}`);
});
