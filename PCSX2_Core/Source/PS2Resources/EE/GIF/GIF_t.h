#pragma once

#include <memory>

#include "Common/Interfaces/PS2ResourcesSubobject.h"

class Register32_t;
class ConstantMemory_t;

class GIF_t : public PS2ResourcesSubobject
{
public:
	explicit GIF_t(const PS2Resources_t *const PS2Resources);

	/*
	GIF memory mapped registers. See page 21 of EE Users Manual.
	*/
	std::shared_ptr<Register32_t>     CTRL;
	std::shared_ptr<Register32_t>     MODE;
	std::shared_ptr<Register32_t>     STAT;
	std::shared_ptr<ConstantMemory_t> MEMORY_3030;
	std::shared_ptr<Register32_t>     TAG0;
	std::shared_ptr<Register32_t>     TAG1;
	std::shared_ptr<Register32_t>     TAG2;
	std::shared_ptr<Register32_t>     TAG3;
	std::shared_ptr<Register32_t>     CNT;
	std::shared_ptr<Register32_t>     P3CNT;
	std::shared_ptr<Register32_t>     P3TAG;
	std::shared_ptr<ConstantMemory_t> MEMORY_30B0;
};
