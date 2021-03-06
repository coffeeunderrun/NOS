// Pager.cpp - Working with page tables.

#include <cstdint>
#include <new>
#include <Pager.h>
#include <Symbol.h>
#include <Console.h>
#include INC_SUBARCH(PageTable.h)
#include INC_BITS(SystemRegs.h)

namespace Kernel
{
	namespace Pager
	{
		Memory::PageBits MappedSize(uintptr_t virt)
		{
			return Memory::PGB_INV;
		}

		template<Memory::PageBits bits> void MapPage(Memory::PhysAddr phys, uintptr_t virt, Memory::MemType type)
		{
			static_assert(IsValidSize(bits), "invalid page size");
		}

		template<Memory::PageBits bits> void UnmapPage(uintptr_t virt)
		{
			static_assert(IsValidSize(bits), "invalid page size");
		}

		template<> void MapPage<Memory::PGB_4K>(Memory::PhysAddr phys, uintptr_t virt, Memory::MemType type)
		{
		}

		template<> void UnmapPage<Memory::PGB_4K>(uintptr_t virt)
		{
		}

		template<> void MapPage<Memory::PGB_64K>(Memory::PhysAddr phys, uintptr_t virt, Memory::MemType type)
		{
		}

		template<> void UnmapPage<Memory::PGB_64K>(uintptr_t virt)
		{
		}

		template<> void MapPage<Memory::PGB_1M>(Memory::PhysAddr phys, uintptr_t virt, Memory::MemType type)
		{
		}

		template<> void UnmapPage<Memory::PGB_1M>(uintptr_t virt)
		{
		}

		template<> void MapPage<Memory::PGB_16M>(Memory::PhysAddr phys, uintptr_t virt, Memory::MemType type)
		{
		}

		template<> void UnmapPage<Memory::PGB_16M>(uintptr_t virt)
		{
		}

		Memory::PhysAddr VirtToPhys(uintptr_t addr)
		{
			bool kernel;
			unsigned int tab, entry;

			//Console::WriteFormat("VirtToPhys(%p)\n", addr);

			if(addr >= MinKernelVirt)
				kernel = true;
			else if(addr <= MaxUserVirt)
				kernel = false;
			else
				return ~0;

			addr &= (1ULL << (64 - PageSizeOffset)) - 1;

			if constexpr(InitialLookupLevel == 0)
			{
				entry = addr >> (4 * GranuleSize - 9);
				PageTableEntry& pte0 = (kernel ? PageTableLevel<0>::TableKernel(0) : PageTableLevel<0>::TableUser(0)).Entry(entry);
				if(pte0.IsInvalid())
					return ~0;
				if(pte0.IsBlock()) // No blocks at this level.
					return ~0;
			}

			if constexpr(InitialLookupLevel <= 1)
			{
				entry = (addr >> (3 * GranuleSize - 6)) & ((1ULL << (GranuleSize - 3)) - 1);
				tab = addr >> (4 * GranuleSize - 9);
				PageTableEntry& pte1 = (kernel ? PageTableLevel<1>::TableKernel(tab) : PageTableLevel<1>::TableUser(tab)).Entry(entry);
				if(pte1.IsInvalid())
					return ~0;
				if(pte1.IsBlock())
					return pte1.Phys() | (addr & ((1ULL << (3 * GranuleSize - 6)) - 1));
			}

			if constexpr(InitialLookupLevel <= 2)
			{
				entry = (addr >> (2 * GranuleSize - 3)) & ((1ULL << (GranuleSize - 3)) - 1);
				tab = addr >> (3 * GranuleSize - 6);
				PageTableEntry& pte2 = (kernel ? PageTableLevel<2>::TableKernel(tab) : PageTableLevel<2>::TableUser(tab)).Entry(entry);
				if(pte2.IsInvalid())
					return ~0;
				if(pte2.IsBlock())
					return pte2.Phys() | (addr & ((1ULL << (2 * GranuleSize - 3)) - 1));
			}

			entry = (addr >> GranuleSize) & ((1ULL << (GranuleSize - 3)) - 1);
			tab = addr >> (2 * GranuleSize - 3);
			PageTableEntry& pte3 = (kernel ? PageTableLevel<3>::TableKernel(tab) : PageTableLevel<3>::TableUser(tab)).Entry(entry);
			if(pte3.IsPage())
				return pte3.Phys() | (addr & ((1ULL << GranuleSize) - 1));

			return ~0;
		}
	}
}
