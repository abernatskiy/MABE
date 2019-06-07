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
	static std::shared_ptr<ParameterLink<bool>> logLineagesPL;

	std::vector<std::vector<double>> scores;
	std::vector<std::string> scoreNames;

	std::vector<std::shared_ptr<Organism>> newPopulation;
	std::vector<unsigned> paretoOrder;
	std::vector<std::shared_ptr<Organism>> paretoFront;

	std::vector<std::shared_ptr<Abstract_MTree>> optimizeFormulasMTs;

	AgeFitnessParetoOptimizer(std::shared_ptr<ParametersTable> PT_);
	void optimize(std::vector<std::shared_ptr<Organism>> &population) override;
	void cleanup(std::vector<std::shared_ptr<Organism>> &population) override; // that's mafioso

private:
	bool firstGenIsNow;
	bool logLineages;
	std::shared_ptr<Organism> templateOrganism;
	std::shared_ptr<Organism> makeNewOrganism();
	unsigned getNewLineageID();
	void logParetoFrontSize(const std::vector<std::shared_ptr<Organism>>& paretoFront);
	void logParetoFrontLineages(const std::vector<std::shared_ptr<Organism>>& paretoFront);
};
