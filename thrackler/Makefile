all: build/thrackler

clean:
	rm -rf build
	rm -rf dist

build/thrackler: thrackler.c shared/multicode_base.c shared/multicode_input.c
	mkdir -p build
	cc -o $@ -O4 $^
