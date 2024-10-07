.PHONY: submodules configure build flash debug

submodules:
	git submodule update --init --recursive

configure: submodules
	cmake -B build -S src .

build: configure
	cmake --build build

flash: build
	openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program build/esc_sproj3.elf verify reset exit"

debug: build
	openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000"
