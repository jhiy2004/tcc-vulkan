FROM python:3.12-slim

WORKDIR /app

# Dependências do Devito e compilação
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    git \
    && rm -rf /var/lib/apt/lists/*

COPY .env .
COPY requirements.txt .

RUN pip install --upgrade pip && \
    pip install --no-cache-dir -r requirements.txt

COPY main.py .

RUN mkdir -p /app/simulations

# Pasta persistente para resultados
VOLUME ["/app/simulations"]

CMD ["python", "main.py"]