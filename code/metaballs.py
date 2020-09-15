import numpy as np
from matplotlib import pyplot as plt
from matplotlib import cm as cm
import click
import re


re_time = re.compile(r'^\d+.?\d*e?-?\d*')
re_point = re.compile(
    r'([A-Z])\((-?\d+.?\d*e?-?\d*), (-?\d+.?\d*e?-?\d*)\)'
)


H_RANGE = tuple( np.array([-0.5, 0.5]) * 32 )
V_RANGE = tuple( np.array([-0.5, 0.5]) * 18 )
SPACE_FACTOR = 20
SPACE_BIAS = np.array([0.25, 0.25])

@click.group()
def main():
    pass


def render_frame(filename, width, height, cells, mutants):
    x = np.linspace(*H_RANGE, width)
    y = np.linspace(*V_RANGE, height)
    X, Y = np.meshgrid(x, y)
    Z = 0*X + 0*Y

    for cell in cells:
        z = np.exp(-((X - cell[0])**2 + (Y - cell[1])**2 + 0.6)**4)
        Z += z
    for cell in mutants:
        z = np.exp(-((X - cell[0])**2 + (Y - cell[1])**2 + 0.6)**4)
        Z += z
    Z[Z < 0.5] = 0
    Z[Z > 0] = 1

    fig, axs = plt.subplots()

    im = axs.imshow(Z, interpolation='bilinear', cmap=cm.binary,
                    origin='lower', extent=[*H_RANGE, *V_RANGE],
                    vmax=Z.max()*5, vmin=Z.min())

    S=200
    for cell in cells:
        axs.scatter(*cell, color='teal', s=S)
    for cell in mutants:
        axs.scatter(*cell, color='goldenrod', s=S)

    plt.axis('off')
    fig.set_size_inches(16, 9)
    plt.savefig(filename)
    plt.close()


def parse(line):
    cells = []
    mutants = []
    # time = re_time.match(line).group()
    for point in re_point.findall(line):
        typ, x, y = point
        if typ == 'A':
            cells.append(
                (np.array((float(x), float(y))) + SPACE_BIAS)*SPACE_FACTOR
            )
        else:
            mutants.append(
                (np.array((float(x), float(y))) + SPACE_BIAS)*SPACE_FACTOR
            )
    return cells, mutants


@main.command()
@click.option('-x', '--width', type=int)
@click.option('-y', '--height', type=int)
@click.argument('infile', type=click.Path())
@click.argument('outdir', type=str)
def render_frames(width, height, infile, outdir):
    with open(infile, 'r') as inf:
        for i, line in enumerate(inf):
            cells, mutants = parse(line)
            render_frame(
                outdir + '/frame{:09d}.png'.format(i),
                width, height,
                cells, mutants
            )


if __name__ == '__main__':
    main()


# y = np.linspace(*v, 1000)
# X, Y = np.meshgrid(x, y)

# Z = 0*X + 0*Y
# for cell in cells:
#     z = np.exp(-((X - cell[0])**2 + (Y - cell[1])**2 + 0.6)**4)
#     Z += z
# for cell in mutants:
#     z = np.exp(-((X - cell[0])**2 + (Y - cell[1])**2 + 0.6)**4)
#     Z += z
# Z[Z < 0.5] = 0
# Z[Z > 0] = 1

# fig, axs = plt.subplots()

# im = axs.imshow(Z, interpolation='bilinear', cmap=cm.binary,
#                 origin='lower', extent=[*h, *v],
#                 vmax=Z.max()*5, vmin=Z.min())

# S=200
# for cell in cells:
#     axs.scatter(*cell, color='teal', s=S)
# for cell in mutants:
#     axs.scatter(*cell, color='goldenrod', s=S)

# plt.axis('off')
# fig.set_size_inches(16, 9)
# plt.savefig('frontpage' + str(K) + '.pdf')  
# plt.show()

# K += 1
