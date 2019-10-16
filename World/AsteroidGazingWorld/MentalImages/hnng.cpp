#include "hnng.h"

#include <iostream>
#include <utility>
#include <cstdlib>
#include <type_traits>

using namespace std;

HammingNearestNeighborsGenerator::HammingNearestNeighborsGenerator(unsigned patternLength, unsigned subpatternLength) :
	subpatternsPerPattern(1 + ((patternLength-1) / subpatternLength)),
	candidates(0),
	hits(0) {

	if(subpatternLength>8*sizeof(subpattern_t)) {
		cerr << "HammingNearestNeighborsGenerator: subpatterns of requested length (" << subpatternLength
		     << ") cannot fit into the storage used by the current implementation (" << 8*sizeof(subpattern_t)
		     << "-bit integers), exiting" << endl;
		exit(EXIT_FAILURE);
	}

	for(size_t spi=0; spi<subpatternsPerPattern; spi++)
		multiIndex.emplace_back();
}

void HammingNearestNeighborsGenerator::index(shared_ptr<vector<vector<subpattern_t>>> newDatabase) {
	database = newDatabase;
	for(size_t pi=0; pi<database->size(); pi++) {
		for(size_t spi=0; spi<subpatternsPerPattern; spi++) {
			auto itCurIndex = multiIndex.begin() + spi;
			subpattern_t newSubpattern = database->at(pi).at(spi);
			auto itSubpatternRecord = itCurIndex->find(newSubpattern);
			if(itSubpatternRecord == itCurIndex->end())
				itCurIndex->emplace(make_pair(newSubpattern, vector<size_t>({pi})));
			else
				itSubpatternRecord->second.push_back(pi);
		}
	}
	candidates = 0; hits = 0;
}

void HammingNearestNeighborsGenerator::printIndex() {
	cout << "Index of a database at " << database << ":" << endl;
	for(size_t spi=0; spi<subpatternsPerPattern; spi++) {
		cout << "subpattern " << spi << "|";
		for(const auto& spipair : multiIndex[spi]) {
			cout << " " << hex << spipair.first << dec << ":";
			size_t recnum = 0;
			for(const auto& idxrec : spipair.second)
				cout << (recnum++ ? "," : "") << idxrec;
		}
		cout << endl;
	}
}

set<pair<size_t,size_t>> HammingNearestNeighborsGenerator::getIndicesAndDistancesOfNeighborsWithinSphere(const vector<subpattern_t>& centralPattern, unsigned radius) {
	static_assert(is_same<subpattern_t,uint32_t>::value, "HammingNearestNeighborsGenerator::getIndicesOfNeighborsWithinSphere requires rework whenever subpattern type is changed");
	set<pair<size_t,size_t>> neighbors;
	const subpattern_t subpatternRadius = radius / subpatternsPerPattern;
	for(size_t spi=0; spi<subpatternsPerPattern; spi++) { // subpattern index
		cout << "Subpattern " << spi << ":";
		for(const auto& curSPRecord : multiIndex[spi]) {
			cout << " " << hex << curSPRecord.first << dec << ":d";

			subpattern_t buffer = curSPRecord.first;
			buffer ^= centralPattern.at(spi);

			buffer = (buffer & 0x55555555) + ((buffer>>1) & 0x55555555);
			buffer = (buffer & 0x33333333) + ((buffer>>2) & 0x33333333);
			buffer = (buffer & 0x0f0f0f0f) + ((buffer>>4) & 0x0f0f0f0f);
			buffer = (buffer & 0x00ff00ff) + ((buffer>>8) & 0x00ff00ff);
			buffer = (buffer & 0x0000ffff) + ((buffer>>16) & 0x0000ffff);

			cout << buffer;

			if(buffer<=subpatternRadius) {
				cout << ",added";
				for(const auto& nci : curSPRecord.second) { // neighbor candidate index
					candidates++;

					unsigned totalDist = buffer;
					for(size_t spj=0; spj<subpatternsPerPattern; spj++) {
						if(spj==spi) continue;

						subpattern_t buffer1 = database->at(nci).at(spj);
						buffer1 ^= centralPattern.at(spj);

						buffer1 = (buffer1 & 0x55555555) + ((buffer1>>1) & 0x55555555);
						buffer1 = (buffer1 & 0x33333333) + ((buffer1>>2) & 0x33333333);
						buffer1 = (buffer1 & 0x0f0f0f0f) + ((buffer1>>4) & 0x0f0f0f0f);
						buffer1 = (buffer1 & 0x00ff00ff) + ((buffer1>>8) & 0x00ff00ff);
						buffer1 = (buffer1 & 0x0000ffff) + ((buffer1>>16) & 0x0000ffff);

						totalDist += buffer1;
					}

					if(totalDist<=radius) {
						hits++;
						neighbors.insert(make_pair(nci, totalDist));
						cout << nci << ",";
					}
				}
			}
		}
		cout << endl;
	}
	return neighbors;
}
