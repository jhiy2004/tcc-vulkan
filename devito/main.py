import argparse
from pathlib import Path
from dataclasses import dataclass, field
import dotenv
import math
import os
import struct
import time

import matplotlib.pyplot as plt
import numpy as np
from devito import Eq, Function, Grid, Operator, TimeFunction, solve

from scipy.ndimage import gaussian_filter

from typing import Tuple

@dataclass
class InitialConditions:
    """
    Armazena as condições iniciais e cria as estruturas do Devito
    necessárias para a simulação.

    A classe valida as dimensões das matrizes de entrada, cria a malha
    computacional (`Grid`) e inicializa as funções do Devito que representam
    as variáveis da simulação.
    """

    nx: int
    ny: int
    lx: float
    ly: float
    g: float
    eta0: np.ndarray
    m0: np.ndarray
    n0: np.ndarray
    d0: np.ndarray
    h0: np.ndarray

    grid: Grid = field(init=False)
    eta: TimeFunction = field(init=False)
    m: TimeFunction = field(init=False)
    n: TimeFunction = field(init=False)
    h: Function = field(init=False)
    d: Function = field(init=False)

    def __post_init__(self):
        """
        Valida as condições iniciais e inicializa as estruturas da simulação.

        Este método executa automaticamente após a criação da instância e
        realiza as seguintes etapas:

        - verifica se todas as matrizes de entrada possuem dimensões
          compatíveis com `nx` e `ny`;
        - cria a malha computacional (`Grid`) do Devito;
        - instancia as funções temporais (`TimeFunction`) para elevação da
          superfície (`eta`) e fluxos (`m` e `n`);
        - instancia as funções espaciais (`Function`) para a batimetria (`h`)
          e profundidade total (`d`);
        - copia os valores das condições iniciais para as estruturas do
          Devito.

        Raises:
            ValueError: Caso alguma das matrizes de entrada não possua as
                dimensões especificadas por `nx` e `ny`.
        """

        for m in (self.eta0, self.m0, self.n0, self.d0, self.h0):
            (x, y) = m.shape
            if x != self.nx or y != self.ny:
                raise ValueError

        self.grid = Grid(shape=(self.ny, self.nx), extent=(self.ly, self.lx), dtype=np.float32)

        self.eta = TimeFunction(name='eta', grid=self.grid, space_order=2)
        self.m = TimeFunction(name='M', grid=self.grid, space_order=2)
        self.n = TimeFunction(name='N', grid=self.grid, space_order=2)
        self.h = Function(name='h', grid=self.grid)
        self.d = Function(name='D', grid=self.grid)

        self.eta.data[0] = self.eta0.copy()
        self.m.data[0] = self.m0.copy()
        self.n.data[0] = self.n0.copy()
        self.d.data[:] = self.eta0 + self.h0
        self.h.data[:] = self.h0.copy()


@dataclass
class SimulationParams:
    """
    Armazena os parâmetros que controlam a execução da simulação.

    A classe reúne as informações relacionadas ao tempo de simulação e à
    frequência de geração de quadros, facilitando a configuração e o
    compartilhamento desses parâmetros entre os componentes da aplicação.

    Attributes:
        t_max: Tempo total da simulação, em segundos.
        nt: Número total de passos de tempo da simulação.
        n_frames: Número de quadros (frames) a serem armazenados ou exportados
            durante a execução da simulação.
    """

    t_max: float
    nt: int
    n_frames: int

