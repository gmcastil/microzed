func-to-macro.o: func-to-macro.c func-to-macro.post-cpp
	@# Compiler flags were chosen to match the Xilinx SDK defaults
	gcc -Wall -O0 -g3 -c func-to-macro.c -o func-to-macro.o

func-to-macro.post-cpp: func-to-macro.c
	gcc -E -c func-to-macro.c > func-to-macro.post-cpp

clean:
	rm -f func-to-macro.post-cpp
	rm -f func-to-macro.o
	rm -f func-to-macro

