// PageTable.h - ARMv8 page table class.

#ifndef __ARCH_ARM_V8_PAGETABLE_H__
#define __ARCH_ARM_V8_PAGETABLE_H__

#include INC_SUBARCH(PageTableEntry.h)
#include INC_VENDOR(Entry.h)
#include <Symbol.h>
#include <Memory.h>

namespace Kernel
{
	namespace Pager
	{
		static const unsigned long PageSizeOffset = 25;
		static const Memory::PageBits GranuleSize = Memory::PGB_4K;
		static const unsigned int InitialLookupLevel = (PageSizeOffset + 5 * GranuleSize - 76) / (GranuleSize - 3);

		static const uintptr_t MaxUserVirt = (1ULL << (64 - PageSizeOffset)) - 1;
		static const uintptr_t MinKernelVirt = ~((1ULL << (64 - PageSizeOffset)) - 1);

		static const unsigned long PageRecursiveKernel = 510;
		static const unsigned long PageRecursiveUser   = 509;
/*
		template<unsigned int> constexpr uintptr_t PageTableAddrKernel = ~0;
		template<unsigned int> constexpr uintptr_t PageTableAddrUser = ~0;

		template<> constexpr uintptr_t PageTableAddrKernel<3> = MinKernelVirt + (PageRecursiveKernel << (67 - PageSizeOffset - GranuleSize));
		template<> constexpr uintptr_t PageTableAddrUser<3> = MinKernelVirt + (PageRecursiveUser << (67 - PageSizeOffset - GranuleSize));
*/
		constexpr uintptr_t PageTableAddr(unsigned int level, unsigned long rec)
		{
			uintptr_t base = ((level == 3) ? MinKernelVirt : PageTableAddr(level + 1, rec));
			return base + (rec << (64 - PageSizeOffset - (GranuleSize - 3) * (4 - level)));
		}

		/** Page table at a fixed level in the paging hierarchy */
		template<unsigned int level> class alignas(1ULL << GranuleSize) PageTableLevel
		{
		private:
			/** Number of entries. */
			static const unsigned long size = 1ULL << (GranuleSize - 3);

			/** Array of page table entries, default constructed. */
			PageTableEntry entry[size] = {PageTableEntry{}};

		public:
			/** Reference to i'th entry in the table. */
			inline PageTableEntry& Entry(unsigned int i);

			/** Check whether table is completely empty. */
			bool IsEmpty(void);

			/** Return i if this is the i'th table at this level. */
			inline unsigned long Index(void);

			/** Reference to the i'th table at this level. */
			static inline PageTableLevel<level>& TableKernel(unsigned long i);
			static inline PageTableLevel<level>& TableUser(unsigned long i);

			/** Page table which contains the page table entry pointing to this page table. */
			inline PageTableLevel<level - 1>& Parent(void);

			/** Page table entry which points to this page table. */
			inline PageTableEntry& Pointer(void);

			/** Check whether the i'th table at this level exists. */
			static bool ExistsKernel(unsigned long i);
			static bool ExistsUser(unsigned long i);

			/** Create new page table at this level. */
			static PageTableLevel<level>& CreateKernel(unsigned long i, Memory::MemType);
			static PageTableLevel<level>& CreateUser(unsigned long i, Memory::MemType);

			/** Destroy a page table. */
			void Destroy(void);
		} PACKED;

		template<unsigned int level> inline PageTableEntry& PageTableLevel<level>::Entry(unsigned int i)
		{
			return entry[i];
		}

		template<unsigned int level> bool PageTableLevel<level>::IsEmpty(void)
		{
			for(unsigned int i = 0; i < size; i++)
			{
				if(!entry[i].IsClear())
					return false;
			}

			return true;
		}

		template<unsigned int level> inline unsigned long PageTableLevel<level>::Index(void)
		{
			static_assert(level < 4, "Table level exceeds number of paging levels.");
			static_assert(level >= InitialLookupLevel, "Table level below initial lookup level.");

			return (reinterpret_cast<uintptr_t>(this) - (reinterpret_cast<uintptr_t>(this) & -(1ULL << (GranuleSize + (level - InitialLookupLevel) * (GranuleSize - 3))))) >> GranuleSize;
		}

		template<unsigned int level> inline PageTableLevel<level>& PageTableLevel<level>::TableKernel(unsigned long i)
		{
			static_assert(level < 4, "Table level exceeds number of paging levels.");
			static_assert(level >= InitialLookupLevel, "Table level below initial lookup level.");

			return reinterpret_cast<PageTableLevel<level>*>(PageTableAddr(level, PageRecursiveKernel))[i];
		}

		template<unsigned int level> inline PageTableLevel<level>& PageTableLevel<level>::TableUser(unsigned long i)
		{
			static_assert(level < 4, "Table level exceeds number of paging levels.");
			static_assert(level >= InitialLookupLevel, "Table level below initial lookup level.");

			return reinterpret_cast<PageTableLevel<level>*>(PageTableAddr(level, PageRecursiveUser))[i];
		}

		template<unsigned int level> inline PageTableLevel<level - 1>& PageTableLevel<level>::Parent(void)
		{
			uintptr_t tabaddr = reinterpret_cast<uintptr_t>(this);
			uintptr_t shifted = tabaddr >> GranuleSize;
			uintptr_t mask = (1ULL << (2 * GranuleSize - 3)) - 1;

			return *reinterpret_cast<PageTableLevel<level - 1>*>((shifted & mask) | (tabaddr & ~mask));
		}

		template<unsigned int level> inline PageTableEntry& PageTableLevel<level>::Pointer(void)
		{
			return Parent().Entry(Index() & ((1 << (GranuleSize - 3)) - 1));
		}

		template<unsigned int level> bool PageTableLevel<level>::ExistsKernel(unsigned long i)
		{
			static_assert(level < 4, "Table level exceeds number of paging levels.");
			static_assert(level >= InitialLookupLevel, "Table level below initial lookup level.");

			if(!PageTableLevel<level - 1>::ExistsKernel(i >> (GranuleSize - 3)))
				return false;

			return TableKernel(i).Pointer().IsPresent();
		}

		template<unsigned int level> bool PageTableLevel<level>::ExistsUser(unsigned long i)
		{
			static_assert(level < 4, "Table level exceeds number of paging levels.");
			static_assert(level >= InitialLookupLevel, "Table level below initial lookup level.");

			if(!PageTableLevel<level - 1>::ExistsUser(i >> (GranuleSize - 3)))
				return false;

			return TableUser(i).Pointer().IsPresent();
		}

		template<> inline bool PageTableLevel<InitialLookupLevel>::ExistsKernel(unsigned long i __attribute__((unused)))
		{
			return true;
		}

		template<> inline bool PageTableLevel<InitialLookupLevel>::ExistsUser(unsigned long i __attribute__((unused)))
		{
			return true;
		}

		inline PageTableLevel<InitialLookupLevel>& PageTableTopKernel(void)
		{
			return PageTableLevel<InitialLookupLevel>::TableKernel(0);
		}

		inline PageTableLevel<InitialLookupLevel>& PageTableTopUser(void)
		{
			return PageTableLevel<InitialLookupLevel>::TableUser(0);
		}
	}
}

#endif
