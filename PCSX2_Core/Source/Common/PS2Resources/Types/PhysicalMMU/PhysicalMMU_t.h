#pragma once

#include <memory>

#include "Common/Global/Globals.h"

class MappedMemory_t;

/*
The PhysicalMMU_t component is responsible for converting the PS2's physical addresses into client storage objects (which is required to properly run a program on the client system).
The remapping method is actually just a page table... but sort of in reverse (PS2 "physical" -> client)!
This means that in the emulator, there are 2 page tables:
- One will be a page table for mapping PS2 virtual addresses into PS2 physical addresses (implmented as MMU sub components in the Interpreter & Recompliler).
- The other (this one) is a page table for mapping PS2 physical addresses into client virtual addresses. It is labeled PhysicalMMU_t to avoid confusion with the PS2 TLB / MMU components.

By using this, it is up to the user to make sure no addresses overlap - they will be overwritten and existing map data lost.

It will throw different runtime errors when the following conditions occur:
- range_error exception if more than PAGE_TABLE_MAX_SIZE in the page tableis accessed.
- runtime_error exception if the returned PFN from the page table was null (indicates invalid entry, needs to be mapped first).

Why is an object used instead of a raw pointer to a block of memory?
Some memory regions of the PS2 require special attributes - such as the reserved regions of the EE registers. When writing to these regions,
the write is disregarded. When a read is performed, an indeterminate value is returned (0 for some registers due to undocumented behaviour).



Example of usage within the context of the EE (IOP has separate physical address space):

The EE Core page table is implemented as a 2 level system with a primary "directory" size of 4,194,304B (4MB addressing chunks) and a secondary "page" size of 16B. 
 2 Levels are used to reduce memory usage by only allocating the page tables within a directory that are needed.

The reason that 16B is used on the second levels due to the physical memory map of the EE & GS registers (timers, vu's, dmac, etc, starting on page 21 of the EE Users Manual). 
 Each register is (at minimum) aligned on a 16B boundary, and we need to reflect this. If a larger page size was used (say 4KB which is a normal value), then we would need
 to somehow make sure that each offset within a page which is a multiple of 16 pointed to a different client storage object - but this is a problem because the physical 
 frame number only points to 1 object. Therefore for now we need to make the page size 16B until a better solution comes along.
This also means that a runtime error will happen if there is a read or write after the edge of the object.

According to the PS2 docs mentioned above, the EE's physical address space is as follows:

0x00000000 - 0x0FFFFFFF 256MB main memory (of which I assume 32MB is accessable from 0x00000000 onwards and the other space raises a bus error?).
0x10000000 - 0x11FFFFFF EE Registers (timers, vu's, dmac, etc).
0x12000000 - 0x13FFFFFF GS Registers.
0x14000000 - 0x1FBFFFFF Reserved (undefined behaviour).
0x1FC00000 - 0x1FFFFFFF Boot rom area (max 4MB).
0x20000000 - 0xFFFFFFFF Extended memory and NO MOUNT (but there is no extended memory in the PS2, so it is undefined and hence not mapped or used).

We can leave out the extended memory and NO MOUNT region from the page table, as these are never used and there is no utilised memory onwards from them.
Therefore, we can limit the map to 512MB (0x00000000 - 0x1FFFFFFF).

By using a directory size of 4MB and a page size of 16B, with a 512 MB address range:
 - Number of directory entries = 512MB / 4MB = 128. Therefore 7 bits are needed to represent the virtual directory number (0 -> 127).
 - Number of page table entries per directory = 4MB / 16B = 262,144. Therefore 18 bits are needed to represent the virtual page number (0 -> 262,143).
 - The offset (within 16B) requires 4 bits.
 - Total number of bits required = 29, which is correct for addressing 512MB. This is done within a 32-bit integer type (upper bits unused).
 =============================================================
 | 28            22 | 21                       4  | 3      0 |
 | VIRTUAL DIR. NUM |     VIRTUAL PAGE NUMBER     |  OFFSET  |
 =============================================================
 */
class PhysicalMMU_t
{
public:
	explicit PhysicalMMU_t(const size_t & MaxAddressableSizeBytes, const size_t & DirectorySizeBytes, const size_t & PageSizeBytes);
	~PhysicalMMU_t();

