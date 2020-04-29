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
	void updateInputs(boost::multi_array<uint8_t,4>* inputPtr);
	void updateOutputs(boost::multi_array<uint8_t,4>* outputPtr);

	virtual void update(std::mt19937* rng=nullptr) = 0; // might use a thread-specific RNG
	virtual std::shared_ptr<AbstractTextureGate> makeCopy(unsigned copyID) const = 0;
	virtual std::string description() const = 0;
	virtual std::string gateType() const = 0;
	virtual void mutateInternalStructure() = 0; // should use the process-wide RNG
	virtual nlohmann::json serialize() const;
	virtual void deserialize(const nlohmann::json&);
};

/***** Auxiliary functions *****/


/***** Definitions *****/

inline std::string textureAddressRepresentation(const std::tuple<size_t,size_t,size_t,size_t>& addr) {
	std::stringstream ss;
	ss << '(' << std::get<0>(addr) << ','
	   << std::get<1>(addr) << ','
	   << std::get<2>(addr) << ','
	   << std::get<3>(addr) << ')';
	return ss.str();
}

inline AbstractTextureGate::AbstractTextureGate(unsigned newID,
                                         std::vector<std::tuple<size_t,size_t,size_t,size_t>> newInputsIndices,
                                         std::vector<std::tuple<size_t,size_t,size_t,size_t>> newOutputsIndices) :
	ID(newID),
	inputsIndices(newInputsIndices),
	outputsIndices(newOutputsIndices),
	inputs(newInputsIndices.size(), nullptr),
	outputs(newOutputsIndices.size(), nullptr) {}

inline void AbstractTextureGate::reset() {
	std::fill(inputs.begin(), inputs.end(), nullptr);
	std::fill(outputs.begin(), outputs.end(), nullptr);
}

inline void AbstractTextureGate::updateInputs(boost::multi_array<uint8_t,4>* inputPtr) {
	for(size_t i=0; i<inputsIndices.size(); i++) {
		size_t x, y, t, c;
		std::tie(x, y, t, c) = inputsIndices[i];
		inputs[i] = &(*inputPtr)[x][y][t][c];
	}
}

inline void AbstractTextureGate::updateOutputs(boost::multi_array<uint8_t,4>* outputPtr) {
	for(size_t o=0; o<outputsIndices.size(); o++) {
		size_t x, y, t, c;
		std::tie(x, y, t, c) = outputsIndices[o];
		outputs[o] = &(*outputPtr)[x][y][t][c];
	}
}

inline nlohmann::json AbstractTextureGate::serialize() const {
	nlohmann::json out = nlohmann::json::object();
	out["id"] = ID;
	out["type"] = this->gateType();

	auto indices2json = [](const std::tuple<size_t,size_t,size_t,size_t>& idxs) {
		return nlohmann::json::array({std::get<0>(idxs), std::get<1>(idxs), std::get<2>(idxs), std::get<3>(idxs)});
	};

	out["inputsIndices"] = nlohmann::json::array();
	for(const auto& inputIdxs : inputsIndices)
		out["inputsIndices"].push_back(indices2json(inputIdxs));
	out["outputsIndices"] = nlohmann::json::array();
	for(const auto& outputIdxs : outputsIndices)
		out["outputsIndices"].push_back(indices2json(outputIdxs));

	return out;
}

inline void AbstractTextureGate::deserialize(const nlohmann::json& in) {
	ID = in["id"];

	auto json2indices = [](const nlohmann::json& idxsjson) {
		return std::tuple<size_t,size_t,size_t,size_t>(idxsjson[0], idxsjson[1], idxsjson[2], idxsjson[3]);
	};

	inputsIndices.clear();
	for(const auto& iidj : in["inputsIndices"])
		inputsIndices.push_back(json2indices(iidj));
	outputsIndices.clear();
	for(const auto& oidj : in["outputsIndices"])
		outputsIndices.push_back(json2indices(oidj));

	inputs.resize(inputsIndices.size());
	std::fill(inputs.begin(), inputs.end(), nullptr);
	outputs.resize(outputsIndices.size());
	std::fill(outputs.begin(), outputs.end(), nullptr);
}
