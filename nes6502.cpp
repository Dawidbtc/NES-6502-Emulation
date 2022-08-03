#include "nes6502.h"
#include "Bus.h"
#include <iostream>

nes6502::nes6502() {
	//Does Nothing
}

nes6502::~nes6502() {
	//Does Nothing
}

void nes6502::busconnector(Bus* n) {
	bus = n;
}

void nes6502::SetCarryFlag(uint8_t val) {
	statusReg |= val;
}

bool nes6502::GetCarryFlag() {
	return statusReg & 0x01;
}

void nes6502::SetZeroFlag(uint8_t val) {
	statusReg |= val << 1;
}

bool nes6502::GetZeroFlag() {
	return statusReg & 0x02;
}

void nes6502::SetInterruptFlag(uint8_t val) {
	statusReg |= val << 2;
}

bool nes6502::GetInterruptFlag() {
	return statusReg & 0x04;
}

void nes6502::SetDecimalFlag(uint8_t val) {
	statusReg |= val << 3;
}

bool nes6502::GetDecimalFlag() {
	return statusReg & 0x08;
}

void nes6502::SetBreakFlag(uint8_t val) {
	statusReg |= val << 4;
}

bool nes6502::GetBreakFlag() {
	return statusReg & 0x10;
}

void nes6502::SetUnusedFlag(uint8_t val) {
	statusReg |= val << 5;
}

bool nes6502::GetUnusedFlag() {
	return statusReg & 0x20;
}

void nes6502::SetOverflowFlag(uint8_t val) {
	statusReg |= val << 6;
}

bool nes6502::GetOverflowFlag() {
	return statusReg & 0x40;
}

void nes6502::SetNegativeFlag(uint8_t val) {
	statusReg |= val << 7;
}

bool nes6502::GetNegativeFlag() {
	return statusReg & 0x80;
}

void nes6502::Immediate() {
	address = PC;
	PC++;
}

void nes6502::Accum() {
	data = aReg;
}

void nes6502::Absolute() {
	uint8_t lowBytes = bus->read(PC);
	PC++;
	uint8_t highBytes = bus->read(PC);
	PC++;
	address = (highBytes << 8) + lowBytes; //Since little endian have to craft address with high byte and low byte.
}

bool nes6502::AbsoluteX() {
	uint8_t lowBytes = bus->read(PC);
	PC++;
	uint8_t highBytes = bus->read(PC);
	PC++;
	address = (highBytes << 8) + lowBytes;
	address += x;
	if (highBytes != address >> 8) { //If page change then may require extra clock cycle.
		return true;
	}
	return false;
}

bool nes6502::AbsoluteY() {
	uint8_t lowBytes = bus->read(PC);
	PC++;
	uint8_t highBytes = bus->read(PC);
	PC++;
	address = (highBytes << 8) + lowBytes;
	address += y;
	if (highBytes != address >> 8) { //If page change then may require extra clock cycle.
		return true;
	}
	return false;
}

void nes6502::ZeroPage() { //Used for lower byte instruction addressing memory in page 0
	address = bus->read(PC);
	PC++;
	address &= 0x00FF;
}

void nes6502::ZeroPageX() {
	address = bus->read(PC);
	address += x;
	PC++;
	address &= 0x00FF;
}

void nes6502::ZeroPageY() {
	address = bus->read(PC);
	address += y;
	PC++;
	address &= 0x00FF;
}

void nes6502::Indirect() { //6502 way of simulating pointers. Notice there is a page boundary bug in the 6502's Indirect Addressing. 
	uint8_t lowBytes = bus->read(PC);
	PC++;
	uint8_t highBytes = bus->read(PC);
	PC++;
	uint16_t ptrAddress = (highBytes << 8) + lowBytes;
	//An indirect jump will (xxFF) will fail because MSB will be fetched from address xx00 instead of page xx+1 00. It won't add +1 to the highBytes. 
	if (lowBytes == 0xFF) {
		uint8_t newLowBytes = bus->read(ptrAddress);
		uint8_t newHighBytes = bus->read(ptrAddress & 0xFF00);
		address = (newHighBytes << 8) + newLowBytes;
	}
	else {
		uint8_t newLowBytes = bus->read(ptrAddress);
		uint8_t newHighBytes = bus->read(ptrAddress + 1);
		address = (newHighBytes << 8) + newLowBytes;
	}
}

void nes6502::IndirectX() {
	uint8_t ptr = bus->read(PC);
	PC++;
	uint8_t newLowBytes = bus->read((uint16_t)(ptr + x) & 0x00FF); //type cast and ANDING with 0x00FF because 2 byte addresses. 
	uint8_t newHighBytes = bus->read((uint16_t)(ptr + x + 1) & 0x00FF);
	address = (newHighBytes << 8) + newLowBytes;
}

