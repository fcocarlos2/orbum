#pragma once

#include "Common/Global/Globals.h"

#include "Common/PS2Constants/PS2Constants.h"
#include "Common/Interfaces/PS2ResourcesSubobject.h"
#include "Common/PS2Resources/EE/EECore/EECoreTLB/Types/EECoreTLBEntryInfo_t.h"

class EECoreTLB_t : public PS2ResourcesSubobject
{
public:
	explicit EECoreTLB_t(const PS2Resources_t* const PS2Resources);

	/*
	Performs an iterative lookup on the TLB for the given VPN contained in the PS2VirtualAddress.
	A return value of -1 indicates an entry was not found. Any functions that call this may throw a TLB refill exception if an entry was not found (this function doesn't do this automatically).
	*/
	s32 findTLBIndex(u32 PS2VirtualAddress) const; // -1 indicates not found.

	/*
	Gets the TLB entry at the specified index - use findTLBIndex() to make sure it exists first.
	*/
	const EECoreTLBEntryInfo_t & getTLBEntry(s32 index) const;

	/*
	Sets the TLB entry at the specified index.
	*/
	void setTLBEntry(const EECoreTLBEntryInfo_t & entry, const s32 & index);

	/*
	Gets an index to a new TLB entry position.
	*/
	s32 getNewTLBIndex();

	/*
	A zeroed-TLB entry, pointed to by the MMUHandler initially.
	*/
	static constexpr EECoreTLBEntryInfo_t EMPTY_TLB_ENTRY = {0};

private:
	/*
	TLB entries. See EE Core Users Manual page 120.
	In total there are 48 entries.
	*/
	EECoreTLBEntryInfo_t mTLBEntries[PS2Constants::EE::EECore::MMU::NUMBER_TLB_ENTRIES];
};
