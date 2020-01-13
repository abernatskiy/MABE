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
	static std::shared_ptr<ParameterLink<bool>> useTournamentSelectionPL;
	static std::shared_ptr<ParameterLink<int>> tournamentSizePL;
	static std::shared_ptr<ParameterLink<bool>> disableSelectionByAgePL;

	std::vector<std::vector<double>> scores;
	std::vector<std::string> scoreNames;

	std::vector<std::shared_ptr<Organism>> newPopulation;
	std::vector<int> paretoRanks;
	std::vector<std::shared_ptr<Organism>> paretoFront;

	int maxParetoRank;

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
	bool useTournamentSelection;
	unsigned tournamentSize;
	bool selectByAge;
	std::shared_ptr<Organism> templateOrganism;
	std::set<unsigned> survivorIds;
	std::map<std::string,std::map<std::string,std::vector<double>>> mutationStatistics; // first index is mutation type, second is
	                                                                                    // evaluation name (including deltas)

	void updateParetoRanks(std::vector<std::shared_ptr<Organism>>& population);
	std::shared_ptr<Organism> makeNewOrganism();
	unsigned getNewLineageID();
	void writeCompactParetoMessageToStdout();
	void writeDetailedParetoMessageToStdout(std::vector<std::shared_ptr<Organism>>& population);
	void logParetoFrontSize(const std::vector<std::shared_ptr<Organism>>& paretoFront);
	void logLineages(const std::vector<std::shared_ptr<Organism>>& population);
	void logMutationStatistics();
};
