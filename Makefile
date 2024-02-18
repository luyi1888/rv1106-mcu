CROSS_COMPILE ?= arm-rockchip830-linux-uclibcgnueabihf-
CC = $(CROSS_COMPILE)gcc


CFLAGS = -Wall -Wextra -pedantic -std=c99

all: mcutool

mcutool: mcutool.o

clean:
	-rm -f mcutool *.o

.PHONY: all clean
