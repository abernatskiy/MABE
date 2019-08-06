#include <iostream>
#include <cstdlib>

#include "hngen.h"

using namespace std;

/***** Auxiliary functions *****/

void recGetFlips(const string& basepat,
                 const vector<unsigned>& cap,
                 const vector<unsigned>& remCap,
                 unsigned flips,
                 unsigned pos,
                 string prefix,
                 vector<string>& output) {

	// DEBUG
//	if(pos!=prefix.size()) {
//		cerr << "Pos " << pos << ", prefix " << prefix << " (size " << prefix.size() << ")" << endl;
//		exit(EXIT_FAILURE);
//	}
//	if(remCap[pos]<flips) {
//		cerr << "Pos " << pos << ", remaining capacity here is " << remCap[pos] << ", " << flips << " bits requested" << endl;
//		exit(EXIT_FAILURE);
//	}

//	cout << "Called basepat=" << basepat << " flips=" << flips << " pos=" << pos << " prefix=" << prefix << endl;

	if(flips==0) {
		string newEntry = basepat;
		for(unsigned i=0; i<pos; i++)
			newEntry[i] = prefix[i];
//		cout << "adding entry " << newEntry << endl;
		output.push_back(newEntry);
	}
	else {
		unsigned curLocalFlips = flips>cap[pos] ? cap[pos] : flips;
		while(true) {
			for(const char cc : getNeighborhood(cap[pos], curLocalFlips, basepat[pos])) {
				string newPrefix = prefix;
				newPrefix.push_back(cc);
				recGetFlips(basepat, cap, remCap, flips-curLocalFlips, pos+1, newPrefix, output);
			}
			curLocalFlips--;

			if(flips-curLocalFlips>remCap[pos+1])
				break;

			if(curLocalFlips==0) {
				string newPrefix = prefix;
				newPrefix.push_back(basepat[pos]);
				recGetFlips(basepat, cap, remCap, flips, pos+1, newPrefix, output);
				break;
			}
		}
	}
}

/***** HammingNeighborhoodGenerator public definitions *****/

HammingNeighborhoodGenerator::HammingNeighborhoodGenerator(unsigned numFlippableBits) {
	while(numFlippableBits>4) {
		fieldCapacities.push_back(4);
		numFlippableBits -= 4;
	}
	fieldCapacities.push_back(numFlippableBits);

	remainingCapacity.resize(fieldCapacities.size()+1, 0);
	for(int i=fieldCapacities.size()-1; i>=0; i--)
		remainingCapacity[i] = remainingCapacity[i+1] + fieldCapacities[i];
}

vector<string> HammingNeighborhoodGenerator::getNeighbors(string pattern, unsigned distance) {
	if(distance>remainingCapacity[0]) {
		cerr << "HammingNeighborhoodGenerator::getNeighbors: cannot produce a neighborhood at the distance of " << distance
		     << " as max distance in this binary space is " << remainingCapacity[0] << endl;
		exit(EXIT_FAILURE);
	}

	// TODO: add a LRU cache here
	vector<string> neighs;
	recGetFlips(pattern, fieldCapacities, remainingCapacity, distance, 0, "", neighs);
	return neighs;
}
