#include "stdafx.h"

#include "Common/Global/Globals.h"
#include "Common/Types/Registers/Register32_t.h"
#include "Common/Types/PhysicalMMU/PhysicalMMU_t.h"

#include "VM/Systems/IOP/IOPCoreInterpreter/IOPCoreInterpreter_s.h"

#include "Resources/IOP/IOPCore/IOPCore_t.h"
#include "Resources/IOP/IOPCore/Types/IOPCoreR3000_t.h"

void IOPCoreInterpreter_s::LB()
{
	// Rd = MEM[SB]. Address error or TLB error generated.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	auto& sourceReg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	const s16 imm = mInstruction.getIImmS();

	u32 virtualAddress = sourceReg->readWord(IOP) + imm;
	u32 physicalAddress;
	if (getPhysicalAddress(virtualAddress, READ, physicalAddress))
		return;

	auto value = static_cast<s8>(mPhysicalMMU->readByte(IOP, physicalAddress));
	destReg->writeWord(IOP, static_cast<s32>(value));
}

void IOPCoreInterpreter_s::LBU()
{
	// Rd = MEM[UB]. Address error or TLB error generated.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	auto& sourceReg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	const s16 imm = mInstruction.getIImmS();

	u32 virtualAddress = sourceReg->readWord(IOP) + imm;
	u32 physicalAddress;
	if (getPhysicalAddress(virtualAddress, READ, physicalAddress))
		return;

	auto value = mPhysicalMMU->readByte(IOP, physicalAddress);
	destReg->writeWord(IOP, static_cast<u32>(value));
}

void IOPCoreInterpreter_s::LH()
{
	// Rd = MEM[SH]. Address error or TLB error generated.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	auto& sourceReg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	const s16 imm = mInstruction.getIImmS();

	u32 virtualAddress = sourceReg->readWord(IOP) + imm;
	u32 physicalAddress;
	if (getPhysicalAddress(virtualAddress, READ, physicalAddress))
		return;

	auto value = static_cast<s16>(mPhysicalMMU->readHword(IOP, physicalAddress));
	destReg->writeWord(IOP, static_cast<s32>(value));
}

void IOPCoreInterpreter_s::LHU()
{
	// Rd = MEM[UH]. Address error or TLB error generated.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	auto& sourceReg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	const s16 imm = mInstruction.getIImmS();

	u32 virtualAddress = sourceReg->readWord(IOP) + imm;
	u32 physicalAddress;
	if (getPhysicalAddress(virtualAddress, READ, physicalAddress))
		return;

	auto value = mPhysicalMMU->readHword(IOP, physicalAddress);
	destReg->writeWord(IOP, static_cast<u32>(value));
}

void IOPCoreInterpreter_s::LUI()
{
	// Rd = Imm << 16. No exceptions generated.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	auto imm = static_cast<s32>(mInstruction.getIImmS());

	s32 result = imm << 16;

	destReg->writeWord(IOP, result);
}

void IOPCoreInterpreter_s::LW()
{
	// Rd = MEM[SW]. Address error or TLB error generated.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	auto& sourceReg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	const s16 imm = mInstruction.getIImmS();

	u32 virtualAddress = sourceReg->readWord(IOP) + imm;
	u32 physicalAddress;
	if (getPhysicalAddress(virtualAddress, READ, physicalAddress))
		return;

	auto value = mPhysicalMMU->readWord(IOP, physicalAddress);
	destReg->writeWord(IOP, static_cast<s32>(value));
}

void IOPCoreInterpreter_s::LWL()
{
	// TODO: check this, dont think its right. This should work for little-endian architectures (ie: x86), but not sure about big-endian. Luckily most machines are little-endian today, so this may never be a problem.
	// Rd = MEM[SW]. Address error or TLB error generated.
	// Unaligned memory read. Alignment occurs on an 4 byte boundary, but this instruction allows an unaligned read. LWL is to be used with LWR, to read in a full 32-bit value.
	// LWL reads in the most significant bytes (MSB's) depending on the virtual address offset, and stores them in the most significant part of the destination register.
	// Note that the other bytes already in the register are not changed. They are changed through LDR.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	auto& sourceReg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	const s16 imm = mInstruction.getIImmS();

	u32 unalignedAddress = sourceReg->readWord(IOP) + imm; // Get the unaligned virtual address.
	u32 baseAddress = unalignedAddress & ~static_cast<u32>(0x3); // Strip off the last 2 bits, making sure we are now aligned on a 4-byte boundary.
	u32 offset = unalignedAddress & static_cast<u32>(0x3); // Get the value of the last 2 bits, which will be from 0 -> 3 indicating the byte offset within the 4-byte alignment.

	u32 physicalAddress;
	if (getPhysicalAddress(baseAddress, READ, physicalAddress)) // Check for MMU error and do not continue if true.
		return;

	u32 alignedValue = mPhysicalMMU->readWord(IOP, physicalAddress); // Get the full aligned value, but we only want the full value minus the offset number of bytes.

	u8 MSBShift = ((4 - (offset + 1)) * 8); // A shift value used thoughout.
	u32 MSBMask = Constants::VALUE_U32_MAX >> MSBShift; // Mask for getting rid of the unwanted bytes from the aligned value.
	u32 MSBValue = (alignedValue & MSBMask) << MSBShift; // Calculate the MSB value part.

	u32 keepMask = ~(MSBMask << MSBShift); // The keep mask is used to select the bytes in the register which we do not want to change - this mask will be AND with those bytes, while stripping away the other bytes about to be overriden.
	destReg->writeWord(IOP, static_cast<s32>(static_cast<s32>((destReg->readWord(IOP) & keepMask) | MSBValue))); // Calculate the new desination register value and write to it.
}

