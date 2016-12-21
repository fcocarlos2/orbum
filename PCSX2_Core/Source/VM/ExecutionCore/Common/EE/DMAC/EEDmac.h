#pragma once

#include "Common/Global/Globals.h"

#include "Common/Interfaces/VMExecutionCoreComponent.h"
#include "PS2Resources/EE/DMAC/Types/DMAtag_t.h"
#include "PS2Constants/PS2Constants.h"
#include "Common/Tables/EEDmacChannelTable/EEDmacChannelTable.h"

using ChannelProperties_t = EEDmacChannelTable::ChannelProperties_t;
using Direction_t = EEDmacChannelTable::Direction_t;
using LogicalMode_t = EEDmacChannelTable::LogicalMode_t;

class EEDmac_t;
class PhysicalMMU_t;
class EEDmacChannel_t;

/*
The EE DMAC sytem controls the execution of the EE DMAC and transfers through DMA.

The EE DMAC is synced to the BUSCLK clock source, and at most transfers a qword (a 128-bit data unit) every tick on slice and burst channels.
In a slice physical transfer mode, 8 qwords are transfered before the DMAC releases the bus to the CPU - it waits for a 'DMA request' command before continuing.
In a burst physical transfer mode, n qwords are transfered all at once - the CPU must wait for the DMAC to release the bus.

See EE Users Manual page 41 onwards.

TODO: Not implemented:
 - MFIFO handling.
 - D_ENABLER/W handling.
 - Cycle stealing.

TODO: Speedups can be done here:
 - Dont need to transfer 1-qword at a time.
 - Dont need to turn on cycle stealing if requested? Kind of redundant in an emulator.
*/
class EEDmac : public VMExecutionCoreComponent
{
public:
	explicit EEDmac(VMMain * vmMain);
	~EEDmac();

	/*
	Check through the channels and initate data transfers. 
	Slice channels transfer 8 qwords (128 bytes) before suspending transfer, where as burst channels transfer all data without suspending.
	TODO: Currently it is assumed that the software uses the DMAC correctly (such as using correct chain mode for a channel). Need to add in checks?
	*/
	s64 executionStep(const ClockSource_t & clockSource) override;

private:
	/*
	Context variables set by executionStep() in each cycle. 
	Used by all of the functions below.
	*/
	u32 mChannelIndex;
	std::shared_ptr<EEDmac_t> mDMAC;
	std::shared_ptr<PhysicalMMU_t> mEEMMU;
	std::shared_ptr<EEDmacChannel_t> mChannel;
	const ChannelProperties_t * mChannelProperties;

	

	// DMAC helper functions.

	/*
	Do a normal logical mode transfer through the specified DMA channel.
	*/
	void executionStep_Normal();

	/*
	Do a chain logical mode transfer through the specified DMA channel.
	*/
	void executionStep_Chain();

	/*
	Do a interleaved logical mode transfer through the specified DMA channel.
	*/
	void executionStep_Interleaved();

	/*
	Returns if the DMAC is enabled.
	*/
	bool isDMACEnabled() const;

	/*
	Returns if there is an DMA transfer interrupt pending, indicating the EE Core should be interrupted.
	*/
	bool isInterruptPending() const;

	/*
	Raises an interrupt request with the EE Core, by setting the exception context.
	*/
	void raiseInterrupt() const;

	/*
	Transfers data units (128-bits) between mem <-> channel.
	Returns the number of data units transfered.
	On no data available, returns 0.
	TODO: for now, always attempts to transfer one unit (and returns 1 on success, 0 on "try again next cycle").
	*/
	u32 transferData();

	/*
	Sets the channel state for suspend conditions.
	*/
	void setStateSuspended() const;

	/*
	Sets the channel state for failed transfer conditions.
	TODO: not yet implemented, throws runtime_error.
	*/
	void setStateFailedTransfer() const;





	// Raw data transfer helper functions.

	/*
	Reads a qword from memory using the address given.
	SPRAccess controls if the read is through the EE main memory or the EE Core scratchpad.
	*/
	u128 readDataMemory(u32 PhysicalAddressOffset, bool SPRAccess);

