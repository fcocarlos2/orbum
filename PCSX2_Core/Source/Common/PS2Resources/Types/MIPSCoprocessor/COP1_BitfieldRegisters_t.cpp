#include "stdafx.h"

#include "Common/PS2Resources/Types/MIPSCoprocessor/COP1_BitfieldRegisters_t.h"

COP1RegisterIRR_t::COP1RegisterIRR_t()
{
	registerField(Fields::Rev, "Rev", 0, 8, 0);
	registerField(Fields::Imp, "Imp", 8, 8, 0x2E);
}

COP1RegisterCSR_t::COP1RegisterCSR_t()
{
	registerField(Fields::SU, "SU", 3, 1, 0);
	registerField(Fields::SO, "SO", 4, 1, 0);
	registerField(Fields::SD, "SD", 5, 1, 0);
	registerField(Fields::SI, "SI", 6, 1, 0);
	registerField(Fields::U, "U", 14, 1, 0);
	registerField(Fields::O, "O", 15, 1, 0);
	registerField(Fields::D, "D", 16, 1, 0);
	registerField(Fields::I, "I", 17, 1, 0);
	registerField(Fields::C, "C", 23, 1, 0);
}