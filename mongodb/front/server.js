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
const ATLAS_URI = ; 

//process.env.MONGODB_URI;
mongoose.connect(ATLAS_URI)
    .then(() => console.log("✅ Servidor conectado ao MongoDB Atlas"))
    .catch(err => console.error("❌ Erro de conexão:", err));

// --- MODELOS ---

// Mantido exatamente como o seu original, conforme solicitado
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
    ac_status: Boolean,
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
        // Mudança aqui: Retornamos o err.message verdadeiro para o front-end
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
            { new: true }
        );
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
            { new: true }
        );
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
            { new: true }
        );
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

// APAGUE ISTO:
// const PORT = process.env.PORT || 3000;
// app.listen(PORT, () => console.log(`🚀 API rodando na porta ${PORT}`));

// E SUBSTITUA POR ISTO:
const PORT = process.env.PORT || 5000; // Mantendo a 5000 para não ter que mexer no ESP32 agora

// Exportando o 'io' para podermos usar nas rotas depois, se necessário
app.set('io', io); 

server.listen(PORT, '0.0.0.0', () => {
    console.log(`🚀 Servidor Híbrido (Express + Socket.IO) rodando na porta ${PORT}`);
});
