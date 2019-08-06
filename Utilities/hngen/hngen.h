#pragma once

#include <vector>
#include <map>

#include "constHammingNeighborhoods.h"

class HammingNeighborhoodGenerator {
public:
	HammingNeighborhoodGenerator(unsigned numFlippableBits);
	std::vector<std::string> getNeighbors(std::string pattern, unsigned distance);
	unsigned numFields() { return fieldCapacities.size(); };

private:
	std::vector<unsigned> fieldCapacities;
	std::vector<unsigned> remainingCapacity;
};

inline std::vector<char> getNeighborhood(unsigned nbits, unsigned nflips, char c) {
	using namespace std;
	const vector<vector<map<char,vector<char>>>> allhoods ALLNEIGBORS;
	return nflips==0 ? vector<char> {c} : allhoods[nbits-1][nflips-1].at(c);
}