bool nes6502::IndirectY() {
	uint8_t ptr = bus->read(PC);
	PC++;
	uint8_t newLowBytes = bus->read((uint16_t)ptr);
	uint8_t newHighBytes = bus->read((uint16_t)(ptr + 1) & 0x00FF);
	address = (newHighBytes << 8) + newLowBytes;
	address += y;
	if (newHighBytes != address >> 8) {
		return true;
	}
	return false;
}

void nes6502::Relative() {
	relativeJump = bus->read(PC);
	PC++;
	if (relativeJump & 0x80) {
		relativeJump |= 0xFF00; //If jump is negative in twos complement form. Meaning the sign bit is 1 then we should change bit representation to twos complement for easier
	}                           //addition in opcode function.
}
void nes6502::BRK() {
	PC++; //PC increment because BRK is a two byte instruct with a padding byte. We can skip over padding byte to get PC register ready for next instruct.
	SetInterruptFlag(1);
	uint8_t lowBytePC = PC & 0x00FF;
	uint8_t highBytePC = PC >> 8;
	bus->write(0x0100 + rspPtr, highBytePC); //High Byte of PC register saved to stack 
	rspPtr--;
	bus->write(0x0100 + rspPtr, lowBytePC);  //Low Byte of PC register saved to stack 
	rspPtr--;
	bus->write(0x0100 + rspPtr, statusReg | 1 << 4); //save status register to stack with B flag on 
	SetBreakFlag(0);
	PC = (bus->read(0xFFFF) << 8) | bus->read(0xFFFE);
}