def generate_simulation_file(
    filepath: Path,
    initial_cond: InitialConditions,
    simulation_params: SimulationParams,
    buffer_size: int,
    num_type: str,
    save_images: bool = False,
    images_path: Path = None,
):
    """
    Executa a simulação numérica e grava os estados selecionados em um arquivo
    binário.

    A simulação é executada em blocos de tamanho `buffer_size` utilizando o
    operador de propagação. Ao final de cada bloco, os estados da superfície
    livre são copiados para um buffer temporário e apenas a quantidade de quadros
    especificados por `n_frames` são serializados no arquivo de saída.

    O arquivo binário contém um cabeçalho com informações sobre o tipo numérico,
    as dimensões do domínio e a quantidade de quadros armazenados, seguido pelos
    dados da batimetria e pelos snapshots da simulação.

    Args:
        filepath: Caminho do arquivo binário que será gerado.
        initial_cond: Condições iniciais e estruturas do Devito utilizadas na
            simulação.
        simulation_params: Parâmetros que definem o tempo total de simulação,
            número de passos temporais e quantidade de quadros a serem
            armazenados.
        buffer_size: Número máximo de passos de tempo processados por bloco.
        num_type: Tipo numérico utilizado na serialização. Os valores aceitos
            são `"int"`, `"float"` e `"double"`.
        save_images: Se `True`, salva uma imagem do primeiro estado de cada bloco
            para fins de inspeção ou depuração.
        images_path: Diretório onde as imagens serão salvas quando
            `save_images=True`.

    Raises:
        ValueError: Caso `num_type` não seja um dos tipos suportados.
    """

    grid = initial_cond.grid

    eta = initial_cond.eta
    m = initial_cond.m
    n = initial_cond.n
    h = initial_cond.h
    d = initial_cond.d

    eta_res = TimeFunction(
        name="eta_res",
        grid=grid,
        space_order=2,
        save=buffer_size
    )

    op = ForwardOperator(
        eta_res,
        eta,
        m,
        n,
        h,
        d,
        initial_cond.g,
        grid
    )

    t_max = simulation_params.t_max
    nt = simulation_params.nt
    n_frames = simulation_params.n_frames

    dt = t_max / nt
    delta_frames = nt // n_frames
    
    nblocks = math.ceil(nt / buffer_size)

    with open(filepath, "wb") as f:
        if num_type == 'double':
            np_type = np.float64
            f.write(b'\x02')
            fmt = "<d"
        elif num_type == 'float':
            np_type = np.float32
            f.write(b'\x01')
            fmt = "<f"
        elif num_type == 'int':
            np_type = np.int32
            f.write(b'\x00')
            fmt = "<i"
        else:
            raise ValueError
        f.write(struct.pack(fmt, grid.extent[0]))
        f.write(struct.pack(fmt, grid.extent[1]))

        f.write(struct.pack("<I", eta.data.shape[1]))
        f.write(struct.pack("<I", eta.data.shape[2]))
        f.write(struct.pack("<I", n_frames))

        h.data.astype(np_type).tofile(f)

        image_count = 0
        selected_frames = set([i for i in range(nt) if i % delta_frames == 0])

        assert len(selected_frames) == n_frames, "O número de quadros selecionados não corresponde a n_frames" 

        for b in range(nblocks):
            steps = min(buffer_size, nt - b * buffer_size)

            op.apply(time_M=steps - 1, dt=dt)

            data = eta_res.data[:steps].copy()

            if save_images:
                fig, ax = plt.subplots(figsize=(6, 5))
                current_frame = data[0]
                im = ax.imshow(current_frame.T, vmin=-1.0, vmax=1.0, cmap="seismic")
                plt.colorbar(im, ax=ax)
                plt.xlabel("x")
                plt.ylabel("y")
                plt.title(f"Snapshot - Bloco {b} | Time step {image_count}")
                plt.savefig(images_path / f"simulation_{b}.png")
                plt.close(fig)

            for i in range(steps):
                if image_count + i in selected_frames:
                    np.nan_to_num(data[i], nan=0.0).astype(np_type).tofile(f)
                    selected_frames.remove(image_count + i)

            image_count += steps


def ForwardOperator(etasave, eta, M, N, h, D, g, grid):
    """
    Constrói o operador do Devito responsável por avançar a simulação das
    equações de águas rasas em um passo de tempo.

    O operador gera os estênceis de diferenças finitas para a elevação da
    superfície livre (`eta`) e para os fluxos nas direções x (`M`) e y (`N`),
    atualiza a profundidade total (`D`) e armazena cada estado de `eta` na
    função temporal `etasave`.

    Args:
        etasave: Função temporal utilizada para armazenar os estados de
            `eta` produzidos durante a simulação.
        eta: Função temporal que representa a elevação da superfície livre.
        M: Função temporal que representa o fluxo na direção x.
        N: Função temporal que representa o fluxo na direção y.
        h: Função espacial que representa a batimetria.
        D: Função espacial que representa a profundidade total da coluna
            d'água.
        g: Aceleração da gravidade.
        grid: Malha computacional (`Grid`) utilizada pelo Devito.

    Returns:
        Operator: Operador do Devito que aplica as equações de evolução do
        sistema e registra os estados da superfície livre ao longo da
        simulação.
    """

    pde_eta = Eq(eta.dt + M.dxc + N.dyc)
    pde_M = Eq(M.dt + (M**2/D).dxc + (M*N/D).dyc + g*D*eta.forward.dxc)
    pde_N = Eq(N.dt + (M.forward*N/D).dxc + (N**2/D).dyc + g*D*eta.forward.dyc)

    stencil_eta = solve(pde_eta, eta.forward)
    stencil_M = solve(pde_M, M.forward)
    stencil_N = solve(pde_N, N.forward)

    update_eta = Eq(eta.forward, stencil_eta, subdomain=grid.interior)
    update_M = Eq(M.forward, stencil_M, subdomain=grid.interior)
    update_N = Eq(N.forward, stencil_N, subdomain=grid.interior)
    eq_D = Eq(D, eta.forward + h)

    return Operator([update_eta, update_M, update_N, eq_D] + [Eq(etasave, eta)])

