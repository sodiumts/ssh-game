.PHONY: all configure build clean

all: configure

configure:
	mkdir -p build/
	cd build && cmake ..

build: configure
	cd build && cmake --build .

clean:
	rm -rf build
