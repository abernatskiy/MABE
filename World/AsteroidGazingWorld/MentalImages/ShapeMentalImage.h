#pragma once

#include <set>
#include <vector>
#include <string>
#include <map>
#include <utility>

#include "../../AbstractMentalImage.h"
#include "../Utilities/AsteroidsDatasetParser.h"

class ShapeMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<std::string> currentAsteroidNamePtr;
	std::shared_ptr<AsteroidsDatasetParser> datasetParserPtr;

	// Parts for state counting
	unsigned numMatches;
	unsigned numMatchedBits;
	std::set<std::string> stateStrings;
	std::set<std::string> labeledStateStrings;

	// Parts for information-theoretic machinery
	std::map<std::string,unsigned> labelCounts;
	std::map<std::string,unsigned> patternCounts;
	std::map<std::pair<std::string,std::string>,unsigned> jointCounts;
	unsigned numSamples;

	// Auxiliary parts
	long long numErasures;
	std::map<std::string,std::string> labelCache;
	const bool mVisualize;

public:
	ShapeMentalImage(std::shared_ptr<std::string> curAstName,
	                 std::shared_ptr<AsteroidsDatasetParser> dsParser);

	void reset(int visualize) override;
	void resetAfterWorldStateChange(int visualize) override {};

	void updateWithInputs(std::vector<double> inputs) override {};

	void recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) override;
	void recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override {};
	void recordSampleScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) override;

	int numInputs() override { return 0; }; // this MentalImage only works with textures, so it doesn't request any inputs

	void* logTimeSeries(const std::string& label) override;

private:
	void readLabelCache();
	double computeRepellingSharedEntropy();
	double computeFastRepellingSharedEntropy();
};
