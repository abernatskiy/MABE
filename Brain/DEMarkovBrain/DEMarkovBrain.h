#pragma once

#include <cmath>
#include <memory>
#include <iostream>
#include <set>
#include <vector>
#include <iomanip>

#include "../AbstractBrain.h"
#include "../MarkovBrain/Gate/AbstractGate.h"
#include "../../Utilities/Random.h"
#include "../MarkovBrain/logFile.h"

class DEMarkovBrain : public AbstractBrain {
private:
	// Major constants
	int nrNodes;
	int nrHiddenNodes;
	int gateMinIns, gateMaxIns, gateMinOuts, gateMaxOuts;

	// Major state vars
	std::vector<std::shared_ptr<AbstractGate>> gates;
	std::vector<double> nodes;
	std::vector<double> nextNodes;

	// Private methods
	std::shared_ptr<AbstractGate> getRandomGate(int gateID);
	void randomize();
	void beginLogging();

	// Infrastructure
	const bool visualize;
	const bool recordIOMap;
	std::vector<std::vector<double>> nodesStatesTimeSeries;
	LogFile log;
	void logBrainStructure();

	static std::shared_ptr<ParameterLink<int>> hiddenNodesPL;
	static std::shared_ptr<ParameterLink<bool>> recordIOMapPL;
	static std::shared_ptr<ParameterLink<std::string>> IOMapFileNamePL;

	static std::shared_ptr<ParameterLink<int>> initialGateCountPL;
	static std::shared_ptr<ParameterLink<double>> gateInsertionProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> gateDeletionProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> gateDuplicationProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> connectionToTableChangeRatioPL;

public:
	// Public methods
	DEMarkovBrain() = delete;
	DEMarkovBrain(int nrInNodes, int nrHidNodes, int nrOutNodes, std::shared_ptr<ParametersTable> PT); // basic constructor that provides an empty object
	DEMarkovBrain(int ins, int outs, std::shared_ptr<ParametersTable> PT_) : // complete constructor that actually generates a working random brain
		DEMarkovBrain(ins, hiddenNodesPL->get(PT_), outs, PT_) {

		randomize();
		beginLogging();
	};
	~DEMarkovBrain() = default;

	void update() override;
	std::shared_ptr<AbstractBrain> makeCopy(std::shared_ptr<ParametersTable> PT_) override;
	std::shared_ptr<AbstractBrain> makeBrain(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // returns a random brain like the one from which it's called
	std::shared_ptr<AbstractBrain> makeBrainFrom(std::shared_ptr<AbstractBrain> brain,
	                                             std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override { return makeBrainFromMany({brain}, _genomes); };
	std::shared_ptr<AbstractBrain> makeBrainFromMany(std::vector<std::shared_ptr<AbstractBrain>> _brains,
	                                                 std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // brains' procreation
	void mutate() override;
	void resetBrain() override;
	void resetOutputs() override;
	void resetInputs() override;
	std::unordered_set<std::string> requiredGenomes() override { return {}; }

	// Infrastructure
	std::string description() override;
	DataMap getStats(std::string& prefix) override;
	std::string getType() override { return "DEMarkov"; }
	void* logTimeSeries(const std::string& label) override;
	void logNote(std::string note) override { log.log("DEMarkov Brain's external note: " + note + "\n"); };
};

inline std::shared_ptr<AbstractBrain> DEMarkovBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT) {
	return std::make_shared<DEMarkovBrain>(ins, outs, PT);
}
