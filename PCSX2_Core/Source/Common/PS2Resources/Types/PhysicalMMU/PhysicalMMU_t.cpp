#include "stdafx.h"

#include <stdexcept>
#include <cmath>

#include "Common/Global/Globals.h"
#include "Common/PS2Resources/Types/PhysicalMMU/PhysicalMMU_t.h"
#include "Common/PS2Resources/Types/MappedMemory/MappedMemory_t.h"

PhysicalMMU_t::PhysicalMMU_t(const size_t & MaxAddressableSizeBytes, const size_t & DirectorySizeBytes, const size_t & PageSizeBytes) :
	MAX_ADDRESSABLE_SIZE_BYTES(MaxAddressableSizeBytes),
	DIRECTORY_SIZE_BYTES(DirectorySizeBytes),
	PAGE_SIZE_BYTES(PageSizeBytes),
	DIRECTORY_ENTRIES(MAX_ADDRESSABLE_SIZE_BYTES / DIRECTORY_SIZE_BYTES),
	PAGE_ENTRIES(DIRECTORY_SIZE_BYTES / PAGE_SIZE_BYTES),
	OFFSET_BITS(static_cast<u32>(log2(PAGE_SIZE_BYTES))),
	OFFSET_MASK(PAGE_SIZE_BYTES - 1),
	DIRECTORY_BITS(static_cast<u32>(log2(DIRECTORY_ENTRIES))),
	DIRECTORY_MASK(DIRECTORY_ENTRIES - 1),
	PAGE_BITS(static_cast<u32>(log2(PAGE_ENTRIES))),
	PAGE_MASK(PAGE_ENTRIES - 1)
{
	// Allocate the page table (directories).
	mPageTable = new std::shared_ptr<MappedMemory_t>*[DIRECTORY_ENTRIES];

	// Ok... VS compiler crashes if we try to do array initalisation above... so try memset to do it? This works...
	// Sets all of the directory to nullptr's.
	memset(mPageTable, 0, DIRECTORY_ENTRIES * sizeof(mPageTable[0]));
}

PhysicalMMU_t::~PhysicalMMU_t()
{
	// Destroy any allocated pages.
	for (u32 i = 0; i < DIRECTORY_ENTRIES; i++)
		if (mPageTable[i] != nullptr) delete[] mPageTable[i];
	
	// Destroy the page table (directories).
	delete[] mPageTable;
}

void PhysicalMMU_t::mapMemory(const std::shared_ptr<MappedMemory_t> & clientStorage)
{
	// Do not do anything for getStorageSize equal to 0.
	if (clientStorage->getStorageSize() == 0) 
		return;

	// Get the base virtual directory number (VDN) and virtual page number (VPN).
	u32 baseVDN = getVDN(clientStorage->getPS2PhysicalAddress());
	u32 baseVPN = getVPN(clientStorage->getPS2PhysicalAddress());

	// Work out how many pages the memory block occupies. If it is not evenly divisible, need to add on an extra page to account for the extra length.
	// Thank you to Ian Nelson for the round up solution: http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division, very good.
	size_t pagesCount = (clientStorage->getStorageSize() + PAGE_SIZE_BYTES - 1) / PAGE_SIZE_BYTES;

	// Get absolute linear page position that we start mapping memory from.
	u32 absPageStartIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);
	
	// Set the base page index of the storage object, so it can calculate the byte(s) it needs to access later on when its used.
	clientStorage->setAbsMappedPageIndex(absPageStartIndex);

	// Iterate through the pages to set the client addresses.
	u32 absDirectoryIndex;
	u32 absPageIndex;
	for (u32 i = 0; i < pagesCount; i++)
	{
		// Get absolute directory and page index.
		absDirectoryIndex = getDirectoryFromPageOffset(absPageStartIndex, i);
		absPageIndex = getDirPageFromPageOffset(absPageStartIndex, i);

		// Make sure directory is allocated.
		allocDirectory(absDirectoryIndex);

		// Check that there is no existing map data - log a warning if there is.
		if (mPageTable[absDirectoryIndex][absPageIndex] != nullptr)
			logDebug("(%s, %d) Warning! Physical MMU mapped storage object \"%s\" @ 0x%08X overwritten with object \"%s\". Please fix!",
				__FILENAME__, __LINE__,
				mPageTable[absDirectoryIndex][absPageIndex]->getMnemonic(),
				clientStorage->getPS2PhysicalAddress(),
				clientStorage->getMnemonic());

		// Map memory entry.
		mPageTable[absDirectoryIndex][absPageIndex] = clientStorage;
	}
}