def example_generator(selector: int, n_frames: int) -> Tuple[InitialConditions, SimulationParams]:
    """
    Gera um dos cenários de simulação pré-definidos.

    A função cria as condições iniciais e os parâmetros de simulação
    correspondentes ao cenário selecionado. Os exemplos são baseados nos
    casos de estudo apresentados na documentação do Devito para a resolução
    das equações de águas rasas.

    Cada cenário define a geometria do domínio, a batimetria, a elevação
    inicial da superfície livre, os fluxos iniciais e os parâmetros
    temporais da simulação.

    Args:
        selector: Identificador do cenário a ser gerado. Os valores válidos
            são:

            - 1: Tsunami em oceano de profundidade constante.
            - 2: Dois tsunamis em oceano de profundidade constante.
            - 3: Tsunami em oceano com variação unidimensional da profundidade.
            - 4: Tsunami em oceano com um monte submarino.
            - 5: Tsunami em oceano com topografia submarina aleatória.
            - 6: Problema bidimensional de ruptura circular de barragem.

        n_frames: Quantidade de quadros (snapshots) que deverão ser
            armazenados durante a simulação.

    Returns:
        tuple[InitialConditions, SimulationParams]:
            Uma tupla contendo:

            - um objeto `InitialConditions` com as condições iniciais do
              cenário;
            - um objeto `SimulationParams` com os parâmetros temporais da
              simulação.

    Raises:
        ValueError: Caso o identificador do cenário seja inválido.
    """

    GRAVITY = 9.81  # m/s²
    NX = 401
    NY = 401

    if selector == 1:
        # Example I: Tsunami in ocean with constant depth
        # Link: https://www.devitoproject.org/examples/cfd/08_shallow_water_equation.html#example-i-tsunami-in-ocean-with-constant-depth
        lx = 100.0
        ly = 100.0

        x = np.linspace(0.0, lx, num=NX)
        y = np.linspace(0.0, ly, num=NY)
        X, Y = np.meshgrid(x, y)

        h0 = 50. * np.ones_like(X)
        eta0 = 0.5 * np.exp(-((X-50)**2/10)-((Y-50)**2/10))
        m0 = 100. * eta0
        n0 = 0. * m0
        d0 = eta0 + 50.

        t_max = 3.0
        nt = 4500
    elif selector == 2:
        # Example II: Two Tsunamis in ocean with constant depth
        # Link: https://www.devitoproject.org/examples/cfd/08_shallow_water_equation.html#example-ii-two-tsunamis-in-ocean-with-constant-depth
        lx = 100.0
        ly = 100.0

        x = np.linspace(0.0, lx, num=NX)
        y = np.linspace(0.0, ly, num=NY)
        X, Y = np.meshgrid(x, y)

        h0 = 50 * np.ones_like(X)
        eta0 = 0.5 * np.exp(-((X-35)**2/10)-((Y-35)**2/10))
        eta0 -= 0.5 * np.exp(-((X-65)**2/10)-((Y-65)**2/10))
        m0 = 100. * eta0
        n0 = 0. * m0
        d0 = eta0 + h0
        
        t_max = 3.5
        nt = 8000
    elif selector == 3:
        # Example III: Tsunami in an ocean with 1D depth variation
        # Link: https://www.devitoproject.org/examples/cfd/08_shallow_water_equation.html#example-iii-tsunami-in-an-ocean-with-1d-tanh-depth-variation
        lx = 100.0
        ly = 100.0

        x = np.linspace(0.0, lx, num=NX)
        y = np.linspace(0.0, ly, num=NY)
        X, Y = np.meshgrid(x, y)
        
        h0 = 50 - 45 * np.tanh((X-70.)/8.)
        eta0 = 0.5 * np.exp(-((X-30)**2/10)-((Y-50)**2/20))
        m0 = 100. * eta0
        n0 = 0. * m0
        d0 = eta0 + h0

        t_max = 2.0
        nt = 4000
    elif selector == 4:
        # Example IV: Tsunami in an ocean with a seamount
        # Link: https://www.devitoproject.org/examples/cfd/08_shallow_water_equation.html#example-iv-tsunami-in-an-ocean-with-a-seamount
        lx = 100.0
        ly = 100.0

        x = np.linspace(0.0, lx, num=NX)
        y = np.linspace(0.0, ly, num=NY)
        X, Y = np.meshgrid(x, y)

        h0 = 50. * np.ones_like(X)
        h0 -= 45. * np.exp(-((X-50)**2/20)-((Y-50)**2/20))
        eta0 = 0.5 * np.exp(-((X-30)**2/5)-((Y-50)**2/5))
        m0 = 100. * eta0
        n0 = 0. * m0
        d0 = eta0 + h0

        t_max = 2.0
        nt = 8000
    elif selector == 5:
        # Example V: Tsunami in an ocean with random seafloor topography variations
        # Link: https://www.devitoproject.org/examples/cfd/08_shallow_water_equation.html#example-v-tsunami-in-an-ocean-with-random-seafloor-topography-variations
        lx = 100.0
        ly = 100.0

        x = np.linspace(0.0, lx, num=NX)
        y = np.linspace(0.0, ly, num=NY)
        X, Y = np.meshgrid(x, y)

        h0 = 30. * np.ones_like(X)
        pert = 5.

        np.random.seed(102034)
        r = 2.0 * (np.random.rand(NY, NX) - 0.5) * pert
        r = gaussian_filter(r, sigma=16)
        h0 = h0 * (1 + r)

        eta0 = 0.2 * np.exp(-((X-30)**2/5)-((Y-50)**2/5))

        m0 = 100. * eta0
        n0 = 0. * m0
        d0 = eta0 + h0

        t_max = 3.0
        nt = 4000
    elif selector == 6:
        # Example VI: 2D circular dam break problem
        # Link: https://www.devitoproject.org/examples/cfd/08_shallow_water_equation.html#example-vi-2d-circular-dam-break-problem
        lx = 100.0
        ly = 100.0

        x = np.linspace(0.0, lx, num=NX)
        y = np.linspace(0.0, ly, num=NY)
        X, Y = np.meshgrid(x, y)

        h0 = 30. * np.ones_like(X)
        eta0 = np.zeros_like(X)
        r0 = 5.
        mask = np.where(np.sqrt((X-50)**2 + (Y-50)**2) <= r0)
        eta0[mask] = 0.5
        eta0 = gaussian_filter(eta0, sigma=8)
        m0 = 1. * eta0
        n0 = 1. * m0
        d0 = eta0 + h0

        t_max = 3.0
        nt = 4000
    else:
        raise ValueError("Invalid example selector")


    initial_conditions = InitialConditions(
        nx=NX,
        ny=NY,
        lx=lx,
        ly=ly,
        g=GRAVITY,
        eta0=eta0,
        m0=m0,
        n0=n0,
        d0=d0,
        h0=h0,
    )
    simulation_params = SimulationParams(t_max=t_max, nt=nt, n_frames=n_frames)

    return (initial_conditions, simulation_params)


