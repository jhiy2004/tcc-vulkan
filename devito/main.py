"""Utilities for generating and post-processing shallow-water simulations."""

from pathlib import Path
import dotenv
import math
import os
import struct
import time

import matplotlib.pyplot as plt
import numpy as np
from devito import Eq, Function, Grid, Operator, TimeFunction, solve, sqrt


class InitialConditions:
    """Container for the grid and initial state used by the simulation."""
    eta: TimeFunction
    m: TimeFunction
    n: TimeFunction
    h: Function
    d: Function

    def __init__(self, nx, ny, lx, ly, g, eta0, m0, n0, d0, h0):
        """Validate the input fields and build the Devito grid objects."""

        for m in (eta0, m0, n0, d0, h0):
            (x, y) = m.shape
            if x != nx or y != ny:
                raise ValueError

        self.g = g

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


def generate_simulation_file(
    filepath: Path,
    initial_cond: InitialConditions,
    t_max: float,
    nt: int,
    buffer_size: int,
    num_type: str,
    save_images: bool = False,
    images_path: Path = None,
):
    """Run the simulation and serialize the snapshots to a binary file."""

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

            op.apply(time_M=steps - 1, dt=dt)

            data = eta_res.data[:steps].copy()

            if save_images:
                fig, ax = plt.subplots(figsize=(6, 5))
                current_frame = eta_res.data[0]
                im = ax.imshow(current_frame.T, vmin=-1.0, vmax=1.0, cmap="seismic")
                plt.colorbar(im, ax=ax)
                plt.xlabel('x')
                plt.ylabel('y')
                plt.title(f"Snapshot - Bloco {b} | Time step {0}")
                plt.savefig(images_path / f'simulation_{b}.png')
                plt.close(fig)
            np.nan_to_num(data, nan=0.0).astype(np_type).tofile(f)


def ForwardOperator(etasave, eta, M, N, h, D, g, grid):
    """Build the Devito operator that advances the shallow-water system."""

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

def main():
    """Generate a sample simulation using the default parameters."""

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

    # h0 = 50 - 20*np.tanh((X-70)/8)
    # eta0 = (
    #     2*np.exp(-((X-35)**2/100)-((Y-35)**2/100))
    #     -
    #     2*np.exp(-((X-65)**2/100)-((Y-65)**2/100))
    # )

    # m0 = eta0 * np.sqrt(9.81*h0)
    # n0 = np.zeros_like(m0)

    # d0 = h0 + eta0


    # Tsunami in an ocean with a seamount
    h0 = 50. * np.ones_like(X)

    # Adding seamount to seafloor topography
    h0 -= 45. * np.exp(-((X-50)**2/20)-((Y-50)**2/20))

    # Define initial eta [m]
    eta0 = 0.5 * np.exp(-((X-30)**2/5)-((Y-50)**2/5))

    # Define initial M and N
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
        print(output_path / 'example.sim')
        generate_simulation_file(output_path / "example.sim", init_cond, 2, 8000, 500, 'float', save_images=True, images_path=output_path / 'images')
        
        print("Generate simulation file")
    except ValueError:
        print("Shit")
        exit(2)


if __name__ == "__main__":
    main()