void IOPCoreInterpreter_s::LWR()
{
	// TODO: check this, dont think its right. This should work for little-endian architectures (ie: x86), but not sure about big-endian. Luckily most machines are little-endian today, so this may never be a problem.
	// Rd = MEM[SW]. Address error or TLB error generated.
	// Unaligned memory read. Alignment occurs on an 4 byte boundary, but this instruction allows an unaligned read. LWR is to be used with LWL, to read in a full 32-bit value.
	// LWR reads in the least significant bytes (LSB's) depending on the virtual address offset, and stores them in the most significant part of the destination register.
	// Note that the other bytes already in the register are not changed. They are changed through LWL.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	auto& sourceReg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	const s16 imm = mInstruction.getIImmS();

	u32 unalignedAddress = sourceReg->readWord(IOP) + imm; // Get the unaligned virtual address.
	u32 baseAddress = unalignedAddress & ~static_cast<u32>(0x3); // Strip off the last 2 bits, making sure we are now aligned on a 4-byte boundary.
	u32 offset = unalignedAddress & static_cast<u32>(0x3); // Get the value of the last 2 bits, which will be from 0 -> 3 indicating the byte offset within the 4-byte alignment.

	u32 physicalAddress;
	if (getPhysicalAddress(baseAddress, READ, physicalAddress)) // Check for MMU error and do not continue if true.
		return;

	u32 alignedValue = mPhysicalMMU->readWord(IOP, physicalAddress); // Get the full aligned value, but we only want the full value minus the offset number of bytes.

	u8 LSBShift = (offset * 8); // A shift value used thoughout.
	u32 LSBMask = Constants::VALUE_U32_MAX << LSBShift; // Mask for getting rid of the unwanted bytes from the aligned value.
	u32 LSBValue = (alignedValue & LSBMask) >> LSBShift; // Calculate the LSB value part.

	u32 keepMask = ~(LSBMask >> LSBShift); // The keep mask is used to select the bytes in the register which we do not want to change - this mask will be AND with those bytes, while stripping away the other bytes about to be overriden.
	destReg->writeWord(IOP, static_cast<s32>(static_cast<s32>((destReg->readWord(IOP) & keepMask) | LSBValue))); // Calculate the new desination register value and write to it.
}

void IOPCoreInterpreter_s::SB()
{
	// MEM[UB] = Rd. Address error or TLB error generated.
	auto& source1Reg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	auto& source2Reg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	const s16 imm = mInstruction.getIImmS();

	u32 virtualAddress = source1Reg->readWord(IOP) + imm;
	u32 physicalAddress;
	if (getPhysicalAddress(virtualAddress, WRITE, physicalAddress))
		return;

	mPhysicalMMU->writeByte(IOP, physicalAddress, source2Reg->readByte(IOP, 0));
}

void IOPCoreInterpreter_s::SH()
{
	// MEM[UH] = Rd. Address error or TLB error generated.
	auto& source1Reg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	auto& source2Reg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	const s16 imm = mInstruction.getIImmS();

	u32 virtualAddress = source1Reg->readWord(IOP) + imm;
	u32 physicalAddress;
	if (getPhysicalAddress(virtualAddress, WRITE, physicalAddress))
		return;

	mPhysicalMMU->writeHword(IOP, physicalAddress, source2Reg->readHword(IOP, 0));
}

void IOPCoreInterpreter_s::SW()
{
	// MEM[UW] = Rd. Address error or TLB error generated.
	auto& source1Reg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	auto& source2Reg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	const s16 imm = mInstruction.getIImmS();

	u32 virtualAddress = source1Reg->readWord(IOP) + imm;
	u32 physicalAddress;
	if (getPhysicalAddress(virtualAddress, WRITE, physicalAddress))
		return;

	mPhysicalMMU->writeWord(IOP, physicalAddress, source2Reg->readWord(IOP));
}

