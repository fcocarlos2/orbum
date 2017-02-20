#include "stdafx.h"

#include "Common/Global/Globals.h"
#include "Common/Types/Registers/Register32_t.h"

#include "VM/Systems/IOP/IOPCoreInterpreter/IOPCoreInterpreter_s.h"

#include "Resources/IOP/IOPCore/IOPCore_t.h"
#include "Resources/IOP/IOPCore/Types/IOPCoreR3000_t.h"
#include "Resources/IOP/IOPCore/Types/IOPCoreCOP0_t.h"

void IOPCoreInterpreter_s::MFC0()
{
	if (handleCOP0Usable())
		return;

	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getRRt()];
	auto& sourceReg = mIOPCore->COP0->Registers[mInstruction.getRRd()];

	destReg->writeWord(IOP, static_cast<u32>(sourceReg->readWord(IOP)));
}

void IOPCoreInterpreter_s::MTC0()
{
	if (handleCOP0Usable())
		return;

	auto& sourceReg = mIOPCore->R3000->GPR[mInstruction.getRRt()];
	auto& destReg = mIOPCore->COP0->Registers[mInstruction.getRRd()];

	destReg->writeWord(IOP, sourceReg->readWord(IOP));
}

void IOPCoreInterpreter_s::MFHI()
{
	// Rd = HI. No exceptions.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getRRd()];
	auto& source1Reg = mIOPCore->R3000->HI;

	destReg->writeWord(IOP, source1Reg->readWord(IOP));
}

void IOPCoreInterpreter_s::MFLO()
{
	// Rd = LO. No exceptions.
	auto& destReg = mIOPCore->R3000->GPR[mInstruction.getRRd()];
	auto& source1Reg = mIOPCore->R3000->LO;

	destReg->writeWord(IOP, source1Reg->readWord(IOP));
}

void IOPCoreInterpreter_s::MTHI()
{
	// HI = Rd. No exceptions.
	auto& source1Reg = mIOPCore->R3000->GPR[mInstruction.getRRd()];
	auto& destReg = mIOPCore->R3000->HI;

	destReg->writeWord(IOP, source1Reg->readWord(IOP));
}

void IOPCoreInterpreter_s::MTLO()
{
	// LO = Rd. No exceptions.
	auto& source1Reg = mIOPCore->R3000->GPR[mInstruction.getRRd()];
	auto& destReg = mIOPCore->R3000->LO;

	destReg->writeWord(IOP, source1Reg->readWord(IOP));
}
