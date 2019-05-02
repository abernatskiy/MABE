#pragma once

#include "../AbstractBrain.h"
#include "../MarkovBrain/MarkovBrain.h"

class DerivedMarkovBrain : public AbstractBrain {
private:
	MarkovBrain mb;

	std::vector<std::shared_ptr<AbstractGate>> getCopiesOfAllGates() const;

public:
	DerivedMarkovBrain(int ins, int outs, std::shared_ptr<ParametersTable> PT);

	DerivedMarkovBrain(const DerivedMarkovBrain& original); // it's really private tbqh

	void logNote(std::string note) override { mb.logNote(note); };

	void update() override;
	DataMap getStats(std::string& prefix) override;
	std::string description() override;
	void resetBrain() override;
	std::shared_ptr<AbstractBrain> makeBrain(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& genomes) override;
	std::shared_ptr<AbstractBrain> makeCopy(std::shared_ptr<ParametersTable> PT) override;
	void initializeGenomes(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& genomes) override;
};

inline std::shared_ptr<AbstractBrain> DerivedMarkovBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT) {
	return std::make_shared<DerivedMarkovBrain>(ins, outs, PT);
}
