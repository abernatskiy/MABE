#include "AgeFitnessParetoOptimizer.h"

std::shared_ptr<ParameterLink<std::string>> AgeFitnessParetoOptimizer::optimizeFormulasPL =
    Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-optimizeFormulas",
	(std::string) "(0-DM_AVE[score])",
    "values to minimize (!) with the optimizer, excluding age (list of MTrees)\n"
	"example for BerryWorld: [(0-DM_AVE[food1]),(0-DM_AVE[food2]),DM_AVE[switches]]");

std::shared_ptr<ParameterLink<std::string>> AgeFitnessParetoOptimizer::optimizeFormulaNamesPL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-optimizeFormulaNames",
	(std::string) "default",
	"column names to associate with optimize formulas in data files."
	"\n'default' will auto generate names as minimizeValue_<formula1>, minimizeValue_<formula2>, ...");

/***** Auxiliary functions *****/

bool firstOrganismIsDominatedBySecond(std::shared_ptr<Organism> first, std::shared_ptr<Organism> second, const std::vector<std::string>& minimizeableAttributes) {
	const bool breakTiesByID = true;

	bool isAnyBetterThan = false;
	for(const auto& attrname : minimizeableAttributes) {
		if(first->dataMap.getDouble(attrname) < second->dataMap.getDouble(attrname)) {
			isAnyBetterThan = true;
			break;
		}
	}

	if(isAnyBetterThan)
		return false;
	else {
		bool isAllEquivalentTo = true;
		for(const auto& attrname : minimizeableAttributes) {
			if(first->dataMap.getDouble(attrname) != second->dataMap.getDouble(attrname)) {
				isAllEquivalentTo = false;
				break;
			}
		}

		if(isAllEquivalentTo)
			if(breakTiesByID)
				return first->ID < second->ID;
			else
				return false;
		else
			return true;
	}
}

/***** AgeFitnessParetoOptimizer class definitions *****/

AgeFitnessParetoOptimizer::AgeFitnessParetoOptimizer(std::shared_ptr<ParametersTable> PT_)
    : AbstractOptimizer(PT_), firstGenIsNow(true) {
	// MTree formulas support inherited with minimal modifications from LexicaseOptimizer
	std::vector<std::string> optimizeFormulasStrings;
	convertCSVListToVector(optimizeFormulasPL->get(PT), optimizeFormulasStrings);
	for (auto s : optimizeFormulasStrings)
		optimizeFormulasMTs.push_back(stringToMTree(s));

	// get names to use with scores
	if (optimizeFormulaNamesPL->get(PT) == "default") {
		// user has not defined names, auto generate names
		for(auto ofs : optimizeFormulasStrings)
			scoreNames.push_back("minimizeValue_" + ofs);
	}
	else {
		// user has defined names, use those
		convertCSVListToVector(optimizeFormulaNamesPL->get(PT), scoreNames);
	}
	scoreNames.push_back("minimizeValue_age");

	popFileColumns.clear();
	for (auto &name : scoreNames)
		popFileColumns.push_back(name);
}

void AgeFitnessParetoOptimizer::optimize(std::vector<std::shared_ptr<Organism>>& population) {

	// Assigning age to all initial organisms
	if(firstGenIsNow) {
		if(population.size()==0) {
			std::cerr << "Age-fitness Pareto optimizer does not support initially empty populations, and it was applied to one" << std::endl << std::flush;
			exit(EXIT_FAILURE);
		}
		templateOrganism = population[0];

		for(unsigned i=0; i<population.size(); i++)
			population[i]->dataMap.set("minimizeValue_age", 0.);
		firstGenIsNow = false;
	}

	// Computing the values of all objectives and writing them into the organisms' data maps
	for(unsigned i=0; i<population.size(); i++) {
		for(unsigned p=0; p<optimizeFormulasMTs.size(); p++) {
			double mtreeeval = optimizeFormulasMTs[p]->eval(population[i]->dataMap, PT)[0];
			population[i]->dataMap.set(scoreNames[p], mtreeeval);
		}
	}

	// Finding and isolating the Pareto front
	paretoOrder.resize(population.size());
	std::fill(paretoOrder.begin(), paretoOrder.end(), 1);
	for(unsigned i=0; i<population.size(); i++) {
		for(auto other : population) {
			if(firstOrganismIsDominatedBySecond(population[i], other, scoreNames)) {
				paretoOrder[i] = 0;
				break;
			}
		}
	}

	paretoFront.clear();
	for(unsigned i=0; i<population.size(); i++)
		if(paretoOrder[i] != 0)
			paretoFront.push_back(population[i]);

	// Building a new population in four steps
	//newPopulation.clear();
	// Step 1: copying the Pareto front to it - all of it is the elite
	newPopulation.insert(newPopulation.end(), paretoFront.begin(), paretoFront.end());

	// Step 2: adding the offspring of the Pareto front organisms to the population
	// until it reaches the projected size of the population minus one
	while(newPopulation.size() != population.size()-1) {
		std::shared_ptr<Organism> parent = paretoFront[Random::getIndex(paretoFront.size())];
		std::shared_ptr<Organism> child = parent->makeMutatedOffspringFrom(parent);
		child->dataMap.set("minimizeValue_age", parent->dataMap.getDouble("minimizeValue_age")); // it is very important to know how farback your ancestry goes
		newPopulation.push_back(child);
	}


	// Printing a summary of the incoming population
	std::cout << std::endl << "Incoming population:" << std::endl;

	for(unsigned i=0; i<population.size(); i++) {
		std::cout << "id" << population[i]->ID << " ";
		for(auto objname : scoreNames) {
			std::cout << objname << population[i]->dataMap.getDouble(objname) << " ";
		}
		std::cout << "ParetoOrder" << paretoOrder[i];
		std::cout << std::endl;
	}

	std::cout << "Pareto front:" << std::endl;
	for(auto org : paretoFront) {
		std::cout << "id" << org->ID << " ";
		for(auto objname : scoreNames) {
			std::cout << objname << org->dataMap.getDouble(objname) << " ";
		}
		std::cout << std::endl;
	}

	// Step 3: incrementing age of everyone involved
	for(auto newOrgPtr : newPopulation)
		newOrgPtr->dataMap.set("minimizeValue_age", newOrgPtr->dataMap.getDouble("minimizeValue_age")+1.);
	// Step 4: adding an new bloodline with age of zero
	auto newOrg = makeNewOrganism();
	newOrg->dataMap.set("minimizeValue_age", 0.);
	newPopulation.push_back(newOrg);


	// some kind of useful output goes here
	// for (size_t fIndex = 0; fIndex < optimizeFormulasMTs.size(); fIndex++) {
	//  std::cout << std::endl
	//              << "   " << scoreNames[fIndex]
	//              << ":  max = " << std::to_string(maxScores[fIndex])
	//              << "   ave = " << std::to_string(aveScores[fIndex]) << std::flush;
	//  }
}

