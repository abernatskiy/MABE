//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#pragma once

#include <cmath>
#include <memory>
#include <iostream>
#include <set>
#include <vector>
#include <iomanip>
#include <sys/stat.h>

#include "GateListBuilder/GateListBuilder.h"
#include "../../Genome/AbstractGenome.h"

#include "../../Utilities/Random.h"

#include "../AbstractBrain.h"

inline double pointEntropy(double p) {
	if(p == 0. || p == 1.)
		return 0;
	else
		return -p*log2(p)-(1.-p)*log2(1.-p);
}

class LogFile {
private:
	static int fileSuffix;
	bool isOpen;
	std::ofstream fs;
	bool fileExists(std::string fn) {
		struct stat buffer;
		return (stat (fn.c_str(), &buffer) == 0); // stackoverflow.com/questions/12774207
	};
	std::string getNextAvailableFile(std::string prefix) {
		std::string candidateFN;
		while(1) {
			candidateFN = prefix + std::to_string(fileSuffix) + ".log";
			if(fileExists(candidateFN)) {
				fileSuffix++;
			} else {
				return candidateFN;
			}
		}
	};

	std::vector<std::vector<unsigned>> nodeStatesTS;
	std::vector<std::vector<unsigned>> nodePairsStatesTS;

public:
	LogFile() : isOpen(false) {};
	void open(std::string prefix) {
		if (!isOpen) {
			std::string availableFile = getNextAvailableFile(prefix);
			fs.open(availableFile, std::ios::out); // std::ios::out|std::ios::app for append
			isOpen = true;
		}
	};
	~LogFile() { if (isOpen) fs.close(); };
	void log(std::string s) { if (isOpen) fs << s; };

	void logStateBeforeUpdate(const std::vector<double>& nodes) {
		// First, good old file logging
		fs << "Transition:" << std::setprecision(1);
    for(const auto& s : nodes)
      fs << ' ' << s;
    fs << " ->";

		// Next, remembering the initial state for future processing
		if(nodeStatesTS.size()==0)
			for(const auto& s : nodes)
				nodeStatesTS.push_back({s>0 ? 1 : 0});
	};

	void logStateAfterUpdate(const std::vector<double>& nodes) {
		// Complimentary logging after the update
		fs << std::setprecision(1);
		for(const auto& s : nodes)
			fs << ' ' << s;
		fs << std::endl;

		// Remembering the current state
		for(unsigned i=0; i<nodes.size(); i++)
			nodeStatesTS[i].push_back(nodes[i]>0 ? 1 : 0);
	};

	void logBrainReset() {

		fs << "Brain was reset" << std::endl;

		fs << "Node probabilities of one:";
		fs << std::setprecision(2);
		std::vector<double> probsOfOne;
		for(auto nsts : nodeStatesTS) {
			unsigned sumOfStates = std::accumulate(nsts.begin(), nsts.end(), 0);
			double prob = static_cast<double>(sumOfStates)/static_cast<double>(nsts.size());
			probsOfOne.push_back(prob);
			fs << ' ' << prob;
		}
		fs << std::endl;
		nodeStatesTS.clear();

		fs << "Node enthropies:";
		fs << std::setprecision(2);
		for(auto p : probsOfOne) {
			fs << ' ' << pointEntropy(p);
		}
		fs << std::endl;
	};
};

class MarkovBrain : public AbstractBrain {

public:
	LogFile log;
	const bool visualize;
	void logBrainStructure();
	void logNote(std::string note) override { log.log("External note: " + note + "\n"); };

	std::vector<std::shared_ptr<AbstractGate>> gates;

	//	static shared_ptr<ParameterLink<int>> bitsPerBrainAddressPL;  // how
	//many bits are evaluated to determine the brain addresses.
	//	static shared_ptr<ParameterLink<int>> bitsPerCodonPL;

	static std::shared_ptr<ParameterLink<bool>> randomizeUnconnectedOutputsPL;
	static std::shared_ptr<ParameterLink<bool>> recordIOMapPL;
	static std::shared_ptr<ParameterLink<std::string>> IOMapFileNamePL;
	static std::shared_ptr<ParameterLink<int>> randomizeUnconnectedOutputsTypePL;
	static std::shared_ptr<ParameterLink<double>>
	    randomizeUnconnectedOutputsMinPL;
	static std::shared_ptr<ParameterLink<double>>
	    randomizeUnconnectedOutputsMaxPL;
	static std::shared_ptr<ParameterLink<int>> hiddenNodesPL;
	static std::shared_ptr<ParameterLink<std::string>> genomeNamePL;

