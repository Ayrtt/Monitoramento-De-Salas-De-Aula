import re

def limpar_codigo_ir(nome_variavel, dados_brutos):
    """
    Limpa os dados brutos do IRremote e formata para C++.
    """
    
    # 1. A Mágica do Regex:
    # Procura por um sinal (+ ou -), seguido opcionalmente de espaços, 
    # seguido de dígitos numéricos.
    # Isso ignora automaticamente textos como "rawIRTimings", "Duration", etc.
    padrao = r'[+-]\s*(\d+)'
    
    # Encontra todos os números que correspondem ao padrão
    matches = re.findall(padrao, dados_brutos)
    
    # Converte para inteiros
    tempos = [int(valor) for valor in matches]
    
    # 2. Remover o Gap Inicial
    # Se o primeiro número for muito grande (o silêncio inicial), removemos.
    if tempos and tempos[0] > 100000: # > 100ms
        tempos.pop(0)
        
    # 3. Formatar a Saída em C++
    tamanho = len(tempos)
    output = f"// Gerado automaticamente\n"
    output += f"const uint16_t {nome_variavel}[{tamanho}] = {{\n    "
    
    for i, tempo in enumerate(tempos):
        # Formata o número alinhado à direita com vírgula
        output += f"{tempo:>5}, "
        
        # Quebra de linha a cada 10 números para ficar legível
        if (i + 1) % 10 == 0:
            output += "\n    "
            
    # Remove a última vírgula e espaço extra, fecha a chave
    output = output.rstrip(", \n    ") 
    output += "\n};\n"
    
    return output

# ==========================================
# ÁREA DE CONFIGURAÇÃO (COLE SEUS DADOS AQUI)
# ==========================================

# Nome que você quer dar ao array no C++
NOME_DO_ARRAY = "CODIGO_TESTE_AR"

# Cole o texto EXATAMENTE como sai do Monitor Serial aqui dentro das aspas triplas:
DADOS_BRUTOS = """
rawIRTimings[228]: 
 -2407750
 +3050,-1650
 + 450,-1150 + 450,-1150 + 400,- 350 + 500,- 350
 + 450,- 350 + 500,-1100 + 450,- 350 + 500,- 350
 + 450,-1150 + 450,-1150 + 450,- 300 + 500,-1150
 + 450,- 300 + 500,- 350 + 450,-1150 + 450,-1150
 + 450,- 350 + 450,-1150 + 450,-1150 + 450,- 350
 + 450,- 350 + 450,-1150 + 450,- 350 + 500,- 300
 + 500,- 350 + 450,-1150 + 450,- 350 + 450,- 350
 + 500,- 350 + 450,- 350 + 500,- 300 + 500,- 350
 + 450,- 350 + 500,- 350 + 450,- 350 + 450,- 350
 + 500,- 350 + 450,- 350 + 500,- 300 + 500,- 350
 + 450,- 350 + 500,- 350 + 450,- 350 + 500,- 300
 + 500,- 350 + 450,- 200 + 650,-1100 + 450,- 350
 + 500,- 350 + 450,- 350 + 500,- 300 + 450,- 400
 + 450,- 200 + 600,-1150 + 450,- 200 + 600,- 400
 + 450,- 350 + 450,- 400 + 400,- 300 + 550,- 250
 + 550,- 250 + 600,- 250 + 550,- 350 + 450,- 400
 + 450,-1150 + 450,-1150 + 400,- 250 + 600,- 350
 + 450,- 350 + 450,- 400 + 450,- 350 + 450,-1150
 + 450,- 350 + 500,- 350 + 450,- 350 + 500,- 300
 + 500,- 350 + 450,- 350 + 500,- 350 + 450,- 350
 + 450,- 400 + 450,- 350 + 500,- 300 + 500,- 350
 + 500,- 300 + 500,- 300 + 500,- 350 + 500,- 300
 + 500,- 350 + 500,- 300 + 500,- 300 + 500,- 350
 + 500,- 300 + 500,- 350 + 450,- 350 + 500,- 300
 + 500,- 350 + 500,- 300 + 500,- 300 + 500,- 350
 + 500,- 300 + 500,- 300 + 500,- 300 + 550,- 300
 + 500,- 300 + 500,- 300 + 500,- 350 + 500,-1100
 + 450,- 350 + 500,- 350 + 450,- 350 + 500,- 300
 + 500
Duration=110700us
"""

# ==========================================
# EXECUÇÃO
# ==========================================
codigo_limpo = limpar_codigo_ir(NOME_DO_ARRAY, DADOS_BRUTOS)

print("-" * 30)
print("COPIE O CÓDIGO ABAIXO:")
print("-" * 30)
print(codigo_limpo)
print("-" * 30)