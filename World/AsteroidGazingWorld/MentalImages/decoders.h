#include "SphericalHarmonicsBasedAsteroidImageMentalImage.h"

#include <algorithm>

// Position-based decoders

inline unsigned decodeUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	return std::distance(begin, std::max_element(begin, end));
}

inline int decodeSInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	int offsetArgmax = std::distance(begin+1, std::max_element(begin+1, end));
	return (*begin)<0.5 ? offsetArgmax : -1*offsetArgmax;
}

inline double decodeDouble(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	return static_cast<double>(std::distance(begin, std::max_element(begin, end))) / static_cast<double>(std::distance(begin, end));
}
