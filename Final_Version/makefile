CC = gcc
SRC_DIR = src
INCLUDE_DIR = include/
CFLAGS = -I ${INCLUDE_DIR}

CMP: BenchMark.cpp ${SRC_DIR}/*.cpp
	${CC} -o $@ $^ ${CFLAGS}

.PHONY: clean
clean:
	rm CMP