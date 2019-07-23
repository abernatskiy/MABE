#include <limits>
#include "../../Utilities/nlohmann/json.hpp"

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

std::shared_ptr<ParameterLink<int>> AgeFitnessParetoOptimizer::logLineagesPL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-logLineages",
	0,
	"logging level for lineages history: 0 - don't log (default), 1 - log Pareto optimal indivs only, 2 - log everyone");

std::shared_ptr<ParameterLink<bool>> AgeFitnessParetoOptimizer::logMutationStatsPL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-logMutationStats",
	false,
	"set to true to log the statistics of mutations, default: false");

std::shared_ptr<ParameterLink<double>> AgeFitnessParetoOptimizer::lineageAdditionPeriodPL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-lineageAdditionPeriod",
	1.0,
	"how frequently new lineages should be added (p>1 - a lineage added once every floor(p) gens,\n"
	"0<p<1 - floor(1/p) lineages added every generation, p<0 - no lineages added except to the initial population), default: 1.0");

std::shared_ptr<ParameterLink<bool>> AgeFitnessParetoOptimizer::useTournamentSelectionPL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-useTournamentSelection",
	false,
	"use rank selection based on Pareto rank instead of selecting the top Pareto front and its offspring, default: false");

std::shared_ptr<ParameterLink<int>> AgeFitnessParetoOptimizer::tournamentSizePL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-tournamentSize",
	2,
	"size of the tournament if tournament selection is used, default: 2");

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

AgeFitnessParetoOptimizer::AgeFitnessParetoOptimizer(std::shared_ptr<ParametersTable> PT_) :
	AbstractOptimizer(PT_),
	firstGenIsNow(true),
	searchIsStuck(false),
	logLineagesLvl(logLineagesPL->get(PT_)),
	logMutationStats(logMutationStatsPL->get(PT_)),
	useTournamentSelection(useTournamentSelectionPL->get(PT_)),
	tournamentSize(tournamentSizePL->get(PT_)) {

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

	double dLineageAdditionPeriod = lineageAdditionPeriodPL->get(PT);
	if(dLineageAdditionPeriod <= 0.) {
		disableLineageAddition = true;
		lineageAdditionPeriod = std::numeric_limits<unsigned>::max();
		lineagesPerAddition = 0;
	}
	else if(dLineageAdditionPeriod >= 1.) {
		disableLineageAddition = false;
		lineageAdditionPeriod = static_cast<unsigned>(floor(dLineageAdditionPeriod));
		lineagesPerAddition = 1;
	}
	else {
		disableLineageAddition = false;
		lineageAdditionPeriod = 1;
		lineagesPerAddition = static_cast<unsigned>(floor(1./dLineageAdditionPeriod));
	}

	// initializing the mutation statistic storage
	for(std::string mutName : {"mutation_insertion", "mutation_deletion", "mutation_duplication", "mutation_table", "mutation_rewiring"}) {
		mutationStatistics[mutName] = {};
		for(const std::string& scName : scoreNames) {
			if(scName == "minimizeValue_age")
				continue;
			mutationStatistics[mutName][scName] = {};
			mutationStatistics[mutName][std::string("delta_") + scName] = {};
		}
	}
}

