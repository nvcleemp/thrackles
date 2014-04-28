all: build/thrackler build/thrackler_debug

clean:
	rm -rf build
	rm -rf dist

build/thrackler: thrackler.c shared/multicode_base.c shared/multicode_input.c
	mkdir -p build
	cc -o $@ -O4 $^

build/thrackler_debug: thrackler.c shared/multicode_base.c shared/multicode_input.c
	mkdir -p build
	cc -o $@ -g -DDEBUG $^
