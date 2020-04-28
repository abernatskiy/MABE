#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <random>
#include <string>
#include <algorithm>
#include <sstream>
#include "boost/multi_array.hpp"
#include "../../../Utilities/nlohmann/json.hpp"

/***** Declarations *****/

std::string textureAddressRepresentation(const std::tuple<size_t,size_t,size_t,size_t>&);

class AbstractTextureGate {
protected:
	std::vector<uint8_t*> inputs;
	std::vector<uint8_t*> outputs;

public:
	// look: this class has public data!
	unsigned ID;
	std::vector<std::tuple<size_t,size_t,size_t,size_t>> inputsIndices;
	std::vector<std::tuple<size_t,size_t,size_t,size_t>> outputsIndices;

	// constructor overrides should call this one and use process-wide RNG to randomize the gate structure
	AbstractTextureGate() = default;
	AbstractTextureGate(unsigned ID,
	                    std::vector<std::tuple<size_t,size_t,size_t,size_t>> inputsIndices,
	                    std::vector<std::tuple<size_t,size_t,size_t,size_t>> outputsIndices);
	virtual ~AbstractTextureGate() = default;

	void reset(); // make this virtual if stateful gates are needed
	void updateInputs(const boost::multi_array<uint8_t,4>* inputPtr);
	void updateOutputs(boost::multi_array<uint8_t,4>* outputPtr);

	virtual void update(std::mt19937* rng=nullptr) const = 0; // might use a thread-specific RNG
	virtual std::shared_ptr<AbstractTextureGate> makeCopy(unsigned copyID) const = 0;
	virtual std::string description() const = 0;
	virtual std::string gateType() const = 0;
	virtual void mutateInternalStructure() = 0; // should use the process-wide RNG
	virtual nlohmann::json serialize() const;
	virtual void deserialize(const nlohmann::json&);
};

/***** Auxiliary functions *****/


/***** Definitions *****/

std::string textureAddressRepresentation(const std::tuple<size_t,size_t,size_t,size_t>& addr) {
	std::stringstream ss;
	ss << '(' << std::get<0>(addr) << ','
	   << std::get<1>(addr) << ','
	   << std::get<2>(addr) << ','
	   << std::get<3>(addr) << ')';
	return ss.str();
}

AbstractTextureGate::AbstractTextureGate(unsigned newID,
                                         std::vector<std::tuple<size_t,size_t,size_t,size_t>> newInputsIndices,
                                         std::vector<std::tuple<size_t,size_t,size_t,size_t>> newOutputsIndices) :
	ID(newID),
	inputsIndices(newInputsIndices),
	outputsIndices(newOutputsIndices),
	inputs(newInputsIndices.size(), nullptr),
	outputs(newOutputsIndices.size(), nullptr) {}

void AbstractTextureGate::reset() {
	std::fill(inputs.begin(), inputs.end(), nullptr);
	std::fill(outputs.begin(), outputs.end(), nullptr);
}

void AbstractTextureGate::updateInputs(const boost::multi_array<uint8_t,4>* inputPtr) {
	for(size_t i=0; i<inputsIndices.size(); i++) {
		size_t x, y, t, c;
		std::tie(x, y, t, c) = inputIndices[i];
		inputs[i] = &(*inputPtr)[x][y][t][c];
	}
}

void AbstractTextureGate::updateOutputs(boost::multi_array<uint8_t,4>* outputPtr) {
	for(size_t o=0; o<outputIndices.size(); o++) {
		size_t x, y, t, c;
		std::tie(x, y, t, c) = outputIndices[o];
		outputs[o] = &(*outputPtr)[x][y][t][c];
	}
}

nlohmann::json AbstractTextureGate::serialize() const {
	nlohmann::json out = nlohmann::json::object();
	out["id"] = ID;
	out["type"] = this->gateType();

	auto indices2json = [](const std::tuple<size_t,size_t,size_t,size_t>& idxs) {
		return nlohmann::json::array({std::get<0>(idxs), std::get<1>(idxs), std::get<2>(idxs), std::get<3>(idxs)});
	};

	out["inputIndices"] = nlohmann::json::array();
	for(const auto& inputIdxs : inputIndices)
		out["inputIndices"].push_back(indices2json(inputIdxs));
	out["outputIndices"] = nlohmann::json::array();
	for(const auto& outputIdxs : outputIndices)
		out["outputIndices"].push_back(indices2json(outputIdxs));

	return out;
}

void AbstractTextureGate::deserialize(const nlohmann::json& in) {
	ID = in["id"];

	auto json2indices = [](const nlohmall::json& idxsjson) {
		return std::tuple<size_t,size_t,size_t,size_t>(idxsjson[0], idxsjson[1], idxsjson[2], idxsjson[3]);
	};

	inputIndices.clear();
	for(const auto& iidj : in["inputIndices"])
		inputIndices.push_back(json2indices(iidj));
	outputIndices.clear();
	for(const auto& oidj : in["outputIndices"])
		outputIndices.push_back(json2indices(oidj));

	inputs.resize(inputIndices.size());
	std::fill(inputs.begin(), inputs.end(), nullptr);
	outputs.resize(outputIndices.size());
	std::fill(outputs.begin(), outputs.end(), nullptr);
}
