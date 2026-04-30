# Vulkan Simulation Renderer

Este repositório contém uma aplicação dividida em duas partes principais:

- **Renderer em C++ com Vulkan**: responsável por carregar e renderizar simulações.
- **Gerador de simulação em Python (Devito)**: responsável por criar os dados de simulação que serão consumidos pelo renderer.

---

## Estrutura do projeto
```bash
.
├── devito
├── files
├── include
├── shaders
├── src
└── thirdparty
    ├── glad
    ├── glfw-3.4
    ├── glm
    ├── imgui-1.92.6
    └── stb_image
```
---

- **include/**  
  Contém os arquivos de header (`.h`) com as definições de classes, funções e estruturas usadas pelo renderer.

- **src/**  
  Contém os arquivos de implementação (`.cpp`) que definem a lógica das funções declaradas nos headers.

- **devito/**  
  Contém scripts em Python responsáveis por gerar as simulações numéricas utilizadas pelo sistema.

- **shaders/**  
  Contém os arquivos de shaders (Slang) que definem o comportamento gráfico utilizado pelo renderer Vulkan.

- **thirdparty/**  
  Contém bibliotecas e dependências externas utilizadas pelo projeto.

- **files/**  
  Diretório onde o usuário deve colocar os arquivos de simulação gerados, para que o renderer possa carregá-los.

## Visão geral

O pipeline funciona da seguinte forma:

1. O script em **Python (Devito)** executa uma simulação numérica.
2. Os resultados são exportados para um arquivo de dados binário (ex: `example.sim`).
3. A aplicação em **C++ com Vulkan** lê esse arquivo.
4. Os dados são interpretados e renderizados em tempo real.

---

## Como executar
Para buildar o projeto, basta ir na raiz do projeto e rodar:
```bash
cmake --build build
```

O comando acima irá gerar o arquivo executável do renderer vulkan.

Para gerar um arquivo de simulação basta entrar na pasta devito na raiz do projeto e montar o venv.
```bash
python -m venv venv
pip install -r requirements.txt
```

Depois, basta executar o arquivo main.py.
