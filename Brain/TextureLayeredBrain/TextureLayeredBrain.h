#pragma once

#include <iostream>
#include <vector>

#include "../DETextureBrain/DETextureBrain.h"

class TextureLayeredBrain : public AbstractBrain {
private:
	static std::vector<std::shared_ptr<ParametersTable>> layerPTs;

	// Major constants & state vars
	unsigned numLayers;
	std::vector<std::shared_ptr<AbstractBrain>> layers;
	std::vector<bool> layerEvolvable;
	std::vector<double> componentMutationRates;
	std::vector<void*> layerTextures; // is in inverted order to ease the interpretation by receiver

	// Infrastructure
	const bool visualize;
	static std::shared_ptr<ParameterLink<double>> totalMutationProbabilityPL;

public:
	// Public methods
	TextureLayeredBrain() = delete;
	TextureLayeredBrain(int ins, int outs, std::shared_ptr<ParametersTable> PT); // constructor generates an empty brain, then it is randomized be the factory
	~TextureLayeredBrain() = default;

	void update(std::mt19937* rng=nullptr) override;
	std::shared_ptr<AbstractBrain> makeCopy(std::shared_ptr<ParametersTable> PT_) override;
	std::shared_ptr<AbstractBrain> makeBrain(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // returns a random brain like the one from which it's called
	std::shared_ptr<AbstractBrain> makeBrainFrom(std::shared_ptr<AbstractBrain> brain,
	                                             std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override { return makeBrainFromMany({brain}, _genomes); };
	std::shared_ptr<AbstractBrain> makeBrainFromMany(std::vector<std::shared_ptr<AbstractBrain>> _brains,
	                                                 std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // brains' procreation
	void mutate() override;
	void resetBrain() override;
	void attachToSensors(void*) override;
	void* getDataForMotors() override;
	nlohmann::json getPostEvaluationStats() override;
	std::unordered_set<std::string> requiredGenomes() override { return {}; }

	// Infrastructure
	std::string description() override;
	DataMap getStats(std::string& prefix) override;
	std::string getType() override { return "TextureLayered"; };
	void* logTimeSeries(const std::string& label) override;
	void logNote(std::string note) override { std::cout << "TextureLayered Brain's external note: " << note << std::endl; };
	DataMap serialize(std::string& name) override;
	void deserialize(std::shared_ptr<ParametersTable> PT, std::unordered_map<std::string,std::string>& orgData, std::string& name) override;
};

inline std::shared_ptr<AbstractBrain> TextureLayeredBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT) {
	auto newBrain = std::make_shared<TextureLayeredBrain>(ins, outs, PT);
	return newBrain;
}
