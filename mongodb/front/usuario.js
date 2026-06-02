const form = document.getElementById("form-usuario");
const mensagem = document.getElementById("mensagem");
const API_URL = "http://192.168.12.18:5000";
//"https://monitoramento-de-salas.onrender.com"
//

form.addEventListener("submit", async (e) => {
  e.preventDefault();

  const matricula = document.getElementById("matriculaUsuario").value.trim();
  const nome = document.getElementById("nomeUsuario").value.trim();
  const email = document.getElementById("emailUsuario").value.trim();
  const senha = document.getElementById("senhaUsuario").value.trim();
  const tipo = document.getElementById("tipoUsuario").value; // 0 ou 1

  if (!matricula || !nome || !email || !senha) {
    mensagem.textContent = "Preencha todos os campos.";
    mensagem.style.color = "orange";
    return;
  }

  try {
    const response = await fetch(`${API_URL}/usuarios`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        matricula,
        nome,
        email,
        senha,
        tipo: Number(tipo)
      })
    });

    const data = await response.json();
    if (response.ok) {
      mensagem.textContent = "Usuário cadastrado com sucesso!";
      mensagem.style.color = "lightgreen";
      form.reset();
    } else {
      mensagem.textContent = data.message || "Erro ao cadastrar usuário.";
      mensagem.style.color = "red";
    }
  } catch (err) {
    mensagem.textContent = "Erro ao conectar ao servidor.";
    mensagem.style.color = "red";
  }
});

const btnOpen = document.getElementById("btnOpenModal");
const btnCancel = document.getElementById("btnCancelModal");
const modal = document.getElementById("modalExclusao");
const btnConfirm = document.getElementById("btnConfirmDelete");
const inputMatricula = document.getElementById("matriculaDelete");

btnOpen.onclick = () => {
  modal.style.display = "flex";
  inputMatricula.focus();
};

btnCancel.onclick = () => {
  modal.style.display = "none";
  inputMatricula.value = "";
};

btnConfirm.onclick = async () => {
  const matricula = inputMatricula.value.trim();

  if (!matricula) {
    alert("Por favor, digite uma matrícula.");
    return;
  }

  try {
    const response = await fetch(`${API_URL}/usuarios/${matricula}`, {
      method: 'DELETE'
    });

    if (response.ok) {
      alert("Usuário removido com sucesso.");
      modal.style.display = "none";
      inputMatricula.value = "";
    } else {
      const data = await response.json();
      alert(data.message || "Erro ao remover usuário.");
    }
  } catch (err) {
    alert("Erro ao conectar ao servidor.");
  }
};