u32 PhysicalMMU_t::getVDN(u32 PS2PhysicalAddress) const
{
	return (PS2PhysicalAddress >> (OFFSET_BITS + PAGE_BITS)) & DIRECTORY_MASK;
}

u32 PhysicalMMU_t::getVPN(u32 PS2PhysicalAddress) const
{
	return (PS2PhysicalAddress >> OFFSET_BITS) & PAGE_MASK;
}

u32 PhysicalMMU_t::getOffset(u32 PS2PhysicalAddress) const
{
	return PS2PhysicalAddress & OFFSET_MASK;
}

u32 PhysicalMMU_t::getDirectoryFromPageOffset(u32 absPageIndexStart, u32 pageOffset) const
{
	return (absPageIndexStart + pageOffset) / PAGE_ENTRIES;
}

u32 PhysicalMMU_t::getDirPageFromPageOffset(u32 absPageIndexStart, u32 pageOffset) const
{
	return (absPageIndexStart + pageOffset) % PAGE_ENTRIES;
}

u32 PhysicalMMU_t::getAbsPageFromDirAndPageOffset(u32 absDirectoryIndex, u32 pageOffset) const
{
	return absDirectoryIndex * PAGE_ENTRIES + pageOffset;
}

void PhysicalMMU_t::allocDirectory(u32 directoryIndex) const
{
	// Allocate VDN only if empty and set to null initially.
	if (mPageTable[directoryIndex] == nullptr) {
		mPageTable[directoryIndex] = new std::shared_ptr<MappedMemory_t>[PAGE_ENTRIES];
		// Ok... VS compiler crashes if we try to do array initalisation above... so try memset to do it? This works...
		memset(mPageTable[directoryIndex], 0, PAGE_ENTRIES * sizeof(mPageTable[directoryIndex][0]));
	}
}

std::shared_ptr<MappedMemory_t> & PhysicalMMU_t::getClientMappedMemory(u32 baseVDN, u32 baseVPN) const
{
	// Lookup the page in the page table to get the client storage object (aka page frame number (PFN)).
	// If the directory or client storage object comes back as nullptr (= 0), throw a runtime exception.
	std::shared_ptr<MappedMemory_t>* tableDirectory = mPageTable[baseVDN];
	if (tableDirectory == nullptr)
	{
		char message[1000];
		sprintf_s(message, "Not found: Given baseVDN returned a null VDN. Check input for error, or maybe it has not been mapped in the first place. VDN = %X, VPN = %X.", baseVDN, baseVPN);
		throw std::runtime_error(message);
	}
	std::shared_ptr<MappedMemory_t> & clientMappedMemory = tableDirectory[baseVPN];
	if (clientMappedMemory == nullptr)
	{
		char message[1000];
		sprintf_s(message, "Not found: Given baseVPN returned a null PFN. Check input for error, or maybe it has not been mapped in the first place. VDN = %X, VPN = %X.", baseVDN, baseVPN);
		throw std::runtime_error(message);
	}

	// Return storage object.
	return clientMappedMemory;
}

u8 PhysicalMMU_t::readByteU(u32 PS2PhysicalAddress) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the read on the storage object at the specified index.
	return clientMappedMemory->readByteU(storageIndex);
}

