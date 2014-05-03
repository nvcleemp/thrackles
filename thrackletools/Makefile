all: build/thrackle2planar

clean:
	rm -rf build
	rm -rf dist

build/thrackle2planar: thrackle2planar.c
	mkdir -p build
	cc -o $@ -O4 $^
