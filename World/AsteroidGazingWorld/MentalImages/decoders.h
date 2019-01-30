#include "SphericalHarmonicsBasedAsteroidImageMentalImage.h"

#include <algorithm>

// Decoders based on "one-hot" encoding
// Quotes are due to the fact that only the leftmost one counts, if there are any more ones to the right of it the decoder does not care
// This makes all states valid

inline unsigned decodeOHUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	return std::distance(begin, std::max_element(begin, end));
}

inline int decodeOHSInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	int offsetArgmax = std::distance(begin+1, std::max_element(begin+1, end));
	return (*begin)<0.5 ? offsetArgmax : -1*offsetArgmax;
}

inline double decodeOHDouble(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	return static_cast<double>(std::distance(begin, std::max_element(begin, end))) / static_cast<double>(std::distance(begin, end));
}

// Decoders based on the standard positional encoding

inline unsigned decodeSPUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	unsigned retval = 0;
	while(begin != end) {
		retval <<= 1;
		retval += static_cast<unsigned>(*begin);
		begin++;
	}
	return retval;
}
