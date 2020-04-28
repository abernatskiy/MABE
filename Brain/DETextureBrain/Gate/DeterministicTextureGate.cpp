#include "DeterministicTextureGate.h"

#include <climits>
#include <stdexcept>
#include <sstream>

#include "../../../Utilities/Random.h"

using namespace std;

/***** Auxiliary functions *****/

string fixedWidthBinaryRepresentation(size_t number, size_t width) {
	stringstream ss;
//	for(size_t i=width; i>0; i--)
//		ss << ((number >> (i-1)) & 1);
	for(size_t i=0; i<width; i++)
		ss << ((number >> i) & 1);
	return ss.str();
}

/***** DetermininisticTextureGate definitions *****/

DeterministicTextureGate::DeterministicTextureGate(unsigned newID,
                                                   std::vector<std::tuple<size_t,size_t,size_t,size_t>> newInputsIndices,
                                                   std::vector<std::tuple<size_t,size_t,size_t,size_t>> newOutputsIndices) :
	AbstractTextureGate(newID, newInputsIndices, newOutputsIndices) {

	// inputs and outputs are initialized to the correct size in the base class constructor,
	// so I can freely use their .size()s to validate the gate and make an appropriate table
	if(inputs.size()>4) throw length_error("DeterministicTextureGate was given more than four inputs, "
	                                       "which is supported but should be avoided. You'll need to "
	                                       "comment out this throw and recompile to proceed.");
	if(inputs.size()>=CHAR_BITS*sizeof(size_t)) throw length_error("DeterministicTextureGate was given far too many inputs");

	size_t tableSize = 1;
	tableSize <<= inputs.size();

	for(size_t ipat=0; ipat<tableSize; ipat++) {
		table.push_back({});
		for(size_t o=0; o<outputs.size(); o++)
			table[ipat].push_back(getInt(1)); // the table is generated randomly
	}
}

shared_ptr<AbstractTextureGate> DeterministicTextureGate::makeCopy(unsigned copyID) const {
	auto newGate = make_shared<DeterministicTextureGate>(copyID, inputsIndices, outputsIndices);
	newGate->table = table;
	return newGate;
}

string DeterministicTextureGate::description() const {
	stringstream ss;
	ss << "DETERMINISTIC TEXTURE GATE:" << endl
	   << "Input texture addresses:";
	for(const auto& ii : inputsIndices)
		ss << ' ' << textureAddressRepresentation(ii);
	ss << endl << "Output texture addresses:";
	for(const auto& oi : outputsIndices)
		ss << ' ' << textureAddressRepresentation(oi);
	ss << endl << "Current input pointers:";
	for(const auto& i : inputs)
		ss << ' ' << i;
	ss << endl << "Current output pointers:";
	for(const auto& o : outputs)
		ss << ' ' << o;
	ss << endl << "Table:" << endl;
	for(size_t ipat=0; ipat<table.size(); ipat++) {
		ss << fixedWidthBinaryRepresentation(ipat, inputs.size()) << ":";
		for(size_t o=0; o<outputs.size(); o++)
			ss << ' ' << table[ipat][o];
		ss << endl;
	}
	return ss.str();
}

void DeterministicTextureGate::mutateInternalStructure() const {
	int inPatIdx = Random::getIndex(table.size());

	// True point mutation - legacy, check before enabling!
	// int outIdx = Random::getIndex(table[inPatIdx].size());
	// table[inPatIdx][outIdx] = table[inPatIdx][outIdx] ? 0 : 1;

	// Randomize output pattern mutation
	// It is supposed to make info landscape convex with an optimum at 1 and its empirical performance seems better, but that is not conclusive
	size_t outPatternWidth = table[inPatIdx].size();
	vector<int> newOutPattern(outPatternWidth);
	while(1) {
		for(size_t i=0; i<outPatternWidth; i++)
			newOutPattern[i] = Random::getInt(0, 1);
		if(table[inPatIdx]!=newOutPattern) {
			table[inPatIdx] = newOutPattern;
			break;
		}
	}
}

void DeterministicTextureGate::update(std::mt19937* rng=nullptr) const {
	size_t inPat = 0;
	for(size_t i=0; i<inputs.size(); i++) {
		inPat <<= 1;
		inPat &= (*inputs[i]); // add "!= 0" to allow inputs outside of {0,1}
	}

	for(size_t o=0; o<outputs.size(); o++)
		*outputs[o] = table[inPat][o];
}

nlohmann::json DeterministicTextureGate::serialize() const {
	nlohmann::json out = AbstractTextureGate::serialize();
	out["table"] = nlohmann::json::array();
	for(const auto& row : table) {
		out["table"].push_back(nlohmann::json::array());
		for(const auto& columnVal : row)
			out["table"].back().push_back(static_cast<unsigned>(columnVal));
	}
}

void DeterministicTextureGate::deserialize(const nlohmann::json& in) {
	AbstractTextureGate::deserialize(in);
	table.clear();
	for(const auto& row : in["table"]) {
		table.push_back({});
		for(const auto& val : row)
			table.back().push_back(static_cast<uint8_t>(val));
	}
}
