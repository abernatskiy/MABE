#include <limits>
#include <math.h>
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
	"if on DEMarkovBrains, set to true to log the statistics of mutations, default: false");

std::shared_ptr<ParameterLink<double>> AgeFitnessParetoOptimizer::lineageAdditionPeriodPL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-lineageAdditionPeriod",
	1.0,
	"how frequently new lineages should be added (p>1 - a lineage added once every floor(p) gens,\n"
	"0<p<1 - floor(1/p) lineages added every generation, p<0 - no lineages added except to the initial population), default: 1.0");

std::shared_ptr<ParameterLink<int>> AgeFitnessParetoOptimizer::selectionTypePL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-selectionType",
	0,
	"selection type, new population will be composed of: 0 (default, AFPO-style) - top-1 Pareto front + its offspring, "
	"1 (rank tournament) - results of popsize/tournsize tournaments where the losers get replaced by the winner's children, "
	"2 (NSGAII-style) - population gets collapsed twofold elitistically, then expanded with tournaments");

std::shared_ptr<ParameterLink<int>> AgeFitnessParetoOptimizer::tournamentSizePL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-tournamentSize",
	2,
	"size of the tournament if tournament or nsgaii-style selection is used, default: 2");

std::shared_ptr<ParameterLink<bool>> AgeFitnessParetoOptimizer::disableSelectionByAgePL =
Parameters::register_parameter("OPTIMIZER_AGEFITNESSPARETO-disableSelectionByAge",
	false,
	"if set to true, Pareto-optimization for age will be disabled; it will still be available for use in compound objectives as minimizeValue_age");

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
	selectionType(selectionTypePL->get(PT_)),
	tournamentSize(tournamentSizePL->get(PT_)),
	selectByAge(!disableSelectionByAgePL->get(PT_)),
	maxParetoRank(-1), numEvals(0) {

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
	if(selectByAge)
		scoreNames.push_back("minimizeValue_age");

	popFileColumns.clear();
	for (auto &name : scoreNames)
		popFileColumns.push_back(name);

	optimizeFormula = stringToMTree(maxFileFormulaPL->get(PT)); // from the parent class

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

	// initializing the mutation statistics storage
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
	if(searchIsStuck) return;

	// Assigning age to all initial organisms, and
	// preserving a copy of one of the progenitors
	// to serve as a template for progenitors of new lineages, and
	// increasing the number of evaluations by the initial population size
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

		numEvals += population.size();

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

	updateParetoRanks(population);

	paretoFront.clear();
	for(unsigned i=0; i<population.size(); i++)
		if(paretoRanks[i] == 0)
			paretoFront.push_back(population[i]);

	if(paretoFront.size()==0) {
		std::cerr << "AgeFitnessParetoOptimizer: Pareto front is empty! Something is clearly wrong with the objectives, exiting" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Computing how many new lineages we should add
	unsigned lineagesToAddNow = (Global::update % lineageAdditionPeriod == 0) ? lineagesPerAddition : 0;
	survivorIds.clear();

	// Each of these functions must provide:
	// 1. newPopulation
	// 2. survivorIDs - a vector of IDs of all the individuals in the old population that should not be killed by cleanup()
	// 3. an appropriate increment of numEvals
	if(selectionType==0)
		doAFPOStyleSelection(population, population.size()-lineagesToAddNow);
	else if(selectionType==1)
		doAFPOStyleTournament(population, population.size()-lineagesToAddNow);
	else if(selectionType==2)
		doNSGAIIStyleSelection(population, population.size()-lineagesToAddNow);
	else {
		std::cerr << "AgeFitnessParetoOptimizer: unrecognized section type " << selectionType << std::endl;
		exit(EXIT_FAILURE);
	}

	/**********************************************/

	// Printing a summary of what we just done
	writeCompactParetoMessageToStdout();
	// writeDetailedParetoMessageToStdout(population);
	if(logMutationStats && Global::update == Global::updatesPL->get(PT))
		logMutationStatistics();

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
		numEvals++;
	}

	logParetoStats(); // here because it logs numEvals

	if(newPopulation.size()!=population.size()) {
		std::cerr << "AFPO population sizes alighnment error: old size " << population.size() << ", new size: " << newPopulation.size() << std::endl;
		exit(EXIT_FAILURE);
	}
}

void AgeFitnessParetoOptimizer::cleanup(std::vector<std::shared_ptr<Organism>>& population) {
//	for(auto sid : survivorIds)
//		std::cout << sid << ", ";
//	std::cout << "survivors were" << std::endl;
	if(!searchIsStuck) {
		for(unsigned i=0; i<population.size(); i++) {
			if(survivorIds.find(population[i]->ID)==survivorIds.end() && population[i]->alive) {
//				std::cout << "killing organism " << population[i]->ID << std::endl;
				population[i]->kill();
			}
		}
	}
	population.swap(newPopulation);
	newPopulation.clear();
}

