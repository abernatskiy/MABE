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

#include "../AbstractWorld.h"
#include "../EdlundMazeWorld/EdlundMazeWorld.h"
#include "../ComplexiPhiWorld/ComplexiPhiWorld.h"
#include "../../Global.h"

#include <cstdlib>
//#include <thread>
#include <vector>
#include <algorithm>

class MixtureWorld : public AbstractWorld {

public:
	ComplexiPhiWorld cpw;
	EdlundMazeWorld emw;

	static std::shared_ptr<ParameterLink<int>> environmentChangePeriodPL; // number of iterations between environment changes
	static std::shared_ptr<ParameterLink<int>> firstEnvironmentTypePL; // 1 - ComplexiPhi only, 2 - EdlundMaze only, 3 - average, 4 - minimum
	static std::shared_ptr<ParameterLink<int>> secondEnvironmentTypePL; // same for the second environment

	int environmentChangePeriod, firstEnvironmentType, secondEnvironmentType;

	static std::shared_ptr<ParameterLink<std::string>> groupNamePL;
	static std::shared_ptr<ParameterLink<std::string>> brainNamePL;
	std::string groupName;
	std::string brainName;

	MixtureWorld(std::shared_ptr<ParametersTable> PT_ = nullptr);
	virtual ~MixtureWorld() = default;

	void evaluateSolo(std::shared_ptr<Organism> org, int analyze, int visualize, int debug);

	virtual void evaluate(std::map<std::string, std::shared_ptr<Group>> &groups, int analyze, int visualize, int debug) {
		int popSize = groups[groupNamePL->get(PT)]->population.size();
		for (int i = 0; i < popSize; i++)
			evaluateSolo(groups[groupNamePL->get(PT)]->population[i], analyze, visualize, debug);
	};

	virtual std::unordered_map<std::string, std::unordered_set<std::string>> requiredGroups() override {
		return {{ groupNamePL->get(PT), {"B:" + brainNamePL->get(PT) + ",,"} }};
    // requires a root group and a brain (in root namespace) and no addtional
    // genome,
    // the brain must have 1 input, and the variable numberOfOutputs outputs
  };
};