void AgeFitnessParetoOptimizer::cleanup(std::vector<std::shared_ptr<Organism>>& population) {
//	for(unsigned i=0; i<paretoOrder.size(); i++)
//		if(paretoOrder[i]==0)
//			population[i]->kill();
	population.swap(newPopulation);
	newPopulation.clear();
}

std::shared_ptr<Organism> AgeFitnessParetoOptimizer::makeNewOrganism() {
	std::shared_ptr<Organism> newbie = templateOrganism->makeCopy(); // randomize
	return newbie;
}

/*
int AgeFitnessParetoOptimizer::lexiSelect(const std::vector<int> &orgIndexList) {
	if (!scoresHaveDelta) { // if all scores are the same! pick random
		return Random::getIndex(orgIndexList.size());
	}

	// generate a vector with formulasSize and fill with values from 0 to formulasSize - 1, in a random order.
	std::vector<int> formulasOrder(optimizeFormulasMTs.size());
	iota(formulasOrder.begin(), formulasOrder.end(), 0);
	std::shuffle(formulasOrder.begin(), formulasOrder.end(), Random::getCommonGenerator());
	// now we have a random order in formulasOrder

	// keepers is the current list of indexes into population for orgs which
	// have passed all tests so far.
	std::vector<int> keepers  = orgIndexList;

	while (keepers.size() > 1 && formulasOrder.size() > 0) {
		// while there are still atleast one keeper and there are still formulas
		int formulaIndex = formulasOrder.back();
		formulasOrder.pop_back();

		double scoreCutoff;

		if (epsilonRelativeTo) { // get scoreCutoff relitive to score
			auto scoreRange = std::minmax_element(std::begin(scores[formulaIndex]), std::end(scores[formulaIndex]));
			scoreCutoff = (*scoreRange.second - ((*scoreRange.second - *scoreRange.first) * epsilon));
		}
		else { // get scoreCutoff relitive to rank
			// create a vector of remaning scores
			std::vector<double> keeperScores;
			for (size_t i = 0; i < keepers.size(); i++) {
				keeperScores.push_back(scores[formulaIndex][keepers[i]]);
			}
			// based on the number of keepers, calculate how many to keep.
			size_t cull_index = std::ceil(std::max((((1.0 - epsilon) * keeperScores.size()) - 1.0), 0.0));

			// get score at the cull index position
			std::nth_element(std::begin(keeperScores),
				std::begin(keeperScores) + cull_index,
				std::end(keeperScores));
			scoreCutoff = keeperScores[cull_index];

			//std::cout << "cull_index: " << cull_index << "  keeperScores.size(): " << keeperScores.size() << std::endl;
			//std::cout << "scoreCutoff: " << scoreCutoff << std::endl;
		}

		// for each keeper, see if there are still a keeper, i.e. they have score >= scoreCutoff. remove if not
		keepers.erase(std::remove_if(std::begin(keepers), std::end(keepers), [&](auto k) {
			return scores[formulaIndex][k] < scoreCutoff;
		}), std::end(keepers));
	}
	int pickHere = Random::getIndex(keepers.size()); 
	//std::cout << "    keeping: " << tournamentPopulation[keepers[pickHere]]->ID << std::endl;
	// if there is only one keeper left, return that, otherwise select randomly from keepers.
	return keepers[pickHere];
}
*/