void nes6502::ORA() {
	data = bus->read(address);
	aReg = aReg | data;
	if (aReg == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}

	if (aReg & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::ASL(bool AccumMode) {
	if (!AccumMode) {
		data = bus->read(address);
	}
	bool checkCarry = data & 0b10000000;
	data <<= 1;
	SetCarryFlag((uint8_t)checkCarry);
	if (data & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
	if (data == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (AccumMode) {
		aReg = data;
	}
	else {
		bus->write(address, data);
	}
}

void nes6502::PHP() {
	bus->write(0x0100 + rspPtr, statusReg | 1 << 4 | 1 << 5);
	rspPtr--;
	SetBreakFlag(0);
	SetUnusedFlag(0);
}

void nes6502::BPL() {
	if (!GetNegativeFlag()) { //Branch only if positive. 
		address = PC + relativeJump;
		if ((address >> 8) != (PC >> 8)) { //page change requires extra cycle.
			remainingCycles += 2;
		}
		else {
			remainingCycles += 1; //A jump requires an extra cycle.
		}
		PC = address;
	}
}

void nes6502::JSR() { //Jump to func
	PC--;
	uint8_t lowBytePC = PC & 0x00FF;
	uint8_t highBytePC = PC >> 8;
	bus->write(0x0100 + rspPtr, highBytePC); //High Byte of PC register saved to stack 
	rspPtr--;
	bus->write(0x0100 + rspPtr, lowBytePC);  //Low Byte of PC register saved to stack 
	rspPtr--;
	PC = address;
}

void nes6502::AND() {
	data = bus->read(address);
	aReg = aReg & data;
	if (aReg == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (aReg & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::BIT() {
	data = bus->read(PC);
	if ((aReg & data) == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (data & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
	if (data & 0x40) {
		SetOverflowFlag(1);
	}
	else {
		SetOverflowFlag(0);
	}
}

void nes6502::ROL(bool AccumMode) {
	if (!AccumMode) {
		data = bus->read(address);
	}
	bool checkCarry = data & 0b10000000;
	data = (data << 1) | GetCarryFlag();
	SetCarryFlag((uint8_t)checkCarry);
	if (data & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
	if (data == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (AccumMode) {
		aReg = data;
	}
	else {
		bus->write(address, data);
	}
}

void nes6502::BMI() {
	if (GetNegativeFlag()) {
		address = PC + relativeJump;
		if ((address >> 8) != (PC >> 8)) { //page change requires extra cycle.
			remainingCycles += 2;
		}
		else {
			remainingCycles += 1; //A jump requires an extra cycle.
		}
		PC = address;
	}
}

void nes6502::RTI() { //Return from interrupt
	rspPtr++;
	statusReg = bus->read(0x0100 + rspPtr) & ~(1<<4) & ~(1<<5); 
	rspPtr++;
	uint8_t lowBytes = bus->read(0x0100 + rspPtr);
	rspPtr++;
	uint8_t highBytes = bus->read(0x0100 + rspPtr);
	PC = (highBytes << 8) + lowBytes;
}

void nes6502::EOR() {
	data = bus->read(PC);
	aReg ^= data;
	if (aReg == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (aReg & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::LSR(bool AccumMode) {
	if (!AccumMode) {
		data = bus->read(address);
	}
	bool checkCarry = data & 0b00000001;
	data >>= 1;
	SetCarryFlag((uint8_t)checkCarry);
	if (data & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
	if (data == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (AccumMode) {
		aReg = data;
	}
	else {
		bus->write(address, data);
	}
}

void nes6502::JMP() {
	PC = address;
}

void nes6502::BMI() {
	if (!GetOverflowFlag()) {
		address = PC + relativeJump;
		if ((address >> 8) != (PC >> 8)) { //page change requires extra cycle.
			remainingCycles += 2;
		}
		else {
			remainingCycles += 1; //A jump requires an extra cycle.
		}
		PC = address;
	}
}

void nes6502::RTS() {
	rspPtr++;
	uint8_t lowBytes = bus->read(0x0100 + rspPtr);
	rspPtr++;
	uint8_t highBytes = bus->read(0x0100 + rspPtr);
	PC = (highBytes << 8) + lowBytes;
	PC++;
}

void nes6502::ADC() {
	//Not Implemented Yet
}

void nes6502::SBC(bool Implied){
	if (Implied) {
		data = bus->read(PC);
	}
	//Not Implmented Yet 
}

void nes6502::ROR(bool AccumMode) {
	if (!AccumMode) {
		data = bus->read(PC);
	}
	bool carry = data & 0x01; 
	data = GetCarryFlag() << 7 | data >> 1; 
	SetCarryFlag(carry);
	SetZeroFlag(data == 0x00);
	SetNegativeFlag(data & 0x80); 
	if (AccumMode) {
		aReg = data;
	}
	else {
		bus->write(address, data);
	}
}

void nes6502::PLA() {
	rspPtr++;
	aReg = bus->read(0x0100 + rspPtr);
	SetNegativeFlag(aReg & 0x80);
	SetZeroFlag(aReg == 0x00);
}

void nes6502::BVS() {
	if (GetOverflowFlag()) { //Branch only if overflowflag on. 
		address = PC + relativeJump;
		if ((address >> 8) != (PC >> 8)) { //page change requires extra cycle.
			remainingCycles += 2;
		}
		else {
			remainingCycles += 1; //A jump requires an extra cycle.
		}
		PC = address;
	}
}

void nes6502::STY() {
	bus->write(address, y);
}

void nes6502::STA() {
	bus->write(address, aReg);
}

void nes6502::STX() {
	bus->write(address, x);
}

void nes6502::DEY() {
	y -= 1;
	if (y == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (y & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::TXA() {
	aReg = x;
	if (aReg == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (aReg & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::BCC() {
	if (!GetCarryFlag()){ //Branch only if carry off. 
		address = PC + relativeJump;
		if ((address >> 8) != (PC >> 8)) { //page change requires extra cycle.
			remainingCycles += 2;
		}
		else {
			remainingCycles += 1; //A jump requires an extra cycle.
		}
		PC = address;
	}
}

void nes6502::TYA() {
	aReg = y;
	if (aReg == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (aReg & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::LDY() {
	data = bus->read(PC);
	y = data;
	if (y == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (y & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::LDA() {
	data = bus->read(PC);
	aReg = data;
	if (aReg == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (aReg & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::LDX() {
	data = bus->read(PC);
	x = data;
	if (x == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (x & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::TAY() {
	y = aReg;
	if (y == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (y & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::TAX() {
	x = aReg;
	if (x == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (x & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::BCS() {
	if (GetCarryFlag()){ //Branch only if carry on. 
		address = PC + relativeJump;
		if ((address >> 8) != (PC >> 8)) { //page change requires extra cycle.
			remainingCycles += 2;
		}
		else {
			remainingCycles += 1; //A jump requires an extra cycle.
		}
		PC = address;
	}
}

void nes6502::TSX() {
	x = rspPtr;
	if (x == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (x & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::CPY() { //Compare Y Reg 
	data = bus->read(PC); 
	if ((y-data)==0x00){
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if ((y-data)&0x80){
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
	if (y >= data){
		SetCarryFlag(1);
	}
	else {
		SetCarryFlag(0);
	}
}

void nes6502::CMP() { //Compare aReg 
	data = bus->read(PC);
	if ((aReg-data)==0x00){
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if ((aReg - data) & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
	if (aReg >= data) {
		SetCarryFlag(1);
	}
	else {
		SetCarryFlag(0);
	}
}

void nes6502::DEC() {
	data = bus->read(PC);
	bus->write(address, data - 1);
	if ((data - 1) == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if ((data - 1) & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::INY() {
	y += 1;
	if (y == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}

	if (y & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::DEX() {
	x -= 1;
	if (x == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if (x & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::BNE(){
	if (!GetZeroFlag()){ //Branch Not Equal so on condition that zero flag off.
		address = PC + relativeJump;
		if ((address >> 8) != (PC >> 8)) { //page change requires extra cycle.
			remainingCycles += 2;
		}
		else {
			remainingCycles += 1; //A jump requires an extra cycle.
		}
		PC = address;
	}
}

void nes6502::CPX(){//Compare X Reg 
	data = bus->read(PC);
	if ((x - data) == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if ((x - data) & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
	if (x >= data) {
		SetCarryFlag(1);
	}
	else {
		SetCarryFlag(0);
	}
}

void nes6502::INC() {
	data = bus->read(PC);
	bus->write(address, data + 1);
	if ((data + 1) == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}
	if ((data + 1) & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::INX() {
	x += 1;
	if (x == 0x00) {
		SetZeroFlag(1);
	}
	else {
		SetZeroFlag(0);
	}

	if (x & 0x80) {
		SetNegativeFlag(1);
	}
	else {
		SetNegativeFlag(0);
	}
}

void nes6502::BEQ(){
	if (GetZeroFlag()) { //Branch Equal so on condition that zero flag on
		address = PC + relativeJump;
		if ((address >> 8) != (PC >> 8)) { //page change requires extra cycle.
			remainingCycles += 2;
		}
		else {
			remainingCycles += 1; //A jump requires an extra cycle.
		}
		PC = address;
	}
}
void nes6502::ExecOpCode(uint8_t opcode) {
	switch (opcode) {
	case 0x00: //BRK Immediate 7 cycles
		remainingCycles = 7;
		Immediate();
		BRK();
		break;

	case 0x01:  //ORA Indirect Zero Page X 6 cycles
		remainingCycles = 6;
		IndirectX();
		ORA();
		break;

	case 0x02:
	case 0x0B:
	case 0x12:
	case 0x22:
	case 0x2B:
	case 0x32:
	case 0x42:
	case 0x4B: //Illegal Opcode ILL Implied 2 cycles
	case 0x52:
	case 0x62:
	case 0x6B:
	case 0x72:
	case 0x8B:
	case 0x92:
	case 0xAB:
	case 0xB2:
	case 0xCB:
	case 0xD2:
	case 0xF2:
		remainingCycles = 2;
		break;

	case 0x03:
	case 0x13:
	case 0x23:
	case 0x33:
	case 0x43:
	case 0x53: //Illegal Opcode ILL Implied 8 cycles
	case 0x63:
	case 0x73:
	case 0xC3:
	case 0xD3:
	case 0xE3:
	case 0xF3:
		remainingCycles = 8;
		break;

	case 0x04:
	case 0x44: //Illegal Opcode NOP Implied 3 cycles (Just Burns Cycles)
	case 0x64:
		remainingCycles = 3;
		break;

	case 0x05: //ORA Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		ORA();
		break;

	case 0x06: //ASL Zero Page 5 cycles
		remainingCycles = 5;
		ZeroPage();
		ASL(0);
		break;

	case 0x07:
	case 0x27:
	case 0x47:
	case 0x67:
	case 0x9B: //Illegal Opcode ILL Implied 5 cycles
	case 0x9E: //$07 is used in Disney's Alladin (E) for SLO but I am not going to bother to implement it here. We shall just make it burn cycles.
	case 0x9F: //$B3 is also used in Super Cars (U) for LAX but I am not going to implement it here aswell. Als some homebrew games may use these opcodes differently. 
	case 0xB3: //Ref: https://www.nesdev.org/wiki/CPU_unofficial_opcodes
	case 0xC7: //Maybe i will make these opcodes work correctly lately on. For now these games are niche (and unpopular).
	case 0xE7:
		remainingCycles = 5;
		break;

	case 0x08: //PHP Implied 3 cycles	
		remainingCycles = 3;
		PHP();
		break;

	case 0x09: //ORA Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		ORA();
		break;

	case 0x0A: //ASL Accumulator 2 cycles 
		remainingCycles = 2;
		Accum();
		ASL(1);
		break;

	case 0x0C:
	case 0x14:
	case 0x1C:
	case 0x34:
	case 0x3C:
	case 0x54:
	case 0x5C: //Illegal Opcode NOP Implied 4 cycles (Burns cycles)
	case 0x74:
	case 0x7C:
	case 0xD4:
	case 0xDC:
	case 0xF4:
	case 0xFC:
		remainingCycles = 4;
		break;

	case 0x0D: //ORA Absolute 4 cycles 
		remainingCycles = 4;
		Absolute();
		ORA();
		break;

	case 0x0E: //ASL Absolute 6 cycles
		remainingCycles = 6;
		Absolute();
		ASL(0);
		break;

	case 0x0F:
	case 0x17:
	case 0x2F:
	case 0x37:
	case 0x4F:
	case 0x57:
	case 0x6F: //Illegal Opcode ILL Implied 6 cycles
	case 0x77: //$2F used for RLA in Attribute Zone (Homebrew) (Not Implemented Here)
	case 0x83:
	case 0x93:
	case 0xA3:
	case 0xCF:
	case 0xD7:
	case 0xEF:
	case 0xF7:
		remainingCycles = 6;
		break;

	case 0x10: //BPL Relative 2 cycles
		remainingCycles = 2;
		Relative();
		BPL();
		break;

	case 0x11: //ORA Indirect Zero Page Y 5 cycles
		remainingCycles = 5;
		extraCycle = IndirectY();
		ORA();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x15: //ORA Zero Page X 4 cycles
		remainingCycles = 4;
		ZeroPageX();
		ORA();
		break;

	case 0x16: //ASL Zero Page X 6 cycles
		remainingCycles = 6;
		ZeroPageX();
		ASL(0);
		break;

	case 0x18: //CLC Implied 2 cycles
		remainingCycles = 2;
		SetCarryFlag(0);
		break;

	case 0x19: //ORA Absolute Y 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteY();
		ORA();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x1A:
	case 0x3A:
	case 0x5A:
	case 0x7A: //Illegal Opcode NOP Implied 2 cycles
	case 0x80: //$80 used by Beauty and The Beast (E) 2 byte NOP. Won't be implemented here.
	case 0x82: //F-117A Stealth Fighter uses $89 (2 byte NOP). Won't be implemented here too. 
	case 0x89:
	case 0xC2:
	case 0xE2:
		remainingCycles = 2;
		break;

	case 0x1B:
	case 0x1F:
	case 0x3B:
	case 0x3F:
	case 0x5B:
	case 0x5F: //Illegal Opcode ILL Implied 7 cycles
	case 0x7B:
	case 0x7F:
	case 0xDB:
	case 0xDF:
	case 0xFB:
	case 0xFF:
		remainingCycles = 7;
		break;

	case 0x1D: //ORA Absolute X 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteX();
		ORA();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x1E: //ASL Absolute X 7 cycles
		remainingCycles = 7;
		AbsoluteX();
		ASL(0);
		break;

	case 0x20: //JSR Absolute 6 cycles
		remainingCycles = 6;
		Absolute();
		JSR();
		break;

	case 0x21: //AND Indirect Zero Page X 6 cycles
		remainingCycles = 6;
		IndirectX();
		AND();
		break;

	case 0x24: //BIT Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		BIT();
		break;

	case 0x25: //AND Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		AND();
		break;

	case 0x26: //ROL Zero Page 5 cycles
		remainingCycles = 5;
		ZeroPage();
		ROL(0);
		break;

	case 0x28: //PLP Implied 4 cycles
		remainingCycles = 4;
		rspPtr++;
		statusReg = bus->read(0x0100 + rspPtr);
		SetUnusedFlag(1);
		break;

	case 0x29: //AND Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		AND();
		break;

	case 0x2A: //ROL Accumulator 2 cycles
		remainingCycles = 2;
		Accum();
		ROL(1);
		break;

	case 0x2C: //BIT Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		BIT();
		break;

	case 0x2D: //AND Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		AND();
		break;

	case 0x2E: //ROL Absolute 6 cycles
		remainingCycles = 6;
		Absolute();
		ROL(0);
		break;

	case 0x30: //BMI Relative 2 cycles
		remainingCycles = 2;
		Relative();
		BMI();
		break;

	case 0x31: //AND Indirect Zero Page Y 5 cycles
		remainingCycles = 5;
		extraCycle = IndirectY();
		AND();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x35: //AND Zero Page X 4 cycles 
		remainingCycles = 4;
		ZeroPageX();
		AND();
		break;

	case 0x36: //ROL Zero Page X 6 cycles
		remainingCycles = 6;
		ZeroPageX();
		ROL(0);
		break;

	case 0x38: //SEC Implied 2 cycles
		remainingCycles = 2;
		SetCarryFlag(1);
		break;

	case 0x39: //AND Absolute Y 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteY();
		AND();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x3D: //AND Absolute X 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteX();
		AND();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x3E: //ROL Absolute X 7 cycles
		remainingCycles = 7;
		AbsoluteX();
		ROL(0);
		break;

	case 0x40: //RTI Implied 6 cycles
		remainingCycles = 6;
		RTI();
		break;

	case 0x41: //EOR Indirect Zero Page X 6 cycles
		remainingCycles = 6;
		IndirectX();
		EOR();
		break;

	case 0x45: //EOR Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		EOR();
		break;

	case 0x46: //LSR Zero Page 5 cycles
		remainingCycles = 5;
		ZeroPage();
		LSR(0);
		break;

	case 0x48: //PHA Implied 3 cycles
		remainingCycles = 3;
		bus->write(rspPtr, aReg);
		rspPtr--;
		break;

	case 0x49: //EOR Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		EOR();
		break;

	case 0x4A: //LSR Accumulator 2 cycles
		remainingCycles = 2;
		Accum();
		LSR(1);
		break;

	case 0x4C: //JMP Absolute 3 cycles
		remainingCycles = 3;
		Absolute();
		JMP();
		break;

	case 0x4D: //EOR Absolute 4 cycles 
		remainingCycles = 4;
		Absolute();
		EOR();
		break;

	case 0x4E: //LSR Absolute 6 cycles
		remainingCycles = 6;
		Absolute();
		LSR(0);
		break;

	case 0x50: //BVC Relative 2 cycles
		remainingCycles = 2;
		Relative();
		BVC();
		break;

	case 0x51: //EOR Indirect Zero Page Y 5 cycles
		remainingCycles = 5;
		extraCycle = IndirectY();
		EOR();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x55: //EOR Zero Page X 4 cycles
		remainingCycles = 4;
		ZeroPageX();
		EOR();
		break;

	case 0x56: //LSR Zero Page X 6 cycles
		remainingCycles = 6;
		ZeroPageX();
		LSR(0);
		break;

	case 0x58: //CLI Implied 2 cycles
		remainingCycles = 2;
		SetInterruptFlag(0);
		break;

	case 0x59: //EOR Absolute Y 4 cycles 
		remainingCycles = 4;
		extraCycle = AbsoluteY();
		EOR();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x5D: //EOR Absolute X 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteX();
		EOR();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x5E: //LSR Absolute X 7 cycles 
		remainingCycles = 7;
		AbsoluteX();
		LSR(0);
		break;

	case 0x60: //RTS Implied 6 cycles
		remainingCycles = 6;
		RTS();
		break;

	case 0x61: //ADC Indirect Zero Page X 6 cycles
		remainingCycles = 6;
		IndirectX();
		ADC();
		break;

	case 0x65: //ADC Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		ADC();
		break;

	case 0x66: //ROR Zero Page 5 cycles
		remainingCycles = 5;
		ZeroPage();
		ROR(0);
		break;

	case 0x68: //PLA Implied 4 cycles
		remainingCycles = 4;
		PLA();
		break;

	case 0x69: //ADC Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		ADC();
		break;

	case 0x6A: //ROR Accumulator 2 cycles
		remainingCycles = 2;
		Accum();
		ROR(1);
		break;

	case 0x6C: //JMP Indirect 5 cycles
		remainingCycles = 5;
		Indirect();
		JMP();
		break;

	case 0x6D: //ADC Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		ADC();
		break;

	case 0x6E: //ROR Absolute 6 cycles
		remainingCycles = 6;
		Absolute();
		ROR(0);
		break;

	case 0x70: //BVS Relative 2 cycles 
		remainingCycles = 2;
		Relative();
		BVS();
		break;

	case 0x71: //ADC Indirect Zero Page Y 5 cycles 
		remainingCycles = 5;
		extraCycle = IndirectY();
		ADC();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x75: //ADC Zero Page X 4 cycles 
		remainingCycles = 4;
		ZeroPageX();
		ADC();
		break;

	case 0x76: //ROR Zero Page X 6 cycles 
		remainingCycles = 6;
		ZeroPageX();
		ROR(0);
		break;

	case 0x78: //SEI Implied 2 cycles
		remainingCycles = 2;
		SetInterruptFlag(1);
		break;

	case 0x79: //ADC Absolute Y 4 cycles 
		remainingCycles = 4;
		extraCycle = AbsoluteY();
		ADC();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x7D: //ADC Absolute X 4 cycles 
		remainingCycles = 4;
		extraCycle = AbsoluteX();
		ADC();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0x7E: //ROR Absolute X 7 cycles 
		remainingCycles = 7;
		AbsoluteX();
		ROR(0);
		break;

	case 0x81: //STA Indirect Zero Page X 6 cycles 
		remainingCycles = 6;
		IndirectX();
		STA();
		break;

	case 0x84: //STY Zero Page 3 cycles 
		remainingCycles = 3;
		ZeroPage();
		STY();
		break;

	case 0x85: //STA Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		STA();
		break;

	case 0x86: //STX Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		STX();
		break;

	case 0x87: //Illegal Opcode ILL Implied 3 cycles
	case 0xA7: //Some homebrew games use A7 for LAX. 
		remainingCycles = 3;
		break;

	case 0x88: //DEY Implied 2 cycles 
		remainingCycles = 2;
		DEY();
		break;

	case 0x8A: //TXA Implied 2 cycles 
		remainingCycles = 2;
		TXA();
		break;

	case 0x8C: //STY Absolute 4 cycles 
		remainingCycles = 4;
		Absolute();
		STY();
		break;

	case 0x8D: //STA Absolute 4 cycles 
		remainingCycles = 4;
		Absolute();
		STA();
		break;

	case 0x8E: //STX Absolute 4 cycles 
		remainingCycles = 4;
		Absolute();
		STX();
		break;

	case 0x8F:
	case 0x97:
	case 0xAF: //Illegal Opcode ILL Implied 4 cycles
	case 0xB7: //Some homebrew games use $8F. Not Implemented Here.
	case 0xBB:
	case 0xBF:
		remainingCycles = 4;
		break;

	case 0x90: //BCC Relative 2 cycles
		remainingCycles = 2;
		Relative();
		BCC();
		break;

	case 0x91: //STA Indirect Zero Page Y 6 cycles 
		remainingCycles = 6;
		IndirectY();
		STA();
		break;

	case 0x94: //STY Zero Page X 4 cycles
		remainingCycles = 4;
		ZeroPageX();
		STY();
		break;

	case 0x95: //STA Zero Page X 4 cycles 
		remainingCycles = 4;
		ZeroPageX();
		STA();
		break;

	case 0x96: //STX Zero Page Y 4 cycles 
		remainingCycles = 4;
		ZeroPageY();
		STX();
		break;

	case 0x98: //TYA Implied 2 cycles 
		remainingCycles = 2;
		TYA();
		break;

	case 0x99: //STA Absolute Y 5 cycles
		remainingCycles = 5;
		AbsoluteY();
		STA();
		break;

	case 0x9A: //TXS Implied 2 cycles 
		remainingCycles = 2;
		rspPtr = x;
		break;

	case 0x9C: //Illegal Opcode NOP Implied 5 cycles (Burns Cycles)
		remainingCycles = 5;
		break;

	case 0x9D: //STA Absolute X 5 cycles
		remainingCycles = 5;
		AbsoluteX();
		STA();
		break;

	case 0xA0: //LDY Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		LDY();
		break;

	case 0xA1: //LDA Indirect Zero Page X 6 cycles
		remainingCycles = 6;
		IndirectX();
		LDA();
		break;

	case 0xA2: //LDX Immediate 2 cycles 
		remainingCycles = 2;
		Immediate();
		LDX();
		break;

	case 0xA4: //LDY Zero Page 3 cycles 
		remainingCycles = 3;
		ZeroPage();
		LDY();
		break;

	case 0xA5: //LDA Zero Page 3 cycles 
		remainingCycles = 3;
		ZeroPage();
		LDA();
		break;

	case 0xA6: //LDX Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		LDX();
		break;

	case 0xA8: //TAY Implied 2 cycles 
		remainingCycles = 2;
		TAY();
		break;

	case 0xA9: //LDA Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		LDA();
		break;

	case 0xAA: //TAX Implied 2 cycles
		remainingCycles = 2;
		TAX();
		break;

	case 0xAC: //LDY Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		LDY();
		break;

	case 0xAD: //LDA Absolute 4 cycles 
		remainingCycles = 4;
		Absolute();
		LDA();
		break;

	case 0xAE: //LDX Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		LDX();
		break;

	case 0xB0: //BCS Relative 2 cycles 
		remainingCycles = 2;
		Relative();
		BCS();
		break;

	case 0xB1: //LDA Indirect Zero Page Y 5 cycles
		remainingCycles = 5;
		extraCycle = IndirectY();
		LDA();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0xB4: //LDY Zero Page X 4 cycles 
		remainingCycles = 4;
		ZeroPageX();
		LDY();
		break;

	case 0xB5: //LDA Zero Page X 4 cycles
		remainingCycles = 4;
		ZeroPageX();
		LDA();
		break;

	case 0xB6: //LDX Zero Page Y 4 cycles 
		remainingCycles = 4;
		ZeroPageY();
		LDX();
		break;

	case 0xB8: //CLV Implied 2 cycles
		remainingCycles = 2;
		SetOverflowFlag(0);
		break;

	case 0xB9: //LDA Absolute Y 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteY();
		LDA();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0xBA: //TSX Implied 2 cycles
		remainingCycles = 2;
		TSX();
		break;

	case 0xBC: //LDY Absolute X 4 cycles 
		remainingCycles = 4;
		extraCycle = AbsoluteX();
		LDY();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0xBD: //LDA Absolute X 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteX();
		LDA();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0xBE: //LDX Absolute Y 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteY();
		LDX();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;
	case 0xC0: //CPY Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		CPY();
		break;
	case 0xC1: //CMP Indirect Zero Page X 6 cycles 
		remainingCycles = 6;
		IndirectX();
		CMP();
		break;

	case 0xC4: //CPY Zero Page 3 cycles 
		remainingCycles = 3;
		ZeroPage();
		CPY();
		break;

	case 0xC5: //CMP Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		CMP();
		break;

	case 0xC6: //DEC Zero Page 5 cycles
		remainingCycles = 5;
		ZeroPage();
		DEC();
		break; 

	case 0xC8: //INY Implied 2 cycles 
		remainingCycles = 2;
		INY();
		break;

	case 0xC9: //CMP Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		CMP();
		break; 

	case 0xCA: //DEX Implied 2 cycles 
		remainingCycles = 2;
		DEX();
		break; 

	case 0xCC: //CPY Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		CPY();
		break; 

	case 0xCD: //CMP Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		CMP();
		break; 

	case 0xCE: //DEC Absolute 6 cycles
		remainingCycles = 6;
		Absolute();
		DEC(); 
		break; 

	case 0xD0: //BNE Relative 2 cycles
		remainingCycles = 2;
		Relative();
		BNE();
		break; 

	case 0xD1: //CMP Indirect Zero Page Y 5 cycles
		remainingCycles = 5;
		extraCycle = IndirectY();
		CMP();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0xD5: //CMP Zero Page X 4 cycles
		remainingCycles = 4;
		ZeroPageX();
		CMP();
		break;

	case 0xD6: //DEC Zero Page X 6 cycles
		remainingCycles = 6;
		ZeroPageX();
		DEX();
		break; 

	case 0xD8: //CLD Implied 2 cycles
		remainingCycles = 2;
		SetDecimalFlag(0);
		break; 

	case 0xD9: //CMP Absolute Y 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteY();
		CMP();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break; 

	case 0xDA: 
	case 0xEA: //NOP Implied 2 cycles
	case 0xFA: 
		break; 

	case 0xDD: //CMP Absolute X 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteX();
		CMP();
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0xDE: //DEC Absolute X 7 cycles
		remainingCycles = 7;
		AbsoluteX();
		DEC();
		break; 

	case 0xE0: //CPX Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		CPX();
		break; 

	case 0xE1: //SBC Indirect Zero Page X 6 cycles
		remainingCycles = 6; 
		IndirectX();
		SBC(0);
		break;

	case 0xE4: //CPX Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		CPX();
		break; 

	case 0xE5: //SBC Zero Page 3 cycles
		remainingCycles = 3;
		ZeroPage();
		SBC(0);
		break; 

	case 0xE6: //INC Zero Page 5 cycles
		remainingCycles = 5;
		ZeroPage();
		INC();
		break; 

	case 0xE8: //INX Implied 2 cycles 
		remainingCycles = 2;
		INX();
		break; 

	case 0xE9: //SBC Immediate 2 cycles
		remainingCycles = 2;
		Immediate();
		SBC(0);
		break; 

	case 0xEB: //Illegal Opcode SBC Implied 2 cycles
		remainingCycles = 2;
		SBC(1);
		break; 

	case 0xEC: //CPX Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		CPX();
		break; 

	case 0xED: //SBC Absolute 4 cycles
		remainingCycles = 4;
		Absolute();
		SBC(0);
		break; 

	case 0xEE: //INC Absolute 6 cycles
		remainingCycles = 6;
		Absolute();
		INC();
		break;

	case 0xF0: //BEQ Relative 2 cycles
		remainingCycles = 2;
		Relative();
		BEQ();
		break; 

	case 0xF1: //SBC Indirect Zero Page Y 5 cycles
		remainingCycles = 5;
		extraCycle = IndirectY();
		SBC(0);
		if (extraCycle) {
			remainingCycles += 1;
		}
		break; 

	case 0xF5: //SBC Zero Page X 4 cycles
		remainingCycles = 4;
		ZeroPageX();
		SBC(0);
		break; 

	case 0xF6: //INC Zero Page X 6 cycles
		remainingCycles = 6;
		ZeroPageX();
		INC();
		break; 

	case 0xF8: //SED Implied 2 cycles
		remainingCycles = 2;
		SetDecimalFlag(1);
		break; 

	case 0xF9: //SBC Absolute Y 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteY();
		SBC(0);
		if (extraCycle) {
			remainingCycles += 1;
		}
		break;

	case 0xFD: //SBC Absolute X 4 cycles
		remainingCycles = 4;
		extraCycle = AbsoluteX();
		SBC(0);
		if (extraCycle){
			remainingCycles += 1;
		}
		break; 

	case 0xFE: //INC Absolute X 7 cycles
		remainingCycles = 7;
		AbsoluteX();
		INC();
		break; 
	}
}
void nes6502::clock() {
	if (remainingCycles == 0) {
		uint8_t opcode = bus->read(PC);
		PC++;
		SetUnusedFlag(1);
		ExecOpCode(opcode);
		SetUnusedFlag(1);
	}
	--remainingCycles;
}