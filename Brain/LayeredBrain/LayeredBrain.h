#pragma once

#include <iostream>
#include <vector>

#include "../AbstractBrain.h"

class LayeredBrain : public AbstractBrain {
private:
	// Major constants
//	int nrNodes;
//	int nrHiddenNodes;

	// Major state vars
	unsigned numLayers;
	std::vector<std::shared_ptr<AbstractBrain>> layers;
	std::vector<bool> layerEvolvable;
	std::vector<double> mutationRates;

	// Private methods
//	std::shared_ptr<AbstractGate> getRandomGate(int gateID);
//	void beginLogging();

	// Infrastructure
	const bool visualize;
//	const bool recordIOMap;
//	std::vector<std::vector<double>> nodesStatesTimeSeries;
//	LogFile log;
//	void logBrainStructure();
//	int getLowestAvailableGateID();
//	std::string originationStory;

/*
	static std::shared_ptr<ParameterLink<int>> hiddenNodesPL;
	static std::shared_ptr<ParameterLink<bool>> recordIOMapPL;
	static std::shared_ptr<ParameterLink<std::string>> IOMapFileNamePL;

	static std::shared_ptr<ParameterLink<int>> initialGateCountPL;
	static std::shared_ptr<ParameterLink<double>> gateInsertionProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> gateDeletionProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> gateDuplicationProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> connectionToTableChangeRatioPL;
	static std::shared_ptr<ParameterLink<int>> minGateCountPL;
*/

public:
	// Public methods
	LayeredBrain() = delete;
	LayeredBrain(int ins, int outs, std::shared_ptr<ParametersTable> PT); // constructor generates an empty brain, then it is randomized be the factory
	~LayeredBrain() = default;

//	void randomize();

	void update() override;
	std::shared_ptr<AbstractBrain> makeCopy(std::shared_ptr<ParametersTable> PT_) override;
	std::shared_ptr<AbstractBrain> makeBrain(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // returns a random brain like the one from which it's called
	std::shared_ptr<AbstractBrain> makeBrainFrom(std::shared_ptr<AbstractBrain> brain,
	                                             std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override { return makeBrainFromMany({brain}, _genomes); };
	std::shared_ptr<AbstractBrain> makeBrainFromMany(std::vector<std::shared_ptr<AbstractBrain>> _brains,
	                                                 std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // brains' procreation
	void mutate() override;
	void resetBrain() override;
//	void resetOutputs() override;
//	void resetInputs() override;
	std::unordered_set<std::string> requiredGenomes() override { return {}; }

	// Infrastructure
	std::string description() override;
	DataMap getStats(std::string& prefix) override;
	std::string getType() override { return "Layered"; };
	void* logTimeSeries(const std::string& label) override;
	void logNote(std::string note) override { std::cout << "Layered Brain's external note: " << note << std::endl; };
	DataMap serialize(std::string& name) override;
	void deserialize(std::shared_ptr<ParametersTable> PT, std::unordered_map<std::string,std::string>& orgData, std::string& name) override;
};

inline std::shared_ptr<AbstractBrain> LayeredBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT) {
	auto newBrain = std::make_shared<LayeredBrain>(ins, outs, PT);
//	newBrain->randomize();
	return newBrain;
}