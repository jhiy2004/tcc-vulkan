from devito import Eq, TimeFunction, sqrt, Function, Operator, Grid, solve
import time
import numpy as np
import dotenv
import os
import math
import struct
from pathlib import Path
import matplotlib.pyplot as plt


class InitialConditions:
    alpha: float

    eta: TimeFunction
    m: TimeFunction
    n: TimeFunction
    h: Function
    d: Function

    def __init__(self, nx, ny, lx, ly, g, alpha, eta0, m0, n0, d0, h0):
        for m in (eta0, m0, n0, d0, h0):
            (x, y) = m.shape
            if x != nx or y != ny:
                raise ValueError

        self.g = g
        self.alpha = alpha

        self.grid = Grid(shape=(ny, nx), extent=(ly, lx), dtype=np.float32)

        self.eta = TimeFunction(name='eta', grid=self.grid, space_order=2)
        self.m = TimeFunction(name='M', grid=self.grid, space_order=2)
        self.n = TimeFunction(name='N', grid=self.grid, space_order=2)
        self.h = Function(name='h', grid=self.grid)
        self.d = Function(name='D', grid=self.grid)

        self.eta.data[0] = eta0.copy()
        self.m.data[0] = m0.copy()
        self.n.data[0] = n0.copy()
        self.d.data[:] = eta0 + h0
        self.h.data[:] = h0.copy()


def generate_simulation_file(filepath: Path, initial_cond: InitialConditions, t_max: float, nt: int, buffer_size: int, num_type: str):
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
        initial_cond.alpha,
        grid
    )

    dt = t_max / nt
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
        f.write(struct.pack("<I", nt))

        h.data.astype(np_type).tofile(f)
        for b in range(nblocks):

            steps = min(buffer_size, nt - b * buffer_size)

            op.apply(time_M=steps-1, dt=dt)

            # salvar snapshots
            data = eta_res.data[:steps].copy()

            if b > nblocks + 1:
                # Cria a figura para o frame atual
                fig, ax = plt.subplots(figsize=(6, 5)) # Tamanho opcional para ficar mais bonito
                # Pegamos o frame atual (substitua eta_res.data pelo array correto que você está usando)
                current_frame = eta_res.data[steps-1]
                # Plota com a transposição (.T) e travando a escala de cores (vmin, vmax)
                # Dica: se a sua onda inicial tem altura 0.5, você pode usar vmin=-0.5, vmax=0.5
                im = ax.imshow(current_frame.T, vmin=-1.0, vmax=1.0, cmap="seismic")
                # Adiciona a barra de cores atrelada a esta imagem
                plt.colorbar(im, ax=ax)
                # Rótulos iguais ao do tutorial (embora em águas rasas 2D o certo seja Y em vez de Z)
                plt.xlabel('x')
                plt.ylabel('y') 
                plt.title(f"Snapshot - Bloco {b} | Time step {steps-1}")
                # Mostra o plot
                plt.show()
                # IMPORTANTE: Fecha a figura para liberar a memória (evita que o Python trave 
                # acumulando centenas de gráficos abertos em background)
                plt.close(fig)
            np.nan_to_num(data, nan=0.0).astype(np_type).tofile(f)

def ForwardOperator(etasave, eta, M, N, h, D, g, alpha, grid):
    """
    Operator that solves the equations expressed above.
    It computes and returns the discharge fluxes M, N and wave height eta from
    the 2D Shallow water equation using the FTCS finite difference method.

    Parameters
    ----------
    eta : TimeFunction
        The wave height field as a 2D array of floats.
    M : TimeFunction
        The discharge flux field in x-direction as a 2D array of floats.
    N : TimeFunction
        The discharge flux field in y-direction as a 2D array of floats.
    h : Function
        Bathymetry model as a 2D array of floats.
    D : Function
        Total thickness of the water column.
    g : float
        gravity acceleration.
    alpha : float
        Manning's roughness coefficient.
    etasave : TimeFunction
        Function that is sampled in a different interval than the normal propagation
        and is responsible for saving the snapshots required for the following
        animations.
    """

    # eps = np.finfo(grid.dtype).eps

    # Friction term expresses the loss of amplitude from the friction with the seafloor
    frictionTerm = g * alpha**2 * sqrt(M**2 + N**2) / D**(7./3.)

    # System of equations
    pde_eta = Eq(eta.dt + M.dxc + N.dyc)
    pde_M = Eq(M.dt + (M**2/D).dxc + (M*N/D).dyc + g*D*eta.forward.dxc + frictionTerm*M)
    pde_N = Eq(
        N.dt + (M.forward*N/D).dxc + (N**2/D).dyc + g*D*eta.forward.dyc
        + g * alpha**2 * sqrt(M.forward**2 + N**2) / D**(7./3.)*N
    )

    stencil_eta = solve(pde_eta, eta.forward)
    stencil_M = solve(pde_M, M.forward)
    stencil_N = solve(pde_N, N.forward)

    # Equations with the forward in time term isolated
    update_eta = Eq(eta.forward, stencil_eta, subdomain=grid.interior)
    update_M = Eq(M.forward, stencil_M, subdomain=grid.interior)
    update_N = Eq(N.forward, stencil_N, subdomain=grid.interior)
    eq_D = Eq(D, eta.forward + h)

    return Operator([update_eta, update_M, update_N, eq_D] + [Eq(etasave, eta)])

