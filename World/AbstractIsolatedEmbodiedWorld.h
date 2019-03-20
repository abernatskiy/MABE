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
#include "../Brain/AbstractBrain.h"

#include "AbstractSensors.h"
#include "AbstractMotors.h"

class AbstractIsolatedEmbodiedWorld : public AbstractWorld {

protected:
	// Initialize these to proper daughter classes in the daughter classes' constuctors
	std::shared_ptr<AbstractMotors> motors;
	std::shared_ptr<AbstractSensors> sensors;

	virtual void resetWorld(int visualize) { brain=nullptr; }; // override this in daughter classes and call the parent's method

	// Do not touch this one, its values will be taken from Organisms
	std::shared_ptr<AbstractBrain> brain;

private:
	static std::shared_ptr<ParameterLink<int>> evaluationsPerGenerationPL;
	int evaluationsPerGeneration;
	static std::shared_ptr<ParameterLink<std::string>> groupNamePL;
	std::string groupName;
	static std::shared_ptr<ParameterLink<std::string>> brainNamePL;
	std::string brainName;
	static std::shared_ptr<ParameterLink<bool>> assumeDeterministicEvaluationsPL;
	bool assumeDeterministicEvaluations;

	void evaluateOnce(std::shared_ptr<Organism> org, int visualize);
	int numInputs() { return motors->numOutputs() + sensors->numOutputs(); }; // brain inputs
	int numOutputs() { return motors->numInputs() + sensors->numInputs(); }; // brain outputs

	virtual bool endEvaluation(unsigned long timeStep) = 0;
	virtual void preEvaluationOuterWorldUpdate(std::shared_ptr<Organism> org, unsigned long timeStep, int visualize) = 0;
	virtual void postEvaluationOuterWorldUpdate(std::shared_ptr<Organism> org, unsigned long timeStep, int visualize) = 0;
	virtual void recordRunningScores(std::shared_ptr<Organism> org, unsigned long evalTime, int visualize) = 0;
	virtual void recordSampleScores(std::shared_ptr<Organism> org, unsigned long evalTime, int visualize) = 0;
	virtual void evaluateOrganism(std::shared_ptr<Organism> currentOrganism, int visualize) = 0;

public:
	AbstractIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_);
	virtual ~AbstractIsolatedEmbodiedWorld() = default;

	// Add parallelization both on the level of organisms and on the level of repeats
	void evaluateSolo(std::shared_ptr<Organism> org, int analyze, int visualize, int debug) {
		if(assumeDeterministicEvaluations && org->dataMap.findKeyInData("evaluated")==11 && org->dataMap.getBool("evaluated"))
			return;
		for(int r=0; r<evaluationsPerGeneration; r++)
			evaluateOnce(org, visualize);
		evaluateOrganism(org, visualize);
	};
	void evaluate(std::map<std::string, std::shared_ptr<Group>> &groups, int analyze, int visualize, int debug) {
		int popSize = groups[groupName]->population.size();
		for(int i=0; i<popSize; i++)
			evaluateSolo(groups[groupName]->population[i], analyze, visualize, debug);
  };

	// Will need a rewrite if sensory or motor system will need a genome
	std::unordered_map<std::string, std::unordered_set<std::string>> requiredGroups() override {
		return {{groupName, {"B:" + brainName + "," + std::to_string(numInputs()) + "," + std::to_string(numOutputs())}}};
  };
};
