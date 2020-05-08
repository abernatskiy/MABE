#include "AbstractIsolatedEmbodiedWorld.h"

std::shared_ptr<ParameterLink<int>> AbstractIsolatedEmbodiedWorld::evaluationsPerGenerationPL =
    Parameters::register_parameter("WORLD_ISOLATED_EMBODIED-evaluationsPerGeneration", 1,
                                   "Number of times to test each genetically unique agent per "
                                   "generation (useful with non-deterministic brains)");
std::shared_ptr<ParameterLink<bool>> AbstractIsolatedEmbodiedWorld::assumeDeterministicEvaluationsPL =
    Parameters::register_parameter("WORLD_ISOLATED_EMBODIED-assumeDeterministicEvaluations", false,
                                   "If true, the world will never re-evaluate an organism that is already evaluated, even if evaluationsPerGenerations is greater than 1");
std::shared_ptr<ParameterLink<std::string>> AbstractIsolatedEmbodiedWorld::groupNamePL =
    Parameters::register_parameter("WORLD_ISOLATED_EMBODIED_NAMES-groupNameSpace", (std::string) "root::",
                                   "Namespace of group to be evaluated");
std::shared_ptr<ParameterLink<std::string>> AbstractIsolatedEmbodiedWorld::brainNamePL =
    Parameters::register_parameter("WORLD_ISOLATED_EMBODIED_NAMES-brainNameSpace", (std::string) "root::",
                                   "Namespace for parameters used to define brain");

AbstractIsolatedEmbodiedWorld::AbstractIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_, std::mt19937* worldRNG) :
	AbstractWorld(PT_),
	brain(nullptr),
	currentActualEvaluationNum(0) {

	// Locatizing the settings
	evaluationsPerGeneration = evaluationsPerGenerationPL->get(PT_);
	groupName = groupNamePL->get(PT_);
	brainName = brainNamePL->get(PT_);
	assumeDeterministicEvaluations = assumeDeterministicEvaluationsPL->get(PT_);
	if(assumeDeterministicEvaluations) {
		if(evaluationsPerGeneration != 1)
			std::cerr << "AbstractIsolatedEmbodiedWorld: WARNING! Request for repeating evaluations " << evaluationsPerGeneration << " times declined because WORLD_ISOLATED_EMBODIED-assumeDeterministicEvaluations is true" << std::endl;
		evaluationsPerGeneration = 1;
	}

	// Remembering the world-specific random generator pointer
	rng = worldRNG;
}

void AbstractIsolatedEmbodiedWorld::evaluateOnce(std::shared_ptr<Organism> org, unsigned repIdx, int visualize) {

	if(visualize) std::cout << "Evaluating organism " << org->ID << " at " << org << std::endl;
//	std::cout << "Evaluating organism " << org->ID << " at " << org << std::endl;

//	org->translateGenomesToBrains();

	resetWorld(visualize);

  brain = org->brains[brainName];

//	std::cout << "Organism is " << (org->alive ? "alive" : "dead") << ", brain pointer is " << brain << std::endl;

	sensors->reset(visualize);
	brain->resetBrain();
	motors->reset(visualize);

	sensors->attachToBrain(brain);
	motors->attachToBrain(brain);

	brain->attachToSensors(sensors->getDataForBrain());

	unsigned long timeStep = 0;

	while(!endEvaluation(timeStep)) {
		sensors->update(visualize);
		brain->update();
		motors->update(visualize);
		preEvaluationOuterWorldUpdate(org, timeStep, visualize);
		recordRunningScores(org, timeStep, visualize);
		postEvaluationOuterWorldUpdate(org, timeStep, visualize);
		timeStep++;
	}

	recordSampleScores(org, timeStep, visualize);

	if(assumeDeterministicEvaluations)
		org->dataMap.set("evaluated", true);
}
