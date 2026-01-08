.PHONY: all clean

all: main

main: main.c
	cc main.c -o main

clean: 
	rm main
