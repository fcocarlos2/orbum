#include "stdafx.h"
#include "Common/Types/Registers/PCRegister32_t.h"
#include "PS2Constants/PS2Constants.h"

u32 PCRegister32_t::getPCValue()
{
	return readWordU();
}

void PCRegister32_t::setPCValueRelative(const s32& relativeLocation)
{
	writeWordU(getPCValue() + relativeLocation);
}

void PCRegister32_t::setPCValueAbsolute(const u32& absoluteLocation)
{
	writeWordU(absoluteLocation);
}

void PCRegister32_t::setPCValueNext(const u32 instructionSize)
{
	setPCValueRelative(instructionSize);
}
