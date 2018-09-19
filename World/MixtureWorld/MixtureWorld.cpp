//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

// Evaluates the organism in two worlds EdlundMaze and ComplexiPhi and sets the resulting scores appropriately

#include "MixtureWorld.h"

std::shared_ptr<ParameterLink<int>> MixtureWorld::environmentChangePeriodPL =
	Parameters::register_parameter("WORLD_MIXTURE-environmentChangePeriod", 100,
	                               "The number of generations that the world will "
	                               "evaluate the parameter changes");
std::shared_ptr<ParameterLink<int>> MixtureWorld::firstEnvironmentTypePL =
	Parameters::register_parameter("WORLD_MIXTURE-firstEnvironmentType", 1,
	                               "1 - ComplexiPhi only, 2 - EdlundMaze only,"
	                               "3 - average, 4 - minimum");
std::shared_ptr<ParameterLink<int>> MixtureWorld::secondEnvironmentTypePL =
	Parameters::register_parameter("WORLD_MIXTURE-secondEnvironmentType", 2,
	                               "1 - ComplexiPhi only, 2 - EdlundMaze only,"
	                               "3 - average, 4 - minimum");

std::shared_ptr<ParameterLink<std::string>> MixtureWorld::groupNamePL =
    Parameters::register_parameter("WORLD_MIXTURE_NAMES-groupNameSpace",
                                   (std::string) "root::",
                                   "namespace of group to be evaluated");
std::shared_ptr<ParameterLink<std::string>> MixtureWorld::brainNamePL =
    Parameters::register_parameter(
        "WORLD_MIXTURE_NAMES-brainNameSpace", (std::string) "root::",
        "namespace for parameters used to define brain");

MixtureWorld::MixtureWorld(std::shared_ptr<ParametersTable> PT_)
    : AbstractWorld(PT_), cpw(PT_), emw(PT_) {

	// localization
	groupName = groupNamePL->get(PT_);
	brainName = brainNamePL->get(PT_);
	environmentChangePeriod = environmentChangePeriodPL->get(PT_);
	firstEnvironmentType = firstEnvironmentTypePL->get(PT_);
	secondEnvironmentType = secondEnvironmentTypePL->get(PT_);

  // columns to be added to ave file
  popFileColumns.clear();
  popFileColumns.push_back("score");
  popFileColumns.push_back("score_VAR"); // specifies to also record the
                                         // variance (performed automatically
                                         // because _VAR)
	popFileColumns.push_back("scoreComplexiPhi");
	popFileColumns.push_back("scoreEdlundMaze");
}

void MixtureWorld::evaluateSolo(std::shared_ptr<Organism> org, int analyze, int visualize, int debug) {

	if (debug) std::cerr << "evaluateSolo called on organism " << org->ID << std::endl << std::flush;

	// determining which mode of operation should be used in this epoch
	// important to do that early so that we can forgo evaluating in the irrelevant world, if there is one
	// (note: this optimization is not implemented at the moment)
	int mode;
	if ( (Global::update / environmentChangePeriod) % 2 == 0 )
		mode = firstEnvironmentType;
	else
		mode = secondEnvironmentType;

	if (debug) std::cerr << "Mode determined to be " << mode << std::endl << std::flush;

	// saving the existing score data
	std::vector<double> oldScores;
	if(org->dataMap.fieldExists("score")) {
		std::copy(org->dataMap.getDoubleVector("score").begin(), org->dataMap.getDoubleVector("score").end(), oldScores.begin());
		if (debug) std::cerr << "Old scores saved" << std::endl << std::flush;
	}

	// localizing the brain so that we can reset it between the worlds
	auto brain = org->brains[brainName];

	// evaluating the organism in both worlds
	brain->resetBrain();
	if (visualize) std::cout << "Evaluating organism " << org->ID << " in ComplexiPhi world" << std::endl;
	cpw.evaluateSolo(org, analyze, visualize, debug);
	double cpwScore = org->dataMap.getDoubleVector("score").back();
	if (visualize) std::cout << "ComplexiPhi world score is " << cpwScore << " for " << org->ID << std::endl;
	org->dataMap.append("scoreComplexiPhi", cpwScore);

	brain->resetBrain();
	if (visualize) std::cout << "Evaluating organism " << org->ID << " in EdlundMaze world" << std::endl;
	emw.evaluateSolo(org, analyze, visualize, debug);
	double emwScore = org->dataMap.getDoubleVector("score").back();
	if (visualize) std::cout << "EdlundMaze world score is " << emwScore << " for " << org->ID << std::endl;
	org->dataMap.append("scoreEdlundMaze", emwScore);

	// restoring the score data
	org->dataMap.clear("score");
	for(auto s : oldScores)
		org->dataMap.append("score", s);

	// determining the new score and appending it to the old scores vector
	double score;
	switch(mode) {
		case 1: score = cpwScore; break;
		case 2: score = emwScore; break;
		case 3: score = (cpwScore+emwScore)/2.; break;
		case 4: score = cpwScore<emwScore ? cpwScore : emwScore; break;
		default:
			std::cerr << "Mode of score mixing " << mode << " unrecognized at update " << Global::update << std::endl;
			exit(EXIT_FAILURE);
	}
	if (score < 0.) score = 0.;
	org->dataMap.append("score", score);

	if (visualize) std::cout << "Final score for organism " << org->ID << " is " << score << std::endl;

/*
	auto brain = org->brains[brainName];

	brain->resetBrain();
	brain->setInput(0, 1); // give the brain a constant 1 (for wire brain)
	brain->update();

  double score = 0.0;
  for (int i = 0; i < brain->nrOutputValues; i++) {
    if (modePL->get(PT) == 0)
      score += Bit(brain->readOutput(i));
    else
      score += brain->readOutput(i);
  }
  if (score < 0.0)
    score = 0.0;
  org->dataMap.append("score", score);
  if (visualize)
    std::cout << "organism with ID " << org->ID << " scored " << score << std::endl;
*/
}