def convert_frame_pos_to_point(frame: np.ndarray, row: int, col: int):
    return (col, row, frame[row, col])


class Geometry:
    def __init__(self, points: np.ndarray, triangles: np.ndarray):
        self.points = points
        self.triangles = triangles


def create_base_geometry(rows, cols, extent):
    xmin, xmax, ymin, ymax = extent

    xs = np.linspace(xmin, xmax, cols, dtype=np.float32)
    ys = np.linspace(ymin, ymax, rows, dtype=np.float32)

    X, Y = np.meshgrid(xs, ys)

    points = np.empty((rows * cols, 3), dtype=np.float32)
    points[:, 0] = X.ravel()
    points[:, 1] = Y.ravel()
    points[:, 2] = 0.0

    i = np.arange(rows * cols, dtype=np.uint32).reshape(rows, cols)

    i0 = i[:-1, :-1].ravel()
    i1 = i[:-1, 1:].ravel()
    i2 = i[1:, :-1].ravel()
    i3 = i[1:, 1:].ravel()

    t1 = np.stack((i0, i1, i2), axis=1)
    t2 = np.stack((i1, i3, i2), axis=1)

    triangles = np.vstack((t1, t2))

    return Geometry(points, triangles)

def triangulate_frame(frame: np.ndarray, geometry: Geometry):
    geometry.points[:, 2] = frame.ravel()
    return geometry

def save_frame(geometry: Geometry, f):
    f.write(struct.pack("<II",
        len(geometry.points),
        len(geometry.triangles)
    ))

    points = np.ascontiguousarray(geometry.points, dtype=np.float32)
    triangles = np.ascontiguousarray(geometry.triangles, dtype=np.uint32)

    points.tofile(f)
    triangles.tofile(f)

def save_triangulated_sim(sim_filename: str, output_filename: str):
    with open(sim_filename, "rb") as f:
        sim_type = f.read(1)

        if sim_type == b'\x02':
            np_type = np.float64
            fmt = "<d"
        elif sim_type == b'\x01':
            np_type = np.float32
            fmt = "<f"
        elif sim_type == b'\x00':
            np_type = np.int32
            fmt = "<i"
        else:
            raise ValueError

        # header
        x_extent = struct.unpack(fmt, f.read(np.dtype(np_type).itemsize))[0]
        y_extent = struct.unpack(fmt, f.read(np.dtype(np_type).itemsize))[0]
        rows = struct.unpack("<I", f.read(4))[0]
        cols = struct.unpack("<I", f.read(4))[0]
        nt   = struct.unpack("<I", f.read(4))[0]

        data = np.fromfile(f, dtype=np_type, count=rows * cols)
        data = data.reshape((rows, cols))

        file = open(output_filename, "wb")
        # save triangulated_sim header
        file.write(struct.pack("<I", nt))

        file.close()

        file = open(output_filename, "ab")

        geometry = create_base_geometry(rows, cols, [0, x_extent, 0, y_extent])

        start = time.perf_counter()
        data.astype(np_type).tofile(f) 
        save_frame(geometry, file)
        end = time.perf_counter()

        print(f"Saved bathymetry in {end - start:.6f} s")

        for i in range(nt):
            data = np.fromfile(f, dtype=np_type, count=rows * cols)
            data = data.reshape((rows, cols))

            start = time.perf_counter()
            triangulate_frame(data, geometry)
            save_frame(geometry, file)
            end = time.perf_counter()

            print(f"Saved frame {i} in {end - start:.6f} s")

        file.close()

if __name__ == "__main__":
    np.set_printoptions(threshold=np.inf)

    dotenv.load_dotenv()

    output_path = Path(os.getenv("OUTPUT_PATH"))

    x = np.linspace(0.0, 100, num=401)
    y = np.linspace(0.0, 100, num=401)
    X, Y = np.meshgrid(x, y)

    #h0 = 50. * np.ones_like(X)
    #eta0 = 0.5 * np.exp(-((X-50)**2/10)-((Y-50)**2/10))
    #m0 = 100. * eta0
    #n0 = 0. * m0
    #d0 = eta0 + 50.

    h0 = 50 * np.ones_like(X)

    eta0 = 0.5 * np.exp(-((X-35)**2/10)-((Y-35)**2/10))  # first Tsunami source
    eta0 -= 0.5 * np.exp(-((X-65)**2/10)-((Y-65)**2/10))  # add second Tsunami source

    m0 = 100. * eta0
    n0 = 0. * m0
    d0 = eta0 + h0

    try:
        init_cond = InitialConditions(
            nx=401,
            ny=401,
            lx=100.0,
            ly=100.0,
            g=9.81,
            alpha=0.00,
            eta0=eta0,
            m0=m0,
            n0=n0,
            d0=d0,
            h0=h0,
        )
    except ValueError:
        print("Vish")
        exit(1)

    try:
        generate_simulation_file(output_path / "example.sim", init_cond, 3.5, 8000, 500, 'float')
        print("Generate simulation file")
    except ValueError:
        print("Shit")
        exit(2)
