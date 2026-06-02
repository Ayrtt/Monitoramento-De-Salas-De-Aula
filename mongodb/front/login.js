document.getElementById("loginForm").addEventListener("submit", async function(event) {
    event.preventDefault();

    const matricula = document.getElementById("matricula").value.trim();
    const password = document.getElementById("password").value.trim();
    const errorMessage = document.getElementById("errorMessage");
    const API_URL = "https://monitoramento-de-salas.onrender.com";

    errorMessage.style.color = "white";
    errorMessage.textContent = "Conectando ao servidor, aguarde...";

    if (!matricula || !password) {
        errorMessage.style.color = "red";
        errorMessage.textContent = "Preencha todos os campos.";
        return;
    }

    try {
        const response = await fetch(`${API_URL}/login`, {
            method: 'POST',
            headers: { 
                'Content-Type': 'application/json' 
            },
            body: JSON.stringify({ 
                matricula: matricula, 
                password: password 
            })
        });

        const data = await response.json();

        if (response.ok) {
            localStorage.setItem("matricula", data.matricula);
            localStorage.setItem("tipoUsuario", data.tipo);
            window.location.href = "monitoramento.html";
        } else {
            errorMessage.style.color = "red";
            errorMessage.textContent = data.message || "Erro ao realizar login.";
        }
    } catch (error) {
        console.error("Erro na requisição:", error);
        errorMessage.style.color = "red";
        errorMessage.textContent = "O servidor está ligando. Tente novamente em 30 segundos.";
    }
});