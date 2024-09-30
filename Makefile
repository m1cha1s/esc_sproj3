CC := arm-none-eabi-gcc

CFLAGS := -O0 -g -mcpu=cortex-m0 -mthumb -Ibase -nostdlib
LDFLAGS := -T link.ld -nostdlib

SRC := main.c boot.c
OBJ := $(SRC:.c=.o)
EXE := firmware.elf

$(EXE): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

.PHONY: build all clean flash

build: $(EXE)

all: $(EXE)

clean:
	rm -f $(OBJ) $(EXE)

flash: $(EXE)
	openocd -f interface/stlink.cfg -f target/rp2040.cfg -c "program firmware.elf verify reset exit"
