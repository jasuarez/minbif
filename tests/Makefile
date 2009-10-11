libnobuffer.so: nobuffer.c
	gcc -o $@ -shared nobuffer.c $(CFLAGS) -fPIC

all: libnobuffer.so

clean:
	rm -f libnobuffer.so

.PHONY: all clean
