#include <vector>

static_assert(sizeof(void*)==sizeof(uint64_t));

namespace doublePointer {

	void toDoubles(void* ptr, std::vector<double>::iterator outputIterator) {
		// will write the pointer data to *outputIterator and *(outputIterator+1)
		uint64_t iptr = reinterpret_cast<uint64_t>(ptr);
		*outputIterator = static_cast<double>(iptr & 0x00000000ffffffff);
		*(outputIterator+1) = static_cast<double>(iptr >> 32);
	};

	void* fromDoubles(std::vector<double>::iterator inputIterator) {
		// will read the pointer data from *outputIterator and *(outputIterator+1)
		uint64_t iptr = ( static_cast<uint64_t>(*(inputIterator+1)) << 32 ) | ( static_cast<uint64_t>(*inputIterator) );
		return reinterpret_cast<void*>(iptr);
	};

	template<typename StoredDataType>
	void toDoubles(StoredDataType* ptr, std::vector<double>::iterator outputIterator) {
		toDoubles(reinterpret_cast<void*>(ptr), outputIterator);
	};

	template<typename StoredDataType>
	StoredDataType* fromDoubles(std::vector<double>::iterator inputIterator) {
		return reinterpret_cast<StoredDataType*>(fromDoubles(inputIterator));
	};

} // namespace doublePointer