	bool randomizeUnconnectedOutputs;
	bool randomizeUnconnectedOutputsType;
	double randomizeUnconnectedOutputsMin;
	double randomizeUnconnectedOutputsMax;
	int hiddenNodes;
	std::string genomeName;

	std::vector<double> nodes;
	std::vector<double> nextNodes;

	int nrNodes;

	std::shared_ptr<AbstractGateListBuilder> GLB;
	std::vector<int> nodesConnections, nextNodesConnections;

	std::vector<int> inputNodeMap;
	std::vector<int> outputNodeMap;

	/*
		* Builds a look up table to convert genome site values into brain state
		* addresses - this is only used when there is a fixed number of brain states
		* if there is a variable number of brain states, then the node map must be
		* rebuilt.
		*/
	static void makeNodeMap(std::vector<int>& nodeMap, int genomeFieldSizeInBits, int minNodeAddress, int maxNodeAddress) {

		const int numAddresses = maxNodeAddress - minNodeAddress + 1;
		const int numStates = pow(2, genomeFieldSizeInBits);

		if(numAddresses > numStates) {
			std::cerr << "WARNING: Requested to map " << genomeFieldSizeInBits
			          << " bits (capable of taking " << numStates << " values) to "
			          << numAddresses << " node addresses (min " << minNodeAddress
			          << ", max " << maxNodeAddress << ")" << std::endl;
		}

		for(int i=0; i<numStates; i++)
			nodeMap.push_back(minNodeAddress + (i % numAddresses));
	}

	MarkovBrain() = delete;

	MarkovBrain(std::vector<std::shared_ptr<AbstractGate>> _gates, int _nrInNodes,
	            int _nrOutNodes, std::shared_ptr<ParametersTable> PT_ = nullptr);
	MarkovBrain(std::shared_ptr<AbstractGateListBuilder> GLB_, int _nrInNodes,
	            int _nrOutNodes, std::shared_ptr<ParametersTable> PT_ = nullptr);
	MarkovBrain(std::shared_ptr<AbstractGateListBuilder> GLB_,
	            std::unordered_map<std::string, std::shared_ptr<AbstractGenome>> &_genomes,
	            int _nrInNodes, int _nrOutNodes,
	            std::shared_ptr<ParametersTable> PT_ = nullptr);

	virtual ~MarkovBrain() = default;

	virtual std::shared_ptr<AbstractBrain>
	makeCopy(std::shared_ptr<ParametersTable> PT_ = nullptr) override;

	void readParameters();

	virtual void update() override;

	void inOutReMap();

	// Make a brain like the brain that called this function, using genomes and
	// initalizing other elements.
	virtual std::shared_ptr<AbstractBrain> makeBrain(
	    std::unordered_map<std::string, std::shared_ptr<AbstractGenome>> &_genomes) override;

	virtual std::string description() override;
	void fillInConnectionsLists();
	virtual DataMap getStats(std::string &prefix) override;
	virtual std::string getType() override { return "Markov"; }

	virtual void resetBrain() override;
	virtual void resetOutputs() override;
	virtual void resetInputs() override;

	virtual std::string gateList();
	virtual std::vector<std::vector<int>> getConnectivityMatrix();
	virtual int brainSize();
	int numGates();

	std::vector<int> getHiddenNodes();

	virtual void initializeGenomes(
	    std::unordered_map<std::string, std::shared_ptr<AbstractGenome>> &_genomes) override;

	virtual std::unordered_set<std::string> requiredGenomes() override {
		return {genomeNamePL->get(PT)};
	}


	std::vector<std::shared_ptr<AbstractBrain>> getAllSingleGateKnockouts();

};

inline std::shared_ptr<AbstractBrain>
MarkovBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT) {
  return std::make_shared<MarkovBrain>(std::make_shared<ClassicGateListBuilder>(PT), ins,
                                  outs, PT);

}

