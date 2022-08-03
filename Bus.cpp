#include "Bus.h"
void Bus::write(uint16_t address, uint8_t data) {
	if (address >= 0x0000 and address <= 0xFFFF) {
		ram[address] = data;
	}
}

uint8_t Bus::read(uint16_t address) {
	if (address >= 0x0000 and address <= 0xFFFF) {
		return ram[address];
	}
	else {
		return 0x00;
	}
}

Bus::Bus() {
	ram = (uint8_t*)calloc(65536, sizeof(uint8_t));
	//connect bus's cpu to bus 
	cpu.busconnector(this);
}
Bus::~Bus() {
	free(ram);
}

