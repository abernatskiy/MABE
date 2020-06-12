#pragma once

#include "AbstractTextureGate.h"

class DeterministicTextureGate: public AbstractTextureGate {
public:
	std::vector<std::vector<uint8_t>> table;

	DeterministicTextureGate() = default;
	DeterministicTextureGate(unsigned ID,
	                         std::vector<TextureIndex> inputsFilterIndices,
	                         std::vector<TextureIndex> outputsFilterIndices,
	                         long* erasureCounterPtr); // uses process-wide RNG to initialize the table
	~DeterministicTextureGate() = default;

	void update(std::mt19937* rng=nullptr) override; // ignores the RNG since the gate is deterministic
	std::shared_ptr<AbstractTextureGate> makeCopy(unsigned copyID) const override;
	std::string description() const override;
	std::string gateType() const override { return "DeterministicTexture"; };
	void mutateInternalStructure() override; // uses the process-wide RNG
	nlohmann::json serialize() const override;
	void deserialize(const nlohmann::json&) override;
};
