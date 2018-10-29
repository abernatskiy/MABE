//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include "AbstractIsolatedEmbodiedWorld.h"

std::shared_ptr<ParameterLink<int>> AbstractIsolatedEmbodiedWorld::evaluationsPerGenerationPL =
    Parameters::register_parameter("WORLD_ISOLATED_EMBODIED-evaluationsPerGeneration", 1,
                                   "Number of times to test each genetically unique agent per "
                                   "generation (useful with non-deterministic brains)");
std::shared_ptr<ParameterLink<std::string>> AbstractIsolatedEmbodiedWorld::groupNamePL =
    Parameters::register_parameter("WORLD_ISOLATED_EMBODIED_NAMES-groupNameSpace", (std::string) "root::",
                                   "Namespace of group to be evaluated");
std::shared_ptr<ParameterLink<std::string>> AbstractIsolatedEmbodiedWorld::brainNamePL =
    Parameters::register_parameter("WORLD_ISOLATED_EMBODIED_NAMES-brainNameSpace", (std::string) "root::",
                                   "Namespace for parameters used to define brain");

AbstractIsolatedEmbodiedWorld::AbstractIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_) : AbstractWorld(PT_), brain(nullptr) {

	// Locatizing the settings
	evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT_);
	groupName = groupNamePL->get(PT_);
	brainName = brainNamePL->get(PT_);
}

void AbstractIsolatedEmbodiedWorld::evaluateOnce(std::shared_ptr<Organism> org, int visualize) {

	resetWorld(visualize);

  brain = org->brains[brainName];

	sensors->reset(visualize);
	brain->resetBrain();
	motors->reset(visualize);

	motors->attachToBrain(brain);
	sensors->attachToBrain(brain);

	unsigned long timeStep = 0;

	while(!endEvaluation(timeStep)) {
		sensors->update(visualize);
		brain->update();
		motors->update(visualize);
		preEvaluationOuterWorldUpdate(timeStep, visualize);
		recordRunningScores(timeStep, visualize);
		postEvaluationOuterWorldUpdate(timeStep, visualize);
		timeStep++;
	}

	recordSampleScores(timeStep, visualize);
}