/************* Private methods *************/

void AgeFitnessParetoOptimizer::doAFPOStyleSelection(std::vector<std::shared_ptr<Organism>>& population, unsigned newPopSize) {
	// Checking if the new population will fit into the size of the old one
	if(paretoFront.size() >= newPopSize)
		std::cout << "WARNING: Pareto elite is so large that there is not enough space for variation" << std::endl;
	if(paretoFront.size() == population.size())
		searchIsStuck = true;

	// Copying the Pareto front to it - all of it is the elite
	newPopulation.insert(newPopulation.end(), paretoFront.begin(), paretoFront.end());
	for(unsigned i=0; i<paretoRanks.size(); i++)
		if(paretoRanks[i]==0)
			survivorIds.insert(population[i]->ID);

	// Adding the offspring of the Pareto front organisms to the population
	// until it reaches the projected size of the population minus one
	while(newPopulation.size() < newPopSize) {
		std::shared_ptr<Organism> parent = paretoFront[Random::getIndex(paretoFront.size())];
		newPopulation.push_back(makeMutatedOffspringFrom(parent));
		numEvals++;
	}
}

void AgeFitnessParetoOptimizer::doAFPOStyleTournament(std::vector<std::shared_ptr<Organism>>& population, unsigned newPopSize) {
	while(newPopulation.size() < newPopSize) {
		// finding the champion
		unsigned championIdx = Random::getIndex(population.size());
		std::vector<unsigned> participants = { championIdx };
		for(unsigned i=0; i<tournamentSize-1; i++) {
			unsigned challengerIdx = championIdx;
			while( std::find(participants.begin(), participants.end(), challengerIdx)!=participants.end() )
				challengerIdx = Random::getIndex(population.size());
			participants.push_back(challengerIdx);

			if(paretoRanks[championIdx]>=paretoRanks[challengerIdx]) // no tie-breaking - the newbie takes the champion's place if it has the same rank
				championIdx = challengerIdx;
		}

		// then copying it and its offspring into the next population
		std::shared_ptr<Organism> champion = population[championIdx];
		newPopulation.push_back(champion); // champion can be inserted into the new population multiple times: a potential for infinite Pareto rank growth (~ to the size of the neutral networks)
		survivorIds.insert(champion->ID);

		for(unsigned i=0; i<tournamentSize-1; i++) {
			if(newPopulation.size()==newPopSize)
				break;
			newPopulation.push_back(makeMutatedOffspringFrom(champion));
			numEvals++;
		}
	}
}

void AgeFitnessParetoOptimizer::doNSGAIIStyleSelection(std::vector<std::shared_ptr<Organism>>& population, unsigned newPopSize) {

	// first we need to explicitly parse the population into Pareto ranks
	std::vector<std::vector<size_t>> ranks(maxParetoRank+1);
	for(unsigned i=0; i<population.size(); i++)
		ranks[paretoRanks[i]].push_back(i);

	// then, for each objective, we need to sort each rank according to the objective value and measure the cuboid side
	// the sum of cuboid sides is the crowding measure
	crowdingMeasure.resize(population.size());
	std::fill(crowdingMeasure.begin(), crowdingMeasure.end(), std::numeric_limits<double>::infinity());
	for(const auto& attrname : scoreNames) {
		auto attrcompfun = [attrname, population] (size_t lefti, size_t righti) {
			return population[lefti]->dataMap.getDouble(attrname) == population[righti]->dataMap.getDouble(attrname) ?
			       population[lefti]->ID > population[righti]->ID :
			       population[lefti]->dataMap.getDouble(attrname) < population[righti]->dataMap.getDouble(attrname);
		};
		for(unsigned r=0; r<=maxParetoRank; r++) {
			std::sort(ranks[r].begin(), ranks[r].end(), attrcompfun);
			double range = population[ranks[r].back()]->dataMap.getDouble(attrname) - population[ranks[r].front()]->dataMap.getDouble(attrname);
			for(unsigned ri=1; ri<ranks[r].size()-1; ri++) {
				double side = population[ranks[r][ri+1]]->dataMap.getDouble(attrname) - population[ranks[r][ri-1]]->dataMap.getDouble(attrname);
				side = side==0 ? 0 : side/range;
				crowdingMeasure[ranks[r][ri]] = isinf(crowdingMeasure[ranks[r][ri]]) ? side : crowdingMeasure[ranks[r][ri]]+side;
			}
		}
	}

	// we then sort each rank according to the crowding measure and fill up 1/2 of the population with our newly acquired elite
	unsigned eliteSize = population.size() / 2;
	std::vector<size_t> eliteIndexes;
	for(unsigned r=0; r<=maxParetoRank && newPopulation.size()<eliteSize; r++) {
		std::sort(ranks[r].begin(), ranks[r].end(), [this](size_t l, size_t r){ return crowdingMeasure[l] > crowdingMeasure[r]; });
		for(size_t i : ranks[r]) {
			newPopulation.push_back(population[i]);
			survivorIds.insert(population[i]->ID);
			eliteIndexes.push_back(i);
			if(newPopulation.size()>=eliteSize)
				break;
		}
	}

	// finally, we fill up the rest of the population with offspring of the elite members selected through tournament
	while(newPopulation.size()<newPopSize) {

		// finding the champion
		unsigned championIdx = Random::getIndex(eliteSize);
		std::vector<unsigned> participants = { championIdx };
		for(unsigned i=0; i<tournamentSize-1; i++) {
			unsigned challengerIdx = championIdx;
			while( std::find(participants.begin(), participants.end(), challengerIdx)!=participants.end() )
				challengerIdx = Random::getIndex(eliteSize);
			participants.push_back(challengerIdx);

			if(paretoRanks[eliteIndexes[championIdx]]>paretoRanks[eliteIndexes[challengerIdx]] ||
			   (paretoRanks[eliteIndexes[championIdx]]==paretoRanks[eliteIndexes[challengerIdx]] &&
			    crowdingMeasure[eliteIndexes[championIdx]]<crowdingMeasure[eliteIndexes[challengerIdx]]))
				championIdx = challengerIdx;
		}

		// then copying its offspring into the next population
		std::shared_ptr<Organism> champion = population[eliteIndexes[championIdx]];
		for(unsigned i=0; i<tournamentSize; i++) {
			newPopulation.push_back(makeMutatedOffspringFrom(champion));
			numEvals++;
			if(newPopulation.size()==newPopSize)
				break;
		}
	}
}

