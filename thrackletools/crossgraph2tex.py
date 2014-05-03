import sys

stdin = [l.strip() for l in sys.stdin if '>>' not in l]

vertex_count = int(sys.argv[1])

while '0' in stdin:
    zero = stdin.index('0')

    graph, stdin = stdin[:zero], stdin[zero+1:]

    positions = {}
    adjacency = {}
    vertices = []

    for l in graph:
        v, x, y , *adj = l.split()
        v = int(v)
        x, y = float(x), float(y)
        adj = [int(n) for n in adj]
        positions[v] = (x,y)
        adjacency[v] = adj
        vertices.append(v)

    print('\\documentclass[a4paper]{article}\n\\usepackage{tikz}\n\\begin{document}')
    print('\\begin{tikzpicture}[scale=2]')
    for v in vertices:
        if v <= vertex_count:
            print('\\node[draw,circle] ({0:}) at ({1:},{2:}) {{{0:}}};'.format(v, *positions[v]))
        else:
            print('\\coordinate ({}) at ({},{});'.format(v, *positions[v]))
    print()
    for v in vertices:
        for n in adjacency[v]:
            if v < n:
                print('\\draw[thick] ({}) to ({});'.format(v, n))
    print('\\end{tikzpicture}')
    print('\\clearpage')
print('\\end{document}')