	/*
	Maps the given client virtual memory address and length to a given PS2 "physical" address.
	Once this has been executed sucesfully, you will be able to read and write to the PS2 physical address, which will automatically tranlate it to the correct client memory object.
	The functions to read and write are "{read or write}{type}{[U]nsigned or [S]igned}()" (listed below). Once the correct object has been retreived, a call will be made to the same function of that object.

	Note that this function simply remaps the memory in a linear fashion, meaning that for example, a PS2 physical address of 0x00000400 - 0x00000600 will map directly to (example mapping) 0x1234A000 - 0x1234A200

	clientStorage = An object which implements the MappedMemory class type.
	*/
	void mapMemory(const std::shared_ptr<MappedMemory_t> & clientStorage);

	/*
	These functions, given a PS2 "physical" address, will read or write a value from/to the address.
	The address is automatically translated into the correct address through the page table.
	You cannot use these functions before mapMemory() has been called - it will return an runtime_error exception otherwise.
	The functions have the syntax "{read or write}{type}{[U]nsigned or [S]igned}()".
	Unfortunately C++ does not allow templating of virtual functions defined in the parent class, so a read/write for each type has to be made.
	*/
	u8 readByteU(u32 PS2PhysicalAddress) const;
	void writeByteU(u32 PS2PhysicalAddress, u8 value) const;
	s8 readByteS(u32 PS2PhysicalAddress) const;
	void writeByteS(u32 PS2PhysicalAddress, s8 value) const;
	u16 readHwordU(u32 PS2PhysicalAddress) const;
	void writeHwordU(u32 PS2PhysicalAddress, u16 value) const;
	s16 readHwordS(u32 PS2PhysicalAddress) const;
	void writeHwordS(u32 PS2PhysicalAddress, s16 value) const;
	u32 readWordU(u32 PS2PhysicalAddress) const;
	void writeWordU(u32 PS2PhysicalAddress, u32 value) const;
	s32 readWordS(u32 PS2PhysicalAddress) const;
	void writeWordS(u32 PS2PhysicalAddress, s32 value) const;
	u64 readDwordU(u32 PS2PhysicalAddress) const;
	void writeDwordU(u32 PS2PhysicalAddress, u64 value) const;
	s64 readDwordS(u32 PS2PhysicalAddress) const;
	void writeDwordS(u32 PS2PhysicalAddress, s64 value) const;

private:
	/*
	Internal parameters calculated in the constructor.
	*/
	const size_t MAX_ADDRESSABLE_SIZE_BYTES;
	const size_t DIRECTORY_SIZE_BYTES;
	const size_t PAGE_SIZE_BYTES;
	const size_t DIRECTORY_ENTRIES;
	const size_t PAGE_ENTRIES;
	const size_t OFFSET_BITS;
	const size_t OFFSET_MASK;
	const size_t DIRECTORY_BITS;
	const size_t DIRECTORY_MASK;
	const size_t PAGE_BITS;
	const size_t PAGE_MASK;

	/*
	The page table which holds all of the page table entries, mapping the addresses.
	The directories are kept in this, which point to individual pages.
	The individual pages are only allocated on access, thereby saving memory.
	(A pointer to an array of directories, each directory pointing to an mComponents of pages, each page pointing to some memory.)
	*/
	std::shared_ptr<MappedMemory_t>** mPageTable;

	/*
	Translates the given PS2 physical address to the stored client object by using the page table. The returned object can then be used to read or write to an address.
	*/
	std::shared_ptr<MappedMemory_t> & getClientMappedMemory(u32 baseVDN, u32 baseVPN) const;

	/*
	Helper functions for mapMemory & others to 
	 - Determine which directory a page belongs to (if they were layed out end to end).
	 - The page offset in a directory.
	 - The absolute page (if they were layed out end to end).
	 - Allocate a new directory of pages if it doesnt exist.
	*/
	u32 getDirectoryFromPageOffset(u32 absPageIndexStart, u32 pageOffset) const;
	u32 getDirPageFromPageOffset(u32 absPageIndexStart, u32 pageOffset) const;
	u32 getAbsPageFromDirAndPageOffset(u32 absDirectoryIndex, u32 pageOffset) const;
	void allocDirectory(u32 directoryIndex) const;

	/*
	Gets the VDN (virtual directory number) from a given PS2 physical address.
	*/
	u32 getVDN(u32 PS2MemoryAddress) const;

	/*
	Gets the VPN (virtual page number) from a given PS2 physical address.
	*/
	u32 getVPN(u32 PS2MemoryAddress) const;

	/*
	Gets the offset from a given PS2 physical address.
	*/
	u32 getOffset(u32 PS2MemoryAddress) const;
};