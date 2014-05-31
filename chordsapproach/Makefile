all: build/chords_5-3

clean:
	rm -rf build
	rm -rf dist

build/chords_5-3: chords_5-3.c
	mkdir -p build
	cc -o $@ -O4 $^