all: build/thrackle2planar build/crossgraph2tex.py build/pathtype_in_cycle

clean:
	rm -rf build
	rm -rf dist

build/thrackle2planar: thrackle2planar.c
	mkdir -p build
	cc -o $@ -O4 $^

build/crossgraph2tex.py: crossgraph2tex.py
	mkdir -p build
	cp $^ $@

build/pathtype_in_cycle: pathtype_in_cycle.c
	mkdir -p build
	cc -o $@ -O4 $^