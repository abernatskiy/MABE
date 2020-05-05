#pragma once

#include <memory>
#include <vector>
#include <stdexcept>
#include <random>
#include <string>
#include <algorithm>
#include <sstream>
#include "../../../Utilities/nlohmann/json.hpp"
#include "../../../Utilities/texture.h"

class AbstractTextureGate {
protected:
	std::vector<uint8_t*> inputs;
	std::vector<uint8_t*> outputs;

public:
	// look: this class has public data!
	unsigned ID;
	std::vector<TextureIndex> inputsIndices;
	std::vector<TextureIndex> outputsIndices;

	// constructor overrides should call this one and use process-wide RNG to randomize the gate structure
	AbstractTextureGate() = default;
	AbstractTextureGate(unsigned ID,
	                    std::vector<TextureIndex> inputsIndices,
	                    std::vector<TextureIndex> outputsIndices);
	virtual ~AbstractTextureGate() = default;

	void updateInputs(Texture* inputPtr);
	void updateOutputs(Texture* outputPtr);

	void reset() {}; // make this virtual if stateful gates are needed

	virtual void update(std::mt19937* rng=nullptr) = 0; // might use a thread-specific RNG
	virtual std::shared_ptr<AbstractTextureGate> makeCopy(unsigned copyID) const = 0;
	virtual std::string description() const = 0;
	virtual std::string gateType() const = 0;
	virtual void mutateInternalStructure() = 0; // should use the process-wide RNG

	virtual nlohmann::json serialize() const;
	virtual void deserialize(const nlohmann::json&);
};

inline AbstractTextureGate::AbstractTextureGate(unsigned newID,
                                         std::vector<TextureIndex> newInputsIndices,
                                         std::vector<TextureIndex> newOutputsIndices) :
	ID(newID),
	inputsIndices(newInputsIndices),
	outputsIndices(newOutputsIndices),
	inputs(newInputsIndices.size(), nullptr),
	outputs(newOutputsIndices.size(), nullptr) {}

inline void AbstractTextureGate::updateInputs(Texture* inputPtr) {
	for(size_t i=0; i<inputsIndices.size(); i++)
		inputs[i] = &((*inputPtr)(inputsIndices[i])); // operator() of boost::multi_array
}

inline void AbstractTextureGate::updateOutputs(Texture* outputPtr) {
	for(size_t o=0; o<outputsIndices.size(); o++)
		outputs[o] = &((*outputPtr)(outputsIndices[o]));
}

inline nlohmann::json AbstractTextureGate::serialize() const {
	nlohmann::json out = nlohmann::json::object();
	out["id"] = ID;
	out["type"] = this->gateType();

	auto indices2json = [](TextureIndex idx) {
		return nlohmann::json::array({idx[0], idx[1], idx[2], idx[3]});
	};

	out["inputsIndices"] = nlohmann::json::array();
	for(auto inputIdxs : inputsIndices)
		out["inputsIndices"].push_back(indices2json(inputIdxs));
	out["outputsIndices"] = nlohmann::json::array();
	for(auto outputIdxs : outputsIndices)
		out["outputsIndices"].push_back(indices2json(outputIdxs));

	return out;
}

inline void AbstractTextureGate::deserialize(const nlohmann::json& in) {
	ID = in["id"];

	auto json2indices = [](const nlohmann::json& idxsjson) {
		return TextureIndex({{idxsjson[0], idxsjson[1], idxsjson[2], idxsjson[3]}});
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
