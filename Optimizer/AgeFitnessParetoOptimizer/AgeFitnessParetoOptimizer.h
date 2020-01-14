#pragma once

#include <memory>
#include <vector>

#include "../AbstractOptimizer.h"
#include "../../Utilities/MTree.h"

class AgeFitnessParetoOptimizer : public AbstractOptimizer {
public:
	static std::shared_ptr<ParameterLink<std::string>> optimizeFormulasPL;
	static std::shared_ptr<ParameterLink<std::string>> optimizeFormulaNamesPL;
	static std::shared_ptr<ParameterLink<std::string>> maxFileFormulaPL;
	static std::shared_ptr<ParameterLink<int>> logLineagesPL;
	static std::shared_ptr<ParameterLink<bool>> logMutationStatsPL;
	static std::shared_ptr<ParameterLink<double>> lineageAdditionPeriodPL;
	static std::shared_ptr<ParameterLink<int>> selectionTypePL;
	static std::shared_ptr<ParameterLink<int>> tournamentSizePL;
	static std::shared_ptr<ParameterLink<bool>> disableSelectionByAgePL;

	std::vector<std::vector<double>> scores;
	std::vector<std::string> scoreNames;

	std::vector<std::shared_ptr<Organism>> newPopulation;
	std::vector<int> paretoRanks;
	std::vector<double> crowdingMeasure; // within each rank: never compare it without checking for rank sameness first!
	std::vector<std::shared_ptr<Organism>> paretoFront; // the first one is kept separately for convenience

	std::vector<std::shared_ptr<Abstract_MTree>> optimizeFormulasMTs;

	AgeFitnessParetoOptimizer(std::shared_ptr<ParametersTable> PT_);
	void optimize(std::vector<std::shared_ptr<Organism>> &population) override;
	void cleanup(std::vector<std::shared_ptr<Organism>> &population) override; // that's mafioso

private:
	bool firstGenIsNow;
	bool searchIsStuck;
	unsigned logLineagesLvl;
	bool logMutationStats;
	unsigned lineageAdditionPeriod;
	unsigned lineagesPerAddition;
	bool disableLineageAddition;
	unsigned selectionType; // 0 - AFPO-style elite, 1 - Pareto-rank tournament, 2 - NSGA-II-style elite
	unsigned tournamentSize;
	bool selectByAge;

	int maxParetoRank;
	int cutoffParetoRank;
	unsigned long long numEvals;

	std::shared_ptr<Organism> templateOrganism;
	std::set<unsigned> survivorIds;
	std::map<std::string,std::map<std::string,std::vector<double>>> mutationStatistics; // first index is mutation type, second is
	                                                                                    // evaluation name (including deltas)

	void doAFPOStyleSelection(std::vector<std::shared_ptr<Organism>>& population, unsigned newPopSize);
	void doAFPOStyleTournament(std::vector<std::shared_ptr<Organism>>& population, unsigned newPopSize);
	void doNSGAIIStyleSelection(std::vector<std::shared_ptr<Organism>>& population, unsigned newPopSize);
	std::shared_ptr<Organism> makeMutatedOffspringFrom(std::shared_ptr<Organism> parent); // can't use the Organism built-in since there are a few edits to the dataMap that need to be made
	void updateParetoRanks(std::vector<std::shared_ptr<Organism>>& population);
	std::shared_ptr<Organism> makeNewOrganism();
	unsigned getNewLineageID();
	void writeCompactParetoMessageToStdout();
	void writeDetailedParetoMessageToStdout(std::vector<std::shared_ptr<Organism>>& population);
	void logParetoStats();
	void logLineages(const std::vector<std::shared_ptr<Organism>>& population);
	void logMutationStatistics();
	void logNSGAIIData(std::vector<std::shared_ptr<Organism>>& population);
};