std::shared_ptr<Organism> AgeFitnessParetoOptimizer::makeMutatedOffspringFrom(std::shared_ptr<Organism> parent) {
	std::shared_ptr<Organism> child = parent->makeMutatedOffspringFrom(parent);
	child->dataMap.set("minimizeValue_age", parent->dataMap.getDouble("minimizeValue_age")); // it is very important to know how farback your ancestry goes
	child->dataMap.set("lineageID", parent->dataMap.getInt("lineageID")); // turns out knowing your surname also helps
	for(const auto& sn : scoreNames)
		child->dataMap.set(std::string("parents_") + sn, parent->dataMap.getDouble(sn));
	return child;
}

void AgeFitnessParetoOptimizer::updateParetoRanks(std::vector<std::shared_ptr<Organism>>& population) {

	// NSGA-II-style Pareto rank sorting

	std::vector<size_t> dominationCount(population.size(), 0); // number of solutions that dominate population[i]
	std::vector<std::vector<size_t>> indivsDominatedBy(population.size()); // set of solution dominated by population[i]

	for(size_t i=0; i<population.size(); i++) {
		for(size_t j=0; j<population.size(); j++) {
			if(i!=j && firstOrganismIsDominatedBySecond(population[i], population[j], scoreNames)) {
				dominationCount[i]++;
				indivsDominatedBy[j].push_back(i);
			}
		}
	}

	paretoRanks.resize(population.size());
	std::fill(paretoRanks.begin(), paretoRanks.end(), -1);
	int currentParetoRank = 0;
	bool populationIsRanked = false;
	while(!populationIsRanked) {
		populationIsRanked = true;
		for(size_t i=0; i<population.size(); i++)
			if(paretoRanks[i]==-1 && dominationCount[i]==0)
				paretoRanks[i] = currentParetoRank;
		for(size_t i=0; i<population.size(); i++)
			if(paretoRanks[i]==currentParetoRank) {
				for(size_t j : indivsDominatedBy[i]) {
					dominationCount[j]--;
					populationIsRanked = false;
				}
			}
		currentParetoRank++;
	}
	maxParetoRank = currentParetoRank - 1;
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
	for(auto it=scoreNames.begin(); it!=scoreNames.end(); it++) {
		if((*it)!="minimizeValue_age") {
			minValues[*it] = std::numeric_limits<double>::max();
			minValueCarriers[*it] = -1;
		}
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
	std::cout << " max_rank=" << maxParetoRank;
	if(selectByAge) std::cout << " max_age=" << maxAge << "@" << oldestOrganism;
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

	unsigned numLineages = 0;
	for(auto li=lineages.begin(); li!=lineages.end(); li++) {
		if(numLineages>10) {
			std::cout << "...";
			break;
		}
		std::cout << (*li) << (li==lineages.end()-1 ? "" : ",");
		numLineages++;
	}
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

void AgeFitnessParetoOptimizer::logParetoStats() {
	std::string logpath = Global::outputPrefixPL->get() + "paretoFront.log";
	std::ofstream pflog;

	static bool firstRun = true;
	if(firstRun) {
		pflog.open(logpath, std::ios::out);
		pflog.close();
		firstRun = false;
	}

	pflog.open(logpath, std::ios::app);
	pflog << paretoFront.size() << " " << maxParetoRank << " " << numEvals << std::endl;
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
