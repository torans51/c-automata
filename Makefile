run_compile: compile
	./main

compile:
	cc -Wall -Wextra -I./src/ -o main main.c
