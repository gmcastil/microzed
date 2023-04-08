func-to-macro.S: func-to-macro.c
	gcc -Wall -O0 -S -c func-to-macro.c -o func-to-macro.S

func-to-macro.o: func-to-macro.c func-to-macro.post-cpp
	gcc -Wall -O0 -g3 -c func-to-macro.c -o func-to-macro.o

func-to-macro.post-cpp: func-to-macro.c
	gcc -E -c func-to-macro.c > func-to-macro.post-cpp

clean:
	rm -f func-to-macro.post-cpp
	rm -f func-to-macro.S
	rm -f func-to-macro.o
	rm -f func-to-macro
