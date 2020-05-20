#include "ProbabilisticTextureGate.h"

#include <climits>
#include <stdexcept>
#include <sstream>
#include <algorithm>

#include "../../../Utilities/Random.h"
#include "../../../Utilities/Utilities.h" // for fixedWidthBinaryRepresentation(size_t number, size_t width)

using namespace std;

/***** Definitions of public methods of ProbabilisticTextureGate *****/

ProbabilisticTextureGate::ProbabilisticTextureGate(unsigned newID,
                                                   vector<TextureIndex> newInputsFilterIndices,
                                                   vector<TextureIndex> newOutputsFilterIndices) :
	AbstractTextureGate(newID, newInputsFilterIndices, newOutputsFilterIndices),
	numInputPatterns(1),
	numOutputPatterns(1) {

	// inputs and outputs are initialized to the correct size in the base class constructor,
	// so I can freely use their .size()s to validate the gate and make an appropriate valuesTable
	if(inputs.size()>4) throw length_error("ProbabilisticTextureGate was given more than four inputs, "
	                                       "which is supported but should be avoided. You'll need to "
	                                       "comment out this throw and recompile to proceed.");
	if(inputs.size()>=CHAR_BIT*sizeof(size_t)) throw length_error("ProbabilisticTextureGate was given far too many inputs");
	if(outputs.size()>4) throw length_error("ProbabilisticTextureGate was given more than four outputs, "
	                                        "which is supported but should be avoided. You'll need to "
	                                        "comment out this throw and recompile to proceed.");
	if(outputs.size()>=CHAR_BIT*sizeof(size_t)) throw length_error("ProbabilisticTextureGate was given far too many outputs");

	numInputPatterns <<= inputs.size();
	numOutputPatterns <<= outputs.size();

	table.resize(numInputPatterns);
	for(size_t ip=0; ip<numInputPatterns; ip++) {
		for(size_t op=0; op<numOutputPatterns; op++)
			table[ip].push_back(Random::getDouble(1.));
		normalizeOutput(ip);
	}
}

shared_ptr<AbstractTextureGate> ProbabilisticTextureGate::makeCopy(unsigned copyID) const {
	auto newGate = make_shared<ProbabilisticTextureGate>(copyID, inputsFilterIndices, outputsFilterIndices);
	newGate->setInputsShift(inputsShift);
	newGate->setOutputsShift(outputsShift);
	newGate->table = table;
	return newGate;
}

string ProbabilisticTextureGate::description() const {
	stringstream ss;
	ss << "PROBABILISTIC TEXTURE GATE " << ID << endl
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
	for(size_t ipat=0; ipat<numInputPatterns; ipat++) {
		ss << fixedWidthBinaryRepresentation(ipat, inputs.size()) << ":";
		for(size_t opat=0; opat<numOutputPatterns; opat++)
			ss << ' ' << table[ipat][opat];
		ss << endl;
	}
	return ss.str();
}

void ProbabilisticTextureGate::mutateInternalStructure() {
	size_t inPat = Random::getIndex(numInputPatterns);
	size_t outPat = Random::getIndex(numOutputPatterns);
	float curVal = table[inPat][outPat];
	float newVal = Random::getNormal(curVal, curVal<0.5 ? curVal : 1.-curVal);
	newVal = newVal>1. ? 1. : newVal;
	newVal = newVal<0. ? 0. : newVal;
	table[inPat][outPat] = newVal;
	normalizeOutput(inPat);
}

void ProbabilisticTextureGate::update(std::mt19937* rng) {
	size_t inPat = 0;
	for(size_t i=0; i<inputs.size(); i++) {
		inPat <<= 1;
		inPat |= (*inputs[i]); // add "!= 0" to allow inputs outside of {0,1}
	}

	float r = uniform_real_distribution<float>(0., 1.)(*rng);
	float spentProb = 0.;
	size_t outPat;
	for(outPat=0; outPat<numOutputPatterns; outPat++) {
		spentProb += table[inPat][outPat];
		if(spentProb>r)
			break;
	}
	if(outPat==numOutputPatterns) outPat--;

	for(size_t o=0; o<outputs.size(); o++)
		*outputs[o] |= (outPat >> o) && 1;
}

nlohmann::json ProbabilisticTextureGate::serialize() const {
	nlohmann::json out = AbstractTextureGate::serialize();

	out["numInputPatterns"] = numInputPatterns;
	out["numOutputPatterns"] = numOutputPatterns;

	out["table"] = nlohmann::json::array();
	for(const auto& row : table) {
		auto jsonRow = nlohmann::json::array();
		for(const auto& columnVal : row)
			jsonRow.push_back(columnVal);
		out["table"].push_back(jsonRow);
	}

	return out;
}

void ProbabilisticTextureGate::deserialize(const nlohmann::json& in) {
	AbstractTextureGate::deserialize(in);
	numInputPatterns = in["numInputPatterns"];
	numOutputPatterns = in["numOutputPatterns"];
	table.clear();
	for(const auto& row : in["table"]) {
		table.push_back({});
		for(const auto& val : row)
			table.back().push_back(val);
	}
}

/***** Definitions of private methods of ProbabilisticTextureGate *****/

void ProbabilisticTextureGate::normalizeOutput(size_t inputIdx) {
	float norm = accumulate(table[inputIdx].begin(), table[inputIdx].end(), 0.);
	for(auto& prob : table[inputIdx])
		prob /= norm;
}
