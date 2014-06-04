all: build/chords_5-3 build/chords_5-5 build/crossing_parity_vectors

clean:
	rm -rf build
	rm -rf dist

build/chords_5-3: chords_5-3.c
	mkdir -p build
	cc -o $@ -O4 $^

build/chords_5-5: chords_5-5.c
	mkdir -p build
	cc -o $@ -O4 $^

build/crossing_parity_vectors: crossing_parity_vectors.c
	mkdir -p build
	cc -o $@ -O4 $^