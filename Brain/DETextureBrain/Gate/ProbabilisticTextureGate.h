#pragma once

#include "AbstractTextureGate.h"

class ProbabilisticTextureGate: public AbstractTextureGate {
public:
	std::vector<std::vector<float>> table; // at [i][j] it contains the probability of output j given input i
	size_t numInputPatterns;
	size_t numOutputPatterns;

	ProbabilisticTextureGate() = default;
	ProbabilisticTextureGate(unsigned ID,
	                         std::vector<TextureIndex> inputsFilterIndices,
	                         std::vector<TextureIndex> outputsFilterIndices); // uses process-wide RNG to initialize the table
	~ProbabilisticTextureGate() = default;

	void update(std::mt19937* rng=nullptr) override;
	std::shared_ptr<AbstractTextureGate> makeCopy(unsigned copyID) const override;
	std::string description() const override;
	std::string gateType() const override { return "ProbabilisticTexture"; };
	void mutateInternalStructure() override; // uses the process-wide RNG
	nlohmann::json serialize() const override;
	void deserialize(const nlohmann::json&) override;

private:
	void normalizeOutput(size_t inputIdx);
};
