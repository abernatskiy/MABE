#pragma once

#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <memory>
#include <type_traits>
#include <unordered_map> // decoder in LabeledHNNG::hexStringPatternToSubpatternVector

#include "hnng.h" // defines HammingNearestNeighborsGenerator and subpattern_t

typedef unsigned ValueType;

class LabeledHNNG {

// Interface between the proper Hamming nearest neighbors generator and the joint counts used in MABE

public:
	LabeledHNNG(unsigned patternLength, unsigned subpatternLength);
	void index(const std::map<std::pair<std::string,std::string>,ValueType>& rawDatabase);
	std::vector<std::tuple<std::string,std::string,ValueType,size_t>> getSomeNeighbors(std::string pattern, size_t minNumNeighbors);
	void print();
	void printPerformanceStats();
	std::pair<long unsigned, long unsigned> getPerformanceStats() { return hnng.stats(); };

private:
	HammingNearestNeighborsGenerator hnng;
	std::shared_ptr<std::vector<std::vector<subpattern_t>>> patternsdb;
	std::vector<std::string> patterns;
	std::vector<std::string> labels;
	std::vector<ValueType> values;

	const unsigned patternLength;
	const unsigned subpatternsPerPattern;
	const unsigned hexCharsPerSubpattern;

	std::vector<subpattern_t> hexStringPatternToSubpatternVector(const std::string& label);
};

inline std::vector<subpattern_t> LabeledHNNG::hexStringPatternToSubpatternVector(const std::string& hexstr) {
	static_assert(std::is_same<subpattern_t,uint32_t>::value, "LabeledHNNG::hexStringPatternToSubpatternVector requires some tweaking if subpattern_t is not uint32_t");
	const std::unordered_map<char,subpattern_t> decoder {{'0',0},{'1',1},{'2',2},{'3',3},{'4',4},{'5',5},{'6',6},{'7',7},{'8',8},{'9',9},{'a',10},{'b',11},{'c',12},{'d',13},{'e',14},{'f',15}};
	// replace with a static 256-valued array if lookup turns out to be too slow

	const char* curpos = hexstr.data();
	const char* stringEnds = curpos + hexstr.size();
	std::vector<subpattern_t> subpatterns;
	for(size_t spi=0; spi<subpatternsPerPattern; spi++) { // subpattern index
		const char* subpatternEnds = curpos + hexCharsPerSubpattern;
		subpattern_t curSubpattern = 0;
		for(; curpos<subpatternEnds; curpos++) {
			curSubpattern <<= 4;
			if(curpos<stringEnds)
				curSubpattern |= decoder.at(*curpos);
		}
		subpatterns.push_back(curSubpattern);
	}
	return subpatterns;
}
