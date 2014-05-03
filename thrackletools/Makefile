all: build/thrackle2planar build/crossgraph2tex.py

clean:
	rm -rf build
	rm -rf dist

build/thrackle2planar: thrackle2planar.c
	mkdir -p build
	cc -o $@ -O4 $^

build/crossgraph2tex.py: crossgraph2tex.py
	mkdir -p build
	cp $^ $@