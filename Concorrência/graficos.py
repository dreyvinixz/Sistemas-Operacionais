import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import numpy as np

# Definição dos nomes das colunas esperadas
colunas = [
    'NumThreads', 'OperacoesPorThread', 'TotalOperacoes', 'TempoExecucao_ms',
    'OperacoesSucesso', 'OperacoesFalhas', 'TaxaSucesso', 'Throughput_ops_ms', 'Timestamp'
]

# Função para carregar o CSV, tentando com e sem cabeçalho
def carregar_csv_com_fallback(caminho_arquivo):
    try:
        df = pd.read_csv(caminho_arquivo)
        if 'NumThreads' not in df.columns:
            raise ValueError("Coluna 'NumThreads' não encontrada no CSV com cabeçalho.")
        print("CSV lido com cabeçalho.")
    except Exception as e:
        print(f"Erro ao ler CSV com cabeçalho: {e}")
        print("Tentando ler CSV sem cabeçalho e aplicando nomes manualmente...")
        df = pd.read_csv(caminho_arquivo, header=None, names=colunas)
        print("CSV lido sem cabeçalho, colunas atribuídas manualmente.")
    return df

# Carrega dados
df = carregar_csv_com_fallback('simulacao_logs.csv')

# Diagnóstico
print("Colunas no DataFrame:")
print(df.columns.tolist())
print("\nPrimeiras linhas:")
print(df.head())

# Converter 'NumThreads' para inteiro para eixo X numérico
df['NumThreads'] = df['NumThreads'].astype(int)

plt.figure(figsize=(10, 6))

# Gráfico de linha com x = número de threads, y = métricas
sns.lineplot(data=df, x='NumThreads', y='TempoExecucao_ms', marker='o', label='Tempo Execução (ms)')
sns.lineplot(data=df, x='NumThreads', y='OperacoesSucesso', marker='s', label='Operações Realizadas')
sns.lineplot(data=df, x='NumThreads', y='OperacoesFalhas', marker='^', label='Operações com Falha')

plt.xlabel('Número de Threads')
plt.ylabel('Valor')
plt.title('Métricas por Número de Threads')

# Controle dos ticks do eixo X (número de threads)
plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))

# Controle dos ticks do eixo Y (valor), ajuste automático ou fixo
# Exemplo fixo para ticks a cada 50:
ticks_y = np.arange(0, df[['TempoExecucao_ms','OperacoesSucesso','OperacoesFalhas']].max().max() + 50, 50)
plt.gca().set_yticks(ticks_y)

plt.legend()
plt.tight_layout()
plt.show()
