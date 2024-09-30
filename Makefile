HOST_CC := cc
CC := arm-none-eabi-gcc

CFLAGS := -O0 -g -mcpu=cortex-m0 -mthumb -Ibase -nostdlib
LDFLAGS := -nostdlib

SRC := main.c
OBJ := $(SRC:.c=.o) boot2.o
EXE := firmware.elf

$(EXE): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

boot2.o: boot2_generic_03h.S tools/pad_checksum
	$(CC) $< -c -o $@ $(CFLAGS) -DCS=0
	$(CC) $< -c -o $@ $(CFLAGS) -DCS=$(shell tools/pad_checksum $@)

tools/pad_checksum: tools/pad_checksum.c
	$(HOST_CC) -o $@ $<

.PHONY: build all clean flash

build: $(EXE)

all: $(EXE)

clean:
	rm -f $(OBJ) $(EXE)

flash: $(EXE)
	openocd -f interface/stlink.cfg -f target/rp2040.cfg -c "program firmware.elf verify reset exit"
