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

#include <string>

#include "AbstractWorld.h"
#include "../Organism/Organism.h"
#include "AbstractSensors.h"
#include "AbstractMotors.h"

class AbstractIsolatedEmbodiedWorld : public AbstractWorld {

public:
	static std::shared_ptr<ParameterLink<int>> evaluationsPerGenerationPL;
	static std::shared_ptr<ParameterLink<std::string>> groupNamePL;
	static std::shared_ptr<ParameterLink<std::string>> brainNamePL;

	AbstractIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_) : AbstractWorld(PT_) {};
	virtual ~AbstractIsolatedEmbodiedWorld() = default;

	void evaluateSolo(std::shared_ptr<Organism> org, int analyze, int visualize, int debug);

	virtual void evaluate(std::map<std::string, std::shared_ptr<Group>> &groups, int analyze, int visualize, int debug) {
		int popSize = groups[groupNamePL->get(PT)]->population.size();
		for(int i=0; i<popSize; i++)
			evaluateSolo(groups[groupNamePL->get(PT)]->population[i], analyze, visualize, debug);
  }

  virtual std::unordered_map<std::string, std::unordered_set<std::string>> requiredGroups() override {
		// If you override this function and use a Brain, please use numInputs() and numOutputs() appropriately in your implementation
		return {{groupNamePL->get(PT), {"B:" + brainNamePL->get(PT) + "," + std::to_string(numInputs()) + "," + std::to_string(numOutputs())}}};
  }

	virtual void resetWorld(int visualize) = 0;

	virtual int numInputs() = 0;
	virtual int numOutputs() = 0;

	std::shared_ptr<AbstractMotors> motors;
	std::shared_ptr<AbstractSensors> sensors;

	virtual bool endEvaluation(unsigned long timeStep) = 0;
	virtual void updateExtraneousWorld(int timeStep, int visualize) = 0;
	virtual void updateRunningScores(int evalTime, int visualize) = 0;
	virtual void recordFinalScores(int evalTime, int visualize) = 0;
	virtual void evaluateOrganism(std::shared_ptr<Organism> currentOrganism, int visualize) = 0;
};
