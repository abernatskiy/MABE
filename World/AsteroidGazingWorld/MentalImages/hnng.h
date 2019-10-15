#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <set>

typedef uint32_t subpattern_t;

class HammingNearestNeighborsGenerator {

// A class that searches for nearest neighbors in Hamming space in sublinear time.
// Implements the Multi-Index Hashing https://ieeexplore.ieee.org/abstract/document/6248043

private:
	const unsigned subpatternsPerPattern;
	std::shared_ptr<std::vector<std::vector<subpattern_t>>> database;
	std::vector<std::unordered_map<subpattern_t,std::vector<size_t>>> multiIndex;

public:
	HammingNearestNeighborsGenerator(unsigned patternLength, unsigned subpatternLength);
	void index(std::shared_ptr<std::vector<std::vector<subpattern_t>>> newDatabase);
	void printIndex();
	std::set<size_t> getIndicesOfNeighborsWithinSphere(const std::vector<subpattern_t>& centralPattern, unsigned radius);
};
