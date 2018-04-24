#pragma once

#include "Common/Constants.hpp"

#include "Resources/Ee/Vpu/Vu/VuUnits.hpp"
#include "Resources/Ee/Vpu/Vu/VuRegisters.hpp"

/// RVu declares the collective VU structure.
class RVu
{
public:
	RVu();

	/// VU structure.
	VuUnit_Vu0 unit_0;
	VuUnit_Vu1 unit_1;
	VuUnit_Base * units[Constants::EE::VPU::VU::NUMBER_VU_CORES];
	
	/// Shared VU registers.
	VuRegister_Fbrst fbrst;
};