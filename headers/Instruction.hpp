//
// Created by steve on 3/25/17.
//

#ifndef POLYHOOK_2_0_INSTRUCTION_HPP
#define POLYHOOK_2_0_INSTRUCTION_HPP

#include <cassert>
#include <string>
#include <vector>
#include <sstream>
#include <iostream> //ostream operator
#include <iomanip> //setw
#include <type_traits>

#include "headers/UID.hpp"
#include "headers/Enums.hpp"

#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#else
#include <signal.h>
#define DEBUG_BREAK raise(SIGTRAP);
#endif

namespace PLH {
class Instruction {
public:
	union Displacement {
		int64_t  Relative = 0;
		uint64_t Absolute;
	};

	Instruction(uint64_t address,
				const Displacement& displacement,
				const uint8_t displacementOffset,
				const bool isRelative,
				const std::vector<uint8_t>& bytes,
				const std::string& mnemonic,
				const std::string& opStr);

	Instruction(uint64_t address,
				const Displacement& displacement,
				const uint8_t displacementOffset,
				bool isRelative,
				uint8_t bytes[],
				size_t arrLen,
				const std::string& mnemonic,
				const std::string& opStr);
	
	/**Get the address of where the instruction points if it's a branching instruction
	* @Notes: Handles eip/rip & immediate branches correctly
	* **/
	uint64_t getDestination() const;

	void setDestination(const uint64_t dest);

	/**Get the address of the instruction in memory**/
	uint64_t getAddress() const;

	/**Set a new address of the instruction in memory
	@Notes: Doesn't move the instruction, marks it for move on writeEncoding and relocates if appropriate**/
	void setAddress(const uint64_t address);

	/**Get the displacement from current address**/
	Displacement getDisplacement() const;

	/**Set where in the instruction bytes the offset is encoded**/
	void setDisplacementOffset(const uint8_t offset);

	void setBranching(const bool status);

	/**Get the offset into the instruction bytes where displacement is encoded**/
	uint8_t getDisplacementOffset() const;

	/**Check if displacement is relative to eip/rip**/
	bool isDisplacementRelative() const;

	/**Check if the instruction is a type with valid displacement**/
	bool hasDisplacement() const;

	bool isBranching() const;

	const std::vector<uint8_t>& getBytes() const;

	/**Get short symbol name of instruction**/
	std::string getMnemonic() const;

	/**Get symbol name and parameters**/
	std::string getFullName() const;

	size_t getDispSize();

	size_t size() const;

	void setRelativeDisplacement(const int64_t displacement);

	void setAbsoluteDisplacement(const uint64_t displacement);

	long getUID() const;

	template<typename T>
	static T calculateRelativeDisplacement(uint64_t from, uint64_t to, uint8_t insSize) {
		if (to < from)
			return (T)(0 - (from - to) - insSize);
		return (T)(to - (from + insSize));
	}
private:
	uint64_t     m_address;       //Address the instruction is at
	Displacement m_displacement;  //Where an instruction points too (valid for jmp + call types)
	uint8_t      m_dispOffset;    //Offset into the byte array where displacement is encoded
	bool         m_isRelative;    //Does the displacement need to be added to the address to retrieve where it points too?
	bool         m_hasDisplacement; //Does this instruction have the displacement fields filled (only rip/eip relative types are filled)
	bool		 m_isBranching; //Does this instrunction jmp/call or otherwise change control flow

	std::vector<uint8_t> m_bytes; //All the raw bytes of this instruction
	std::string          m_mnemonic; //If you don't know what these two are then gtfo of this source code :)
	std::string          m_opStr;
	UID m_uid;
};

	typedef std::vector<Instruction> insts_t;
	bool operator==(const Instruction& lhs, const Instruction& rhs);
	uint16_t calcInstsSz(const insts_t& insts);
template<typename T>
std::string instsToStr(const T& container) {
	std::stringstream ss;
	ss << container;
	return ss.str();
}