	/*
	Writes a qword to memory using the address given.
	SPRAccess controls if the write is through the EE main memory or the EE Core scratchpad.
	*/
	void writeDataMemory(u32 PhysicalAddressOffset, bool SPRAccess, u128 data);





	// Stall control helper functions.

	/*
	Returns if source stall control checks should occur, by checking the channel direction and D_CTRL.STS.
	*/
	bool isSourceStallControlOn() const;

	/*
	Returns if drain stall control checks should occur, by checking the channel direction and D_CTRL.STD.
	*/
	bool isDrainStallControlOn() const;

	/*
	Returns true if MADR + 8 > STADR, which is the condition a drain channel stalls on with stall control.
	Callee is responsible for setting the D_STAT.SIS bit.
	TODO: According to the docs, "SIS bit doesn't change even if the transfer restarts"! PS2 OS sets it back to 0?
	*/
	bool isDrainStallControlWaiting() const;

	/*
	Sets the DMAC STADR register to the current channel conditions.
	*/
	void setDMACStallControlSTADR() const;

	/*
	Sets the DMAC STAT.SISx bit to the current channel.
	*/
	void setDMACStallControlSIS() const;






	// Chain mode helper functions.

	/*
	Temporary context variables, set by the chain mode functions.
	*/
	DMAtag_t mDMAtag;

	/*
	Sets mDMAtag to the tag from the TADR register.
	Also sets the CHCH.TAG field to bits 16-31 of the DMAtag read. If CHCR.TTE is set, transfers the tag.
	Returns if it was successful (true) or not (false) - use to determine if an early exit should occur (need to wait for more data).
	*/
	bool readChainSourceTag();

	/*
	Sets mDMAtag to the tag from the channel queue.
	Also sets the CHCH.TAG field to bits 16-31 of the DMAtag read. If CHCR.TTE is set, transfers the tag.
	Returns if it was successful (true) or not (false) - use to determine if an early exit should occur (need to wait for more data).
	*/
	bool readChainDestTag();

	/*
	Chain DMAtag handler functions. Consult page 59 - 61 of EE Users Manual.
	*/
	void INSTRUCTION_UNSUPPORTED();
	void SRC_CNT();
	void SRC_NEXT();
	void SRC_REF();
	void SRC_REFS();
	void SRC_REFE();
	void SRC_CALL();
	void SRC_RET();
	void SRC_END();
	void DST_CNT();
	void DST_CNTS();
	void DST_END();

	/*
	Static arrays needed to call the appropriate DMAtag handler function.
	There is one for source and destination chain modes. See page 60 and 61 of EE Users Manual for the list of applicable DMAtag instructions.
	*/
	void(EEDmac::*const SRC_CHAIN_INSTRUCTION_TABLE[PS2Constants::EE::DMAC::NUMBER_CHAIN_INSTRUCTIONS])() =
	{
		&EEDmac::SRC_REFE,
		&EEDmac::SRC_CNT,
		&EEDmac::SRC_NEXT,
		&EEDmac::SRC_REF,
		&EEDmac::SRC_REFS,
		&EEDmac::SRC_CALL,
		&EEDmac::SRC_RET,
		&EEDmac::SRC_END
	};
	void(EEDmac::*const DST_CHAIN_INSTRUCTION_TABLE[PS2Constants::EE::DMAC::NUMBER_CHAIN_INSTRUCTIONS])() =
	{
		&EEDmac::DST_CNTS,
		&EEDmac::DST_CNT,
		&EEDmac::INSTRUCTION_UNSUPPORTED,
		&EEDmac::INSTRUCTION_UNSUPPORTED,
		&EEDmac::INSTRUCTION_UNSUPPORTED,
		&EEDmac::INSTRUCTION_UNSUPPORTED,
		&EEDmac::INSTRUCTION_UNSUPPORTED,
		&EEDmac::DST_END,
	};




	// Interleaved mode helper functions.

	/*
	Returns if the transfer or skip limit has been reached.
	(The two states are mutually exclusive).
	*/
	bool isInterleaveLimitReached();

};