def main():
    """
    Executa o fluxo principal de geração de um arquivo de simulação.

    A função realiza as seguintes etapas:

    1. Processa os argumentos informados pela linha de comando.
    2. Carrega as variáveis de ambiente necessárias para a execução da
       aplicação.
    3. Obtém o diretório de saída onde os arquivos da simulação serão
       armazenados.
    4. Gera as condições iniciais e os parâmetros da simulação de acordo
       com o cenário selecionado.
    5. Executa a simulação numérica e serializa os resultados em um arquivo
       binário.
    6. Opcionalmente, salva imagens dos estados da simulação para fins de
       visualização e depuração.

    Argumentos de linha de comando:
        --example:
            Identificador do cenário de simulação a ser executado.

        --frames:
            Quantidade de quadros (snapshots) que serão armazenados no
            arquivo de saída.

        --buffer-size:
            Número máximo de passos temporais processados por bloco durante
            a simulação.

        --type:
            Tipo numérico utilizado na serialização dos dados. Os valores
            aceitos são ``int``, ``float`` e ``double``.

    Variáveis de ambiente:
        OUTPUT_PATH:
            Diretório onde serão gravados o arquivo de simulação e as
            imagens geradas.

    Raises:
        SystemExit: Encerra a aplicação com código de erro caso ocorra uma
            exceção durante a geração do arquivo de simulação.
    """

    parser = argparse.ArgumentParser()

    parser.add_argument("--example", type=int, default=1)
    parser.add_argument("--frames", type=int, default=1000)
    parser.add_argument("--buffer-size", type=int, default=500)
    parser.add_argument("--type", choices=["int", "float", "double"], default="float")

    args = parser.parse_args()

    buffer_size = args.buffer_size
    example_number = args.example
    n_frames = args.frames

    np.set_printoptions(threshold=np.inf)

    dotenv.load_dotenv()
    output_path = Path(os.getenv("OUTPUT_PATH"))

    init_cond, simulation_params = example_generator(example_number, n_frames)

    try:
        print(output_path / 'example.sim')
        generate_simulation_file(output_path / "example.sim", init_cond, simulation_params, buffer_size, 'float', save_images=True, images_path=output_path / 'images')
        
        print("Generate simulation file")
    except ValueError:
        print("Something went wrong. Please check the parameters and try again.")
        exit(1)


if __name__ == "__main__":
    main()
