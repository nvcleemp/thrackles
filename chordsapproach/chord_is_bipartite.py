def is_bipartite(l):
    s = set(l)
    for c in s:
        i1 = l.index(c)
        i2 = l.index(c, i1+1)
        if (i2 - i1) % 2 == 0:
            print(l[i1:i2+1])

import sys

for line in sys.stdin:
    is_bipartite(line.strip().split()) 
    print("")
