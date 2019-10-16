#include "labeledHNNG.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

LabeledHNNG::LabeledHNNG(unsigned patternLength, unsigned subpatternLength) :
	hnng(patternLength, subpatternLength),
	patternsdb(make_shared<vector<vector<subpattern_t>>>()),
	subpatternsPerPattern(1 + ((patternLength-1) / subpatternLength)),
	hexCharsPerSubpattern(subpatternLength/4) {

	if(subpatternLength>8*sizeof(subpattern_t)) {
		cerr << "LabeledHNNG: subpatterns of requested length (" << subpatternLength
		     << ") cannot fit into the storage used by the current implementation (" << 8*sizeof(subpattern_t)
		     << "-bit integers), exiting" << endl;
		exit(EXIT_FAILURE);
	}

	if(subpatternLength % 4 != 0) {
		cerr << "LabeledHNNG: subpattern lengths that are not multiples of 4 are currently unsupported. Requested subpattern length: " << subpatternLength << endl;
		exit(EXIT_FAILURE);
	}
}

void LabeledHNNG::index(const map<pair<string,string>,ValueType>& rawDatabase) {
	patternsdb->clear();
	patterns.clear();
	labels.clear();
	values.clear();
	for(const auto& rawdbrec : rawDatabase) {
		string pattern, label;
		tie(label, pattern) = rawdbrec.first;
		patternsdb->push_back(hexStringPatternToSubpatternVector(pattern));
		patterns.push_back(pattern);
		labels.push_back(label);
		values.push_back(rawdbrec.second);
	}
	hnng.index(patternsdb);
}

vector<tuple<string,string,ValueType,size_t>> LabeledHNNG::getSomeNeighbors(string rawPattern, size_t minNumNeighbors) {
	vector<subpattern_t> pattern = hexStringPatternToSubpatternVector(rawPattern);
	vector<tuple<string,string,ValueType,size_t>> output;
	set<pair<size_t,size_t>> neighborhood;
	size_t radius = 0;
	while(neighborhood.size() < minNumNeighbors) {
		neighborhood = hnng.getIndicesAndDistancesOfNeighborsWithinSphere(pattern, radius);
		radius++;
	}
//	cout << "Stopping at radius " << radius << ", got " << neighborhood.size() << " neighbors" << endl;
	for(auto nnpair : neighborhood) // nearest neighbor + distance pair
		output.push_back(make_tuple(labels[nnpair.first], patterns[nnpair.first], values[nnpair.first], nnpair.second));
	return output;
}

void LabeledHNNG::print() {
	cout << "Cached database entries (format (label,pattern): value):" << endl;
	for(size_t i=0; i<patterns.size(); i++)
		cout << "(" << labels[i] << "," << patterns[i] << "):" << values[i] << endl;
	cout << "Database of patterns:" << endl;
	cout << hex << setw(hexCharsPerSubpattern) << setfill('0');
	for(const auto& pdbrec : *patternsdb) {
		for(const auto& pat : pdbrec)
			cout << setw(hexCharsPerSubpattern) << setfill('0') << pat << " ";
		cout << endl;
	}
	cout << dec << setw(0) << setfill('\0');
	hnng.printIndex();
}

void LabeledHNNG::printPerformanceStats() {
	long unsigned candidates, hits;
	tie(candidates, hits) = hnng.stats();
	cout << "Out of " << candidates << " evaluated candidates " << hits << " were hits" << endl;
}