	std::ostream& operator<<(std::ostream& os, const insts_t& v);
	std::ostream& operator<<(std::ostream& os, const Instruction& obj);



/**Write a 25 byte absolute jump. This is preferred since it doesn't require an indirect memory holder.
 * We first sub rsp by 128 bytes to avoid the red-zone stack space. This is specific to unix only afaik.**/
inline PLH::insts_t makex64PreferredJump(const uint64_t address, const uint64_t destination) {
	PLH::Instruction::Displacement zeroDisp = { 0 };
	uint64_t                       curInstAddress = address;

	std::vector<uint8_t> raxBytes = { 0x50 };
	Instruction pushRax(curInstAddress,
		zeroDisp,
		0,
		false,
		raxBytes,
		"push",
		"rax", Mode::x64);
	curInstAddress += pushRax.size();

	std::stringstream ss;
	ss << std::hex << destination;

	std::vector<uint8_t> movRaxBytes;
	movRaxBytes.resize(10);
	movRaxBytes[0] = 0x48;
	movRaxBytes[1] = 0xB8;
	memcpy(&movRaxBytes[2], &destination, 8);

	Instruction movRax(curInstAddress, zeroDisp, 0, false,
		movRaxBytes, "mov", "rax, " + ss.str(), Mode::x64);
	curInstAddress += movRax.size();

	std::vector<uint8_t> xchgBytes = { 0x48, 0x87, 0x04, 0x24 };
	Instruction xchgRspRax(curInstAddress, zeroDisp, 0, false,
		xchgBytes, "xchg", "QWORD PTR [rsp],rax", Mode::x64);
	curInstAddress += xchgRspRax.size();

	std::vector<uint8_t> retBytes = { 0xC3 };
	Instruction ret(curInstAddress, zeroDisp, 0, false,
		retBytes, "ret", "", Mode::x64);
	curInstAddress += ret.size();

	return { pushRax, movRax, xchgRspRax, ret };
}

/**Write an indirect style 6byte jump. Address is where the jmp instruction will be located, and
 * destHoldershould point to the memory location that *CONTAINS* the address to be jumped to.
 * Destination should be the value that is written into destHolder, and be the address of where
 * the jmp should land.**/
inline PLH::insts_t makex64MinimumJump(const uint64_t address, const uint64_t destination, const uint64_t destHolder) {
	PLH::Instruction::Displacement disp = { 0 };
	disp.Relative = PLH::Instruction::calculateRelativeDisplacement<int32_t>(address, destHolder, 6);

	std::vector<uint8_t> destBytes;
	destBytes.resize(8);
	memcpy(destBytes.data(), &destination, 8);
	Instruction specialDest(destHolder, disp, 0, false, destBytes, "dest holder", "", Mode::x64);

	std::vector<uint8_t> bytes;
	bytes.resize(6);
	bytes[0] = 0xFF;
	bytes[1] = 0x25;
	memcpy(&bytes[2], &disp.Relative, 4);

	std::stringstream ss;
	ss << std::hex << "[" << destHolder << "] ->" << destination;

	return { Instruction(address, disp, 2, true, bytes, "jmp", ss.str(), Mode::x64),  specialDest };
}

inline PLH::insts_t makex86Jmp(const uint64_t address, const uint64_t destination) {
	Instruction::Displacement disp;
	disp.Relative = Instruction::calculateRelativeDisplacement<int32_t>(address, destination, 5);

	std::vector<uint8_t> bytes(5);
	bytes[0] = 0xE9;
	memcpy(&bytes[1], &disp.Relative, 4);

	std::stringstream ss;
	ss << std::hex << destination;

	return { Instruction(address, disp, 1, true, bytes, "jmp", ss.str(), Mode::x86) };
}


inline PLH::insts_t makeAgnosticJmp(const uint64_t address, const uint64_t destination) {
	if constexpr (sizeof(char*) == 4)
		return makex86Jmp(address, destination);
	else
		return makex64PreferredJump(address, destination);
}

}
#endif //POLYHOOK_2_0_INSTRUCTION_HPP
