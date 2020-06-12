#include "DeterministicTextureGate.h"

#include <climits>
#include <stdexcept>
#include <sstream>

#include "../../../Utilities/Random.h"
#include "../../../Utilities/Utilities.h" // for fixedWidthBinaryRepresentation(size_t number, size_t width)

using namespace std;

DeterministicTextureGate::DeterministicTextureGate(unsigned newID,
                                                   vector<TextureIndex> newInputsFilterIndices,
                                                   vector<TextureIndex> newOutputsFilterIndices,
                                                   long* erasCounterPtr) :
	AbstractTextureGate(newID, newInputsFilterIndices, newOutputsFilterIndices, erasCounterPtr) {

	// inputs and outputs are initialized to the correct size in the base class constructor,
	// so I can freely use their .size()s to validate the gate and make an appropriate table
	if(inputs.size()>4) throw length_error("DeterministicTextureGate was given more than four inputs, "
	                                       "which is supported but should be avoided. You'll need to "
	                                       "comment out this throw and recompile to proceed.");
	if(inputs.size()>=CHAR_BIT*sizeof(size_t)) throw length_error("DeterministicTextureGate was given far too many inputs");

	size_t tableSize = 1;
	tableSize <<= inputs.size();

	for(size_t ipat=0; ipat<tableSize; ipat++) {
		table.push_back({});
		for(size_t o=0; o<outputs.size(); o++)
			table.back().push_back(Random::getInt(1)); // the table is generated randomly
	}
}

shared_ptr<AbstractTextureGate> DeterministicTextureGate::makeCopy(unsigned copyID) const {
	auto newGate = make_shared<DeterministicTextureGate>(copyID, inputsFilterIndices, outputsFilterIndices, erasureCounterPtr);
	newGate->setInputsShift(inputsShift);
	newGate->setOutputsShift(outputsShift);
	newGate->table = table;
	return newGate;
}

string DeterministicTextureGate::description() const {
	stringstream ss;
	ss << "DETERMINISTIC TEXTURE GATE " << ID << endl
	   << "Input shift is " << readableRepr(inputsShift) << ". Input filter indices:";
	for(auto ii : inputsFilterIndices)
		ss << ' ' << readableRepr(ii);
	ss << endl << "Output shift is " << readableRepr(outputsShift) << ". Output filter indices:";
	for(auto oi : outputsFilterIndices)
		ss << ' ' << readableRepr(oi);
	ss << endl << "Current input pointers:";
	for(auto i : inputs)
		ss << ' ' << static_cast<void*>(i);
	ss << endl << "Current output pointers:";
	for(auto o : outputs)
		ss << ' ' << static_cast<void*>(o);
	ss << endl << "Table:" << endl;
	for(size_t ipat=0; ipat<table.size(); ipat++) {
		ss << fixedWidthBinaryRepresentation(ipat, inputs.size()) << ":";
		for(size_t o=0; o<outputs.size(); o++)
			ss << ' ' << static_cast<unsigned>(table[ipat][o]);
		ss << endl;
	}
	return ss.str();
}

void DeterministicTextureGate::mutateInternalStructure() {
	int inPatIdx = Random::getIndex(table.size());

	// True point mutation - legacy, check before enabling!
	// int outIdx = Random::getIndex(table[inPatIdx].size());
	// table[inPatIdx][outIdx] = table[inPatIdx][outIdx] ? 0 : 1;

	// Randomize output pattern mutation
	// It is supposed to make info landscape convex with an optimum at 1 and its empirical performance seems better, but that is not conclusive
	size_t outPatternWidth = table[inPatIdx].size();
	vector<uint8_t> newOutPattern(outPatternWidth);
	while(1) {
		for(size_t i=0; i<outPatternWidth; i++)
			newOutPattern[i] = Random::getInt(0, 1);
		if(table[inPatIdx]!=newOutPattern) {
			table[inPatIdx] = newOutPattern;
			break;
		}
	}
}

void DeterministicTextureGate::update(std::mt19937* rng) {
	size_t inPat = 0;
	for(size_t i=0; i<inputs.size(); i++) {
		inPat <<= 1;
		inPat |= (*inputs[i]); // add "!= 0" to allow inputs outside of {0,1}
	}

	for(size_t o=0; o<outputs.size(); o++) {
		if(*outputs[o])
			(*erasureCounterPtr)++;
		*outputs[o] |= table[inPat][o];
	}
}

nlohmann::json DeterministicTextureGate::serialize() const {
	nlohmann::json out = AbstractTextureGate::serialize();

	out["table"] = nlohmann::json::array();
	for(const auto& row : table) {
		auto jsonRow = nlohmann::json::array();
		for(const auto& columnVal : row)
			jsonRow.push_back(static_cast<unsigned>(columnVal));
		out["table"].push_back(jsonRow);
	}

	return out;
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
