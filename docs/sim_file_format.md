# Estrutura do arquivo .sim

O arquivo de simulação (.sim) é composto por um cabeçalho contendo
informações sobre o formato dos dados, dimensões do domínio e quantidade
de quadros, seguido pelos dados da batimetria e pelos valores da superfície
livre ao longo da simulação.

## Cabeçalho

### Byte 0
Define o tipo numérico utilizado para armazenar os dados:

- 0x00: int32
- 0x01: float32
- 0x02: float64

---

## Caso o tipo numérico seja float64 (0x02)

Como cada valor ocupa 8 bytes:

- Bytes 1 - 8:
    Extensão X do domínio (X Extent)

- Bytes 9 - 16:
    Extensão Y do domínio (Y Extent)

---

## Caso o tipo numérico seja int32 ou float32 (0x00 ou 0x01)

Como cada valor ocupa 4 bytes:

- Bytes 1 - 4:
    Extensão X do domínio (X Extent)

- Bytes 5 - 8:
    Extensão Y do domínio (Y Extent)

- Bytes 9 - 12:
    Número de linhas da malha (Rows)

- Bytes 13 - 16:
    Número de colunas da malha (Columns)

- Bytes 17 - 20:
    Número de quadros da simulação (N Frames)

---

## Dados da simulação

Após o cabeçalho:

- Bytes 21 até:
    21 + (rows * columns * tamanho_do_tipo) - 1

    Valores de profundidade/batimetria (Z da batimetria)

- Após a batimetria:

    N Frames * rows * columns * tamanho_do_tipo bytes

    Valores da elevação da superfície livre (Z da superfície)

---

## Observações

- O tamanho de cada valor depende do tipo definido no primeiro byte:
    - int32  → 4 bytes
    - float32 → 4 bytes
    - float64 → 8 bytes

- A ordem dos dados segue o layout da matriz em row-major (C-order).
  Isso significa que os elementos são armazenados linha por linha,
  da esquerda para a direita, e de cima para baixo.

  Exemplo:

  Matriz original:
  ```
    [
        [1, 2, 3],
        [4, 5, 6]
    ]
  ```

  Representação no arquivo:
  ```
    [1, 2, 3, 4, 5, 6]
  ```