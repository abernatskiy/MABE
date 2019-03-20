#include <limits>

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

std::shared_ptr<ParameterLink<std::string>> AgeFitnessParetoOptimizer::maxFileFormulaPL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-maxFileFormula",
	(std::string) "DM_AVE[score]",
	"an MTree that is used to decide which individual is described in the generation's line of max.csv."
	"\nUnlike the formulas provided in optimizeFormulas, this one is maximized, owning to the name of the max.csv file");

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

	optimizeFormula = stringToMTree(maxFileFormulaPL->get(PT));
}

void AgeFitnessParetoOptimizer::optimize(std::vector<std::shared_ptr<Organism>>& population) {

	// Assigning age to all initial organisms
	if(firstGenIsNow) {
		if(population.size()==0) {
			std::cerr << "Age-fitness Pareto optimizer does not support initially empty populations, and it was applied to one" << std::endl << std::flush;
			exit(EXIT_FAILURE);
		}
		templateOrganism = population[0]->makeCopy(population[0]->PT);

		for(unsigned i=0; i<population.size(); i++) {
			population[i]->dataMap.set("minimizeValue_age", 0.);
			population[i]->dataMap.set("lineageID", (int) getNewLineageID());
		}
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
	// Step 1: copying the Pareto front to it - all of it is the elite
	newPopulation.insert(newPopulation.end(), paretoFront.begin(), paretoFront.end());

	// Step 2: adding the offspring of the Pareto front organisms to the population
	// until it reaches the projected size of the population minus one
	while(newPopulation.size() != population.size()-1) {
		std::shared_ptr<Organism> parent = paretoFront[Random::getIndex(paretoFront.size())];
		std::shared_ptr<Organism> child = parent->makeMutatedOffspringFrom(parent);
		child->dataMap.set("minimizeValue_age", parent->dataMap.getDouble("minimizeValue_age")); // it is very important to know how farback your ancestry goes
		child->dataMap.set("lineageID", parent->dataMap.getInt("lineageID")); // turns out knowing your surname also helps
		newPopulation.push_back(child);
	}

	// Intermission: printing a summary of the incoming population
	// Done here to have additional data such as Pareto order etc

	// Compact messages : a line per generation
	std::map<std::string,double> minValues;
	std::map<std::string,int> minValueCarriers;
	for(auto it=scoreNames.begin(); it!=scoreNames.end()-1; it++) { // exclude "minimizeValue_age"
		minValues[*it] = std::numeric_limits<double>::max();
		minValueCarriers[*it] = -1;
	}
	double maxAge = 0;
	int oldestOrganism = -1;
	for(const auto& champ : paretoFront) {
		for(const auto& sn : scoreNames) {
			if(sn == "minimizeValue_age") {
				if(champ->dataMap.getDouble(sn) >= maxAge) {
					maxAge = champ->dataMap.getDouble(sn);
					oldestOrganism = champ->ID;
				}
			}
			else {
				if(champ->dataMap.getDouble(sn) <= minValues[sn]) {
					minValues[sn] = champ->dataMap.getDouble(sn);
					minValueCarriers[sn] = champ->ID;
				}
			}
		}
	}

	logParetoFront(paretoFront);

	std::cout << "pareto_size=" << paretoFront.size();
	std::cout << " max_age=" << maxAge << "@" << oldestOrganism;
	for(const auto& mvtuple : minValues)
		std::cout << " " << mvtuple.first << "=" << mvtuple.second << "@" << minValueCarriers[mvtuple.first];
	std::cout << " extant_lineages=";
	for(auto pii=paretoFront.begin(); pii!=paretoFront.end(); pii++)
		std::cout << (*pii)->dataMap.getInt("lineageID") << (pii==paretoFront.end()-1 ? "" : ",") ;
	std::cout << std::flush;
/*
	// Detailed messages : full population & Pareto front snapshot
	std::cout << std::endl << "Incoming population:" << std::endl;

	for(unsigned i=0; i<population.size(); i++) {
		std::cout << "id" << population[i]->ID << " ";
		std::cout << "lineageid" << population[i]->dataMap.getInt("lineageID") << " ";
		for(auto objname : scoreNames) {
			std::cout << objname << population[i]->dataMap.getDouble(objname) << " ";
		}
		std::cout << "ParetoOrder" << paretoOrder[i];
		std::cout << " genomeSizes";
		for(auto gentuple : population[i]->genomes)
			std::cout << " " << gentuple.second->countSites();

		std::cout << " ++++ " << population[i]->dataMap.getTextualRepresentation(' ');

		std::cout << std::endl;
	}

	std::cout << "Pareto front:" << std::endl;
	for(auto org : paretoFront) {
		std::cout << "id" << org->ID << " ";
		std::cout << "lineageid" << org->dataMap.getInt("lineageID") << " ";
		for(auto objname : scoreNames) {
			std::cout << objname << org->dataMap.getDouble(objname) << " ";
		}
		std::cout << std::endl;
	}
*/
	// Step 3: incrementing age of everyone involved
	for(auto newOrgPtr : newPopulation)
		newOrgPtr->dataMap.set("minimizeValue_age", newOrgPtr->dataMap.getDouble("minimizeValue_age")+1.);
	// Step 4: adding an new bloodline with age of zero
	auto newOrg = makeNewOrganism();
	newOrg->dataMap.set("minimizeValue_age", 0.);
	newOrg->dataMap.set("lineageID", (int) getNewLineageID());
	newPopulation.push_back(newOrg);

}

void AgeFitnessParetoOptimizer::cleanup(std::vector<std::shared_ptr<Organism>>& population) {
	for(unsigned i=0; i<paretoOrder.size(); i++)
		if(paretoOrder[i]==0)
			population[i]->kill();
	population.swap(newPopulation);
	newPopulation.clear();
}

std::shared_ptr<Organism> AgeFitnessParetoOptimizer::makeNewOrganism() {
	std::unordered_map<std::string,std::shared_ptr<AbstractGenome>> genomes;
	for(const auto& gentuple : templateOrganism->genomes) {
		genomes[gentuple.first] = gentuple.second->makeCopy(gentuple.second->PT);
		genomes[gentuple.first]->fillRandom();
	}

	for(const auto& braintuple : templateOrganism->brains)
		braintuple.second->initializeGenomes(genomes);

	std::unordered_map<std::string,std::shared_ptr<AbstractBrain>> brains;
	for(const auto& braintuple : templateOrganism->brains)
		brains[braintuple.first] = braintuple.second->makeBrain(genomes);

	return std::make_shared<Organism>(genomes, brains, templateOrganism->PT);
}

unsigned AgeFitnessParetoOptimizer::getNewLineageID() {
	static unsigned curLineageID = 0;
	unsigned retID = curLineageID;
	curLineageID++;
	return retID;
}

void AgeFitnessParetoOptimizer::logParetoFront(const std::vector<std::shared_ptr<Organism>>& paretoFront) {
	std::string logpath = Global::outputPrefixPL->get() + "paretoFront.log";
	std::ofstream pflog;

	static bool firstRun = true;
	if(firstRun) {
		pflog.open(logpath, std::ios::out);
		pflog.close();
		firstRun = false;
	}

	pflog.open(logpath, std::ios::app);
	pflog << paretoFront.size() << std::endl;
	pflog.close();
}