void AgeFitnessParetoOptimizer::optimize(std::vector<std::shared_ptr<Organism>>& population) {
	// Don't waste our time if we know things are bad
	if(searchIsStuck) {
		population.swap(newPopulation);
		return;
	}

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

	// Gathering mutation statistics
	if(logMutationStats) {
		for(const auto& indiv : population) {
			std::string origStory = indiv->dataMap.getString("originationStory");
			if(origStory != "primordial") {
				for(const auto& scname : scoreNames) {
					if(scname == "minimizeValue_age")
						continue;
					double pastScore = indiv->dataMap.getDouble(std::string("parents_") + scname);
					double curScore = indiv->dataMap.getDouble(scname);
					mutationStatistics[origStory][scname].push_back(curScore);
					mutationStatistics[origStory][std::string("delta_") + scname].push_back(curScore-pastScore);
				}
			}
		}
	}

	// Computing Pareto ranks
	paretoRanks.resize(population.size());
	std::fill(paretoRanks.begin(), paretoRanks.end(), 0);
	if(useTournamentSelection) {
		unsigned currentRank = 0;
		while(!updateParetoRanks(population, currentRank))
			currentRank++;
	}
	else
		updateParetoRanks(population, 0);

	paretoFront.clear();
	for(unsigned i=0; i<population.size(); i++)
		if(paretoRanks[i] == 0)
			paretoFront.push_back(population[i]);

	// Computing how many new lineages we should add
	unsigned lineagesToAddNow = (Global::update % lineageAdditionPeriod == 0) ? lineagesPerAddition : 0;

	// Checking if the new population will fit into the size of the old one
	if(paretoFront.size()+lineagesToAddNow >= population.size())
		std::cout << "WARNING: Pareto front is large enough so that there is not enough space for variation and/or new lineages" << std::endl;
	if(paretoFront.size() == population.size())
		searchIsStuck = true;

	survivorIds.clear();

	if(useTournamentSelection) { // for tournament selection, the population is formed in trials

		while(newPopulation.size() < population.size()-lineagesToAddNow) {
			// finding the champion
			unsigned championIdx = Random::getIndex(population.size());
			std::vector<unsigned> participants = { championIdx };
			for(unsigned i=0; i<tournamentSize-1; i++) {
				unsigned challengerIdx = championIdx;
				while( std::find(participants.begin(), participants.end(), challengerIdx)!=participants.end() )
					challengerIdx = Random::getIndex(population.size());
				participants.push_back(challengerIdx);

				if(paretoRanks[championIdx]>=paretoRanks[challengerIdx])
					championIdx = challengerIdx;
			}

			// then copying it and its offspring into the next population
			std::shared_ptr<Organism> champion = population[championIdx];
			newPopulation.push_back(champion);
			survivorIds.insert(champion->ID);
//			std::cout << "added orgs " << champion->ID;
			for(unsigned i=0; i<tournamentSize-1; i++) {
				std::shared_ptr<Organism> child = champion->makeMutatedOffspringFrom(champion);
				child->dataMap.set("minimizeValue_age", champion->dataMap.getDouble("minimizeValue_age")); // it is very important to know how farback your ancestry goes
				child->dataMap.set("lineageID", champion->dataMap.getInt("lineageID")); // turns out knowing your surname also helps
				for(const auto& sn : scoreNames)
					child->dataMap.set(std::string("parents_") + sn, champion->dataMap.getDouble(sn));
				newPopulation.push_back(child);
//				std::cout << "," << child->ID;
			}
//			std::cout << " to the new population" << std::endl;
		}

		// If the space available in the new population does not divide evenly into tournament-sized chunks,
		// then the procedure above generates more individuals than can fit into the new population.
		// The line below resizes the new population to the right size and destroys the surplus.
		newPopulation.resize(population.size()-lineagesToAddNow);
	}
	else { // in elite selection, a new population is built in four steps

		// Copying the Pareto front to it - all of it is the elite
		newPopulation.insert(newPopulation.end(), paretoFront.begin(), paretoFront.end());
		for(unsigned i=0; i<paretoRanks.size(); i++)
			if(paretoRanks[i]==0)
				survivorIds.insert(population[i]->ID);

		// Adding the offspring of the Pareto front organisms to the population
		// until it reaches the projected size of the population minus one
		while(newPopulation.size() < population.size()-lineagesToAddNow) {
			std::shared_ptr<Organism> parent = paretoFront[Random::getIndex(paretoFront.size())];
			std::shared_ptr<Organism> child = parent->makeMutatedOffspringFrom(parent);
			child->dataMap.set("minimizeValue_age", parent->dataMap.getDouble("minimizeValue_age")); // it is very important to know how farback your ancestry goes
			child->dataMap.set("lineageID", parent->dataMap.getInt("lineageID")); // turns out knowing your surname also helps
			for(const auto& sn : scoreNames)
				child->dataMap.set(std::string("parents_") + sn, parent->dataMap.getDouble(sn));
			newPopulation.push_back(child);
		}
	}

	// Printing a summary of what we just done
	writeCompactParetoMessageToStdout();
	// writeDetailedParetoMessageToStdout(population);
	if(logMutationStats && Global::update == Global::updatesPL->get(PT))
		logMutationStatistics();

	logParetoFrontSize(paretoFront);
	if(logLineagesLvl)
		logLineages(population);

	// Incrementing age of everyone involved
	std::set<unsigned> visitedIDs;
	for(auto newOrgPtr : newPopulation) {
		if(visitedIDs.find(newOrgPtr->ID)==visitedIDs.end()) {
			newOrgPtr->dataMap.set("minimizeValue_age", newOrgPtr->dataMap.getDouble("minimizeValue_age")+1.);
			visitedIDs.insert(newOrgPtr->ID);
		}
	}

	// Adding new bloodlines with age of zero
	for(unsigned i=0; i<lineagesToAddNow; i++) {
		if(newPopulation.size()==population.size())
			break;
		auto newOrg = makeNewOrganism();
		newOrg->dataMap.set("minimizeValue_age", 0.);
		newOrg->dataMap.set("lineageID", (int) getNewLineageID());
		newPopulation.push_back(newOrg);
	}

	if(newPopulation.size()!=population.size()) {
		std::cerr << "AFPO population sizes alighnment error: old size " << population.size() << ", new size: " << newPopulation.size() << std::endl;
		exit(EXIT_FAILURE);
	}
}

