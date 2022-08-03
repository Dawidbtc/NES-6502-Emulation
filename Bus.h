#pragma once
#include <cstdint>
#include "nes6502.h"
#include <stdlib.h>
class Bus
{
public:
	//constructors and deconstructors
	Bus();
	~Bus();
	//read and write function that is used by the cpu. 
	void write(uint16_t address, uint8_t data);
	uint8_t read(uint16_t address);
	//Devices on the Bus 
	nes6502 cpu;
	//ram 
	uint8_t* ram = nullptr;
};