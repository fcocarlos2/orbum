#pragma once

#include "Common/Global/Globals.h"
#include "Common/Types/Registers/Register32_t.h"

/*
SBUS_MSCOM (F200) register.
Writes discarded for IOP.
*/
class SBUSRegister_MSCOM_t : public Register32_t
{
public:
	SBUSRegister_MSCOM_t();
	void writeWord(const Context_t& context, u32 value) override;
};

/*
SBUS_MSFLG (F220) register.
Write sNOT AND (clears) or OR with the previous value.
*/
class SBUSRegister_MSFLG_t : public Register32_t
{
public:
	SBUSRegister_MSFLG_t();
	void writeWord(const Context_t& context, u32 value) override;
};

/*
SBUS_SMFLG (F230) register.
Writes NOT AND (clears) or OR with the previous value.
*/
class SBUSRegister_SMFLG_t : public Register32_t
{
public:
	SBUSRegister_SMFLG_t();
	void writeWord(const Context_t& context, u32 value) override;
};

/*
SBUS_F240 register.
Manipulates with magic values on reads and writes.
*/
class SBUSRegister_F240_t : public Register32_t
{
public:
	SBUSRegister_F240_t();
	u16 readHword(const Context_t& context, size_t arrayIndex) override;
	u32 readWord(const Context_t& context) override;
	void writeHword(const Context_t& context, size_t arrayIndex, u16 value) override;
	void writeWord(const Context_t& context, u32 value) override;
};

/*
SBUS_F300 register.
TODO: not currently implemented properly, throws runtime_error. See HwRead.cpp and sif2.cpp.
*/
class SBUSRegister_F300_t : public Register32_t
{
public:
	SBUSRegister_F300_t();
	u32 readWord(const Context_t& context) override;
	void writeWord(const Context_t& context, u32 value) override;
};
