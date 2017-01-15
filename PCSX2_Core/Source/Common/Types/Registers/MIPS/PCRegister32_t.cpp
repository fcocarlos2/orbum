#include "stdafx.h"

#include "Common/Types/Registers/MIPS/PCRegister32_t.h"

void PCRegister32_t::setPCValueRelative(const s32& relativeLocation)
{
	writeWord(Context_t::RAW, readWord(Context_t::RAW) + relativeLocation);
}

void PCRegister32_t::setPCValueAbsolute(const u32& absoluteLocation)
{
	writeWord(Context_t::RAW, absoluteLocation);
}

void PCRegister32_t::setPCValueNext(const u32 instructionSize)
{
	setPCValueRelative(instructionSize);
}