void IOPCoreInterpreter_s::SWL()
{
	// TODO: check this, dont think its right. This should work for little-endian architectures (ie: x86), but not sure about big-endian. Luckily most machines are little-endian today, so this may never be a problem.
	// MEM[UW] = Rd. Address error or TLB error generated.
	// Unaligned memory write. Alignment occurs on an 4 byte boundary, but this instruction allows an unaligned write. SWL is to be used with SWR, to write a full 32-bit value.
	// SWL writes the most significant bytes (MSB's) depending on the virtual address offset, and stores them in the most significant part of the destination memory.
	// Note that the other bytes already in the register are not changed. They are changed through SWR.
	auto& source1Reg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	auto& source2Reg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	const s16 imm = mInstruction.getIImmS();

	u32 unalignedAddress = source1Reg->readWord(IOP) + imm; // Get the unaligned virtual address.
	u32 baseAddress = unalignedAddress & ~static_cast<u32>(0x3); // Strip off the last 2 bits, making sure we are now aligned on a 4-byte boundary.
	u32 offset = unalignedAddress & static_cast<u32>(0x3); // Get the value of the last 2 bits, which will be from 0 -> 3 indicating the byte offset within the 4-byte alignment.

	u32 regValue = source2Reg->readWord(IOP); // Get the full aligned value, but we only want the full value minus the offset number of bytes.

	u8 MSBShift = ((4 - (offset + 1)) * 8); // A shift value used thoughout.
	u32 MSBMask = Constants::VALUE_U32_MAX << MSBShift; // Mask for getting rid of the unwanted bytes from the aligned value.
	u32 MSBValue = (regValue & MSBMask) >> MSBShift; // Calculate the MSB value part.

	u32 keepMask = ~(MSBMask >> MSBShift); // The keep mask is used to select the bytes in the register which we do not want to change - this mask will be AND with those bytes, while stripping away the other bytes about to be overriden.

	u32 physicalAddress;
	if (getPhysicalAddress(baseAddress, READ, physicalAddress))
		return;

	auto alignedValue = mPhysicalMMU->readWord(IOP, physicalAddress); // Get the full aligned value (from which only the relevant bits will be kept).

	if (getPhysicalAddress(baseAddress, WRITE, physicalAddress)) // Need to get the physical address again, to check for write permissions.
		return;

	mPhysicalMMU->writeWord(IOP, physicalAddress, (alignedValue & keepMask) | MSBValue); // Calculate the new desination memory value and write to it.
}

void IOPCoreInterpreter_s::SWR()
{
	// TODO: check this, dont think its right. This should work for little-endian architectures (ie: x86), but not sure about big-endian. Luckily most machines are little-endian today, so this may never be a problem.
	// MEM[UW] = Rd. Address error or TLB error generated.
	// Unaligned memory write. Alignment occurs on an 4 byte boundary, but this instruction allows an unaligned write. SWR is to be used with SWL, to write a full 32-bit value.
	// SWR writes the least significant bytes (LSB's) depending on the virtual address offset, and stores them in the most significant part of the destination memory.
	// Note that the other bytes already in the register are not changed. They are changed through SWL.
	auto& source1Reg = mIOPCore->R3000->GPR[mInstruction.getIRs()]; // "Base"
	auto& source2Reg = mIOPCore->R3000->GPR[mInstruction.getIRt()];
	const s16 imm = mInstruction.getIImmS();

	u32 unalignedAddress = source1Reg->readWord(IOP) + imm; // Get the unaligned virtual address.
	u32 baseAddress = unalignedAddress & ~static_cast<u32>(0x3); // Strip off the last 2 bits, making sure we are now aligned on a 4-byte boundary.
	u32 offset = unalignedAddress & static_cast<u32>(0x3); // Get the value of the last 2 bits, which will be from 0 -> 3 indicating the byte offset within the 4-byte alignment.

	u32 regValue = source2Reg->readWord(IOP); // Get the full aligned value, but we only want the full value minus the offset number of bytes.

	u8 LSBShift = (offset * 8); // A shift value used thoughout.
	u32 LSBMask = Constants::VALUE_U32_MAX >> LSBShift; // Mask for getting rid of the unwanted bytes from the aligned value.
	u32 LSBValue = (regValue & LSBMask) << LSBShift; // Calculate the LSB value part.

	u32 keepMask = ~(LSBMask << LSBShift); // The keep mask is used to select the bytes in the register which we do not want to change - this mask will be AND with those bytes, while stripping away the other bytes about to be overriden.
	
	u32 physicalAddress;
	if (getPhysicalAddress(baseAddress, READ, physicalAddress))
		return;

	auto alignedValue = mPhysicalMMU->readWord(EE, physicalAddress); // Get the full aligned value (from which only the relevant bits will be kept).

	if (getPhysicalAddress(baseAddress, WRITE, physicalAddress)) // Need to get the physical address again, to check for write permissions.
		return;

	mPhysicalMMU->writeWord(EE, physicalAddress, (alignedValue & keepMask) | LSBValue); // Calculate the new desination memory value and write to it.
}

