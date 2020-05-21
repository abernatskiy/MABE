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
	std::vector<TextureIndex> inputsFilterIndices;
	std::vector<TextureIndex> outputsFilterIndices;
	TextureIndex inputsShift;
	TextureIndex outputsShift;

	// constructor overrides should call this one and use process-wide RNG to randomize the gate structure
	AbstractTextureGate() = default;
	AbstractTextureGate(unsigned ID,
	                    std::vector<TextureIndex> inputsFilterIndices,
	                    std::vector<TextureIndex> outputsFilterIndices);
	virtual ~AbstractTextureGate() = default;

	void updateInputs(Texture* inputPtr);
	void updateOutputs(Texture* outputPtr);
	void setInputsShift(TextureIndex shift);
	void setOutputsShift(TextureIndex shift);

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
                                         std::vector<TextureIndex> newInputsFilterIndices,
                                         std::vector<TextureIndex> newOutputsFilterIndices) :
	ID(newID),
	inputsFilterIndices(newInputsFilterIndices),
	outputsFilterIndices(newOutputsFilterIndices),
	inputs(newInputsFilterIndices.size(), nullptr),
	outputs(newOutputsFilterIndices.size(), nullptr) {}

inline void AbstractTextureGate::updateInputs(Texture* inputPtr) {
	if(!inputPtr) throw std::invalid_argument("AbstractTextureGate::updateInputs caught nullptr");
	for(size_t i=0; i<inputsFilterIndices.size(); i++)
		inputs[i] = &((*inputPtr)(inputsFilterIndices[i]+inputsShift)); // operator() of boost::multi_array
}

inline void AbstractTextureGate::updateOutputs(Texture* outputPtr) {
	if(!outputPtr) throw std::invalid_argument("AbstractTextureGate::updateOutputs caught nullptr");
	for(size_t o=0; o<outputsFilterIndices.size(); o++)
		outputs[o] = &((*outputPtr)(outputsFilterIndices[o]+outputsShift));
}

inline void AbstractTextureGate::setInputsShift(TextureIndex shift) {
	inputsShift = shift;
}

inline void AbstractTextureGate::setOutputsShift(TextureIndex shift) {
	outputsShift = shift;
}

inline nlohmann::json AbstractTextureGate::serialize() const {
	nlohmann::json out = nlohmann::json::object();
	out["id"] = ID;
	out["type"] = this->gateType();

	auto indices2json = [](TextureIndex idx) {
		return nlohmann::json::array({idx[0], idx[1], idx[2], idx[3]});
	};

	out["inputsFilterIndices"] = nlohmann::json::array();
	for(auto inputIdxs : inputsFilterIndices)
		out["inputsFilterIndices"].push_back(indices2json(inputIdxs));
	out["inputsShift"] = indices2json(inputsShift);

	out["outputsFilterIndices"] = nlohmann::json::array();
	for(auto outputIdxs : outputsFilterIndices)
		out["outputsFilterIndices"].push_back(indices2json(outputIdxs));
	out["outputsShift"] = indices2json(outputsShift);

	return out;
}

inline void AbstractTextureGate::deserialize(const nlohmann::json& in) {
	ID = in["id"];

	auto json2indices = [](const nlohmann::json& idxsjson) {
		return TextureIndex({{idxsjson[0], idxsjson[1], idxsjson[2], idxsjson[3]}});
	};

	inputsFilterIndices.clear();
	for(const auto& iidj : in["inputsFilterIndices"])
		inputsFilterIndices.push_back(json2indices(iidj));
	inputsShift = json2indices(in["inputsShift"]);
	outputsFilterIndices.clear();
	for(const auto& oidj : in["outputsFilterIndices"])
		outputsFilterIndices.push_back(json2indices(oidj));
	outputsShift = json2indices(in["outputsShift"]);

	inputs.resize(inputsFilterIndices.size());
	std::fill(inputs.begin(), inputs.end(), nullptr);
	outputs.resize(outputsFilterIndices.size());
	std::fill(outputs.begin(), outputs.end(), nullptr);
}