void AgeFitnessParetoOptimizer::cleanup(std::vector<std::shared_ptr<Organism>>& population) {
//	for(auto sid : survivorIds)
//		std::cout << sid << ", ";
//	std::cout << "survivors were" << std::endl;
	for(unsigned i=0; i<population.size(); i++)
		if(survivorIds.find(population[i]->ID)==survivorIds.end() && population[i]->alive) {
//			std::cout << "killing organism " << population[i]->ID << std::endl;
			population[i]->kill();
		}
	population.swap(newPopulation);
	newPopulation.clear();
}

/************* Private methods *************/

bool AgeFitnessParetoOptimizer::updateParetoRanks(std::vector<std::shared_ptr<Organism>>& population, unsigned currentLevel) {
	std::vector<unsigned> currentRanksIndexes;
	for(unsigned i=0; i<population.size(); i++)
		if(paretoRanks[i]==currentLevel)
			currentRanksIndexes.push_back(i);

	bool wholeRankIsNondominated = true;
	for(unsigned i : currentRanksIndexes)
		for(unsigned j : currentRanksIndexes) {
			if(firstOrganismIsDominatedBySecond(population[i], population[j], scoreNames)) {
				paretoRanks[i]++;
				wholeRankIsNondominated = false;
				break;
			}
		}

	return wholeRankIsNondominated;
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

void AgeFitnessParetoOptimizer::writeCompactParetoMessageToStdout() {
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

	std::cout << "pareto_size=" << paretoFront.size();
	std::cout << " max_age=" << maxAge << "@" << oldestOrganism;
	for(const auto& mvtuple : minValues)
		std::cout << " " << mvtuple.first << "=" << mvtuple.second << "@" << minValueCarriers[mvtuple.first];
	std::cout << " extant_lineages=";

	std::set<int> lineages_set;
	for(const auto& pi : paretoFront)
		lineages_set.insert(pi->dataMap.getInt("lineageID"));
	std::vector<int> lineages;
	for(const auto& lid : lineages_set)
		lineages.push_back(lid);
	std::sort(lineages.begin(), lineages.end());

	for(auto li=lineages.begin(); li!=lineages.end(); li++)
		std::cout << (*li) << (li==lineages.end()-1 ? "" : ",") ;
	std::cout << std::flush;
}

void AgeFitnessParetoOptimizer::writeDetailedParetoMessageToStdout(std::vector<std::shared_ptr<Organism>>& population) {
	// Detailed messages : full population & Pareto front snapshot
	std::cout << std::endl << "Incoming population:" << std::endl;

	for(unsigned i=0; i<population.size(); i++) {
		std::cout << "id" << population[i]->ID << " ";
		std::cout << "lineageid" << population[i]->dataMap.getInt("lineageID") << " ";
		for(auto objname : scoreNames) {
			std::cout << objname << population[i]->dataMap.getDouble(objname) << " ";
		}
		std::cout << "ParetoRank" << paretoRanks[i];
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
}

void AgeFitnessParetoOptimizer::logParetoFrontSize(const std::vector<std::shared_ptr<Organism>>& paretoFront) {
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

void AgeFitnessParetoOptimizer::logLineages(const std::vector<std::shared_ptr<Organism>>& population) {
	if(population.size()!=paretoRanks.size()) {
		std::cerr << "Population size " << population.size() << " is not equal to the Pareto ranks array size " << paretoRanks.size() << ", exiting" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::map<int,std::map<unsigned,nlohmann::json>> lineageJSONs;
	for(unsigned i=0; i<population.size(); i++) {
		if(paretoRanks[i]==0 || logLineagesLvl>1) {
			int lineageID = population.at(i)->dataMap.getInt("lineageID");
			if(lineageJSONs.find(lineageID) == lineageJSONs.end())
				lineageJSONs[lineageID] = std::map<unsigned,nlohmann::json>();
			if(lineageJSONs[lineageID].find(paretoRanks[i]) == lineageJSONs[lineageID].end())
				lineageJSONs[lineageID][paretoRanks[i]] = nlohmann::json::array();
			lineageJSONs[lineageID][paretoRanks[i]].push_back(population.at(i)->getJSONRecord());
		}
	}

	for(const auto& lapair : lineageJSONs) {
		nlohmann::json genDescJSON = nlohmann::json::object();
		genDescJSON["time"] = Global::update;
		for(const auto& prpair : lapair.second)
			genDescJSON[std::string("paretoRank") + std::to_string(prpair.first)] = prpair.second;
		std::string logpath = Global::outputPrefixPL->get() + "lineage" + std::to_string(lapair.first) + ".log";
		std::ofstream lalog;
		lalog.open(logpath, std::ios::app);
		lalog << genDescJSON.dump() << std::endl;
	}
}

void AgeFitnessParetoOptimizer::logMutationStatistics() {
	std::ofstream mutlog;
	for(auto& mutTypePair : mutationStatistics) {
//		mutlog.open(Global::outputPrefixPL->get(PT) + mutTypePair.first + "_gen" + std::to_string(Global::update) + ".log", std::ios::out);
		mutlog.open(Global::outputPrefixPL->get(PT) + mutTypePair.first + ".log", std::ios::out);
		long unsigned numRecords = mutTypePair.second[scoreNames[0]].size();

		mutlog << "#";
		for(const auto& scname : scoreNames) {
			if(scname == "minimizeValue_age")
				continue;
			mutlog << " " << scname << " " << (std::string("delta_") + scname);
		}
		mutlog << std::endl;

		for(long unsigned i=0; i<numRecords; i++) {
			bool firstLine = 0;
			for(const auto& scname : scoreNames) {
				if(scname == "minimizeValue_age")
					continue;
				mutlog << (firstLine++ ? " " : "") << mutTypePair.second[scname][i]
				       << " " << mutTypePair.second[std::string("delta_") + scname][i];
			}
			mutlog << std::endl;
		}

		mutlog.close();
	}
}
