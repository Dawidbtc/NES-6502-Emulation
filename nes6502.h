#pragma once
#include <iostream>
class Bus;

class nes6502
{
public:
	//constructors and destructors
	nes6502();
	~nes6502();
	//Bus connector function 
	void busconnector(Bus* n);
	//clock function
	void clock();
	//Registers

	uint8_t statusReg = 0x00; //status reg contains flags in format MSD -> LSD C(Carry) Z(Zero) I(Interrupt Disable) D(Decimal) B(Break) U(unused) O(overflow) N(negative)
	uint8_t aReg = 0x00; 
	uint8_t x = 0x00; 
	uint8_t y = 0x00;
	uint16_t rspPtr = 0x00; //Stack is really starting with 0x0100 however the stack wraps around on the 6502 so 1 byte offset used to simulate wrap around. 0x0100 - 0x01FF
	uint16_t PC = 0x0000;

	//Function to execute instruction 
	void ExecOpCode(uint8_t opcode);

	//Address being worked on (from addressing mode). 
	uint16_t address = 0x0000;
	//Relative Jump amount in relative jump instructions. 
	uint16_t relativeJump = 0x0000;
	//Keeps track if we need a extra cycle for instruction. 
	bool extraCycle = 0;
	//Data being worked on 
	uint8_t data = 0x00;
	//Flag Operation Functions 1 should be passed for Turn On 0 for Turn Off. 
	void SetCarryFlag(uint8_t val);
	bool GetCarryFlag();
	void SetZeroFlag(uint8_t val);
	bool GetZeroFlag();
	void SetInterruptFlag(uint8_t val);
	bool GetInterruptFlag();
	void SetDecimalFlag(uint8_t val);
	bool GetDecimalFlag();
	void SetBreakFlag(uint8_t val);
	bool GetBreakFlag();
	void SetUnusedFlag(uint8_t val);
	bool GetUnusedFlag();
	void SetOverflowFlag(uint8_t val);
	bool GetOverflowFlag();
	void SetNegativeFlag(uint8_t val);
	bool GetNegativeFlag();
	//Addressing Modes (No Implied because does nothing)
	void Immediate();
	void Accum();
	void Absolute();
	bool AbsoluteX(); //May require extra clock cycle so returns True or False
	bool AbsoluteY(); //May require extra clock cycle so returns True or False
	void ZeroPage();
	void ZeroPageX();
	void ZeroPageY();
	void Relative();
	void Indirect();
	void IndirectX();
	bool IndirectY();
	//Most Frequently Used Opcodes 
	void BRK(); void ORA(); void ASL(bool AccumMode);
	void PHP(); void BPL(); void JSR();
	void AND(); void BIT(); void ROL(bool AccumMode);
	void BMI(); void RTI(); void EOR(); 
	void LSR(bool AccumMode);  void JMP(); 
	void BVC(); void RTS(); void ADC();
	void SBC(bool Implied); void ROR(bool AccumMode);
	void PLA(); void BVS(); void STA();
	void STY(); void STX(); void DEY();
	void TXA(); void BCC(); void TYA();
	void LDY(); void LDA(); void LDX();  
	void TAY(); void TAX(); void BCS(); 
	void TSX(); void CPY(); void CMP(); 
	void DEC(); void INY(); void DEX();
	void BNE(); void CPX(); void INC();
	void INX(); void BEQ(); 

private:
	//Pointer to Bus Connection
	Bus* bus;
	uint8_t remainingCycles = 0; //remaining Cycles count for clock cycle simulation. 
};