void PhysicalMMU_t::writeByteU(u32 PS2PhysicalAddress, u8 value) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the write on the storage object at the specified index.
	clientMappedMemory->writeByteU(storageIndex, value);
}

s8 PhysicalMMU_t::readByteS(u32 PS2PhysicalAddress) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	u32 baseVDN = getVDN(PS2PhysicalAddress);
	u32 baseVPN = getVPN(PS2PhysicalAddress);
	u32 pageOffset = getOffset(PS2PhysicalAddress);
	u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object.
	std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the read on the storage object at the specified index.
	return clientMappedMemory->readByteS(storageIndex);
}

void PhysicalMMU_t::writeByteS(u32 PS2PhysicalAddress, s8 value) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the write on the storage object at the specified index.
	clientMappedMemory->writeByteS(storageIndex, value);
}

u16 PhysicalMMU_t::readHwordU(u32 PS2PhysicalAddress) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the read on the storage object at the specified index.
	return clientMappedMemory->readHwordU(storageIndex);
}

void PhysicalMMU_t::writeHwordU(u32 PS2PhysicalAddress, u16 value) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the write on the storage object at the specified index.
	clientMappedMemory->writeHwordU(storageIndex, value);
}

s16 PhysicalMMU_t::readHwordS(u32 PS2PhysicalAddress) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the read on the storage object at the specified index.
	return clientMappedMemory->readHwordS(storageIndex);
}

void PhysicalMMU_t::writeHwordS(u32 PS2PhysicalAddress, s16 value) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the write on the storage object at the specified index.
	clientMappedMemory->writeHwordS(storageIndex, value);
}

u32 PhysicalMMU_t::readWordU(u32 PS2PhysicalAddress) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the read on the storage object at the specified index.
	return clientMappedMemory->readWordU(storageIndex);
}

void PhysicalMMU_t::writeWordU(u32 PS2PhysicalAddress, u32 value) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the write on the storage object at the specified index.
	clientMappedMemory->writeWordU(storageIndex, value);
}

s32 PhysicalMMU_t::readWordS(u32 PS2PhysicalAddress) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the read on the storage object at the specified index.
	return clientMappedMemory->readWordS(storageIndex);
}

void PhysicalMMU_t::writeWordS(u32 PS2PhysicalAddress, s32 value) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the write on the storage object at the specified index.
	clientMappedMemory->writeWordS(storageIndex, value);
}

u64 PhysicalMMU_t::readDwordU(u32 PS2PhysicalAddress) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the read on the storage object at the specified index.
	return clientMappedMemory->readDwordU(storageIndex);
}

void PhysicalMMU_t::writeDwordU(u32 PS2PhysicalAddress, u64 value) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the write on the storage object at the specified index.
	clientMappedMemory->writeDwordU(storageIndex, value);
}

s64 PhysicalMMU_t::readDwordS(u32 PS2PhysicalAddress) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the read on the storage object at the specified index.
	return clientMappedMemory->readDwordS(storageIndex);
}

void PhysicalMMU_t::writeDwordS(u32 PS2PhysicalAddress, s64 value) const
{
	// Get the virtual directory number (VDN), virtual page number (VPN), absolute page number & offset.
	const u32 baseVDN = getVDN(PS2PhysicalAddress);
	const u32 baseVPN = getVPN(PS2PhysicalAddress);
	const u32 pageOffset = getOffset(PS2PhysicalAddress);
	const u32 absPageIndex = getAbsPageFromDirAndPageOffset(baseVDN, baseVPN);

	// Get client storage object and calculate the storage index to access.
	const std::shared_ptr<MappedMemory_t> & clientMappedMemory = getClientMappedMemory(baseVDN, baseVPN);
	const u32 storageIndex = (absPageIndex - clientMappedMemory->getAbsMappedPageIndex()) * PAGE_SIZE_BYTES + pageOffset;

	// Perform the write on the storage object at the specified index.
	clientMappedMemory->writeDwordS(storageIndex, value);
}