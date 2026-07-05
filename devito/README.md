# Executar o gerador de simulações

```bash
docker compose up --build -d
docker compose exec simulator bash

python main.py --example 3 --frames 500 --buffer-size 200 --grid-x 401 --grid-y 401
python main.py --vtk --example 3 --frames 500 --buffer-size 200 --grid-x 401 --grid-y 401
```