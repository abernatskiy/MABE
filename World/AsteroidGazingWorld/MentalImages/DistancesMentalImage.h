#pragma once

#include <set>
#include <vector>
#include <string>
#include <map>
#include <utility>

#include "../../AbstractMentalImage.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../../AbstractSensors.h"

class DistancesMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<std::string> currentAsteroidNamePtr;
	std::shared_ptr<AsteroidsDatasetParser> datasetParserPtr;
	std::shared_ptr<AbstractSensors> sensorsPtr;

	std::vector<double> curBits;
	//std::string curStateString;
	std::string curLabelString;

	// Parts for state counting
	//std::vector<std::string> stateStrings;
	//std::vector<std::string> labelStrings;
	//std::vector<std::string> labeledStateStrings;
	double totalCrossLabelDistance;
	double totalIntraLabelDistance;

	// Parts for information-theoretic machinery
	std::vector<std::pair<unsigned,unsigned>> infoRanges;
	std::map<std::string,unsigned> labelCounts;
	std::vector<std::map<std::string,unsigned>> rangesPatternCounts;
	std::vector<std::map<std::pair<std::string,std::string>,unsigned>> rangesJointCounts; // vector of (label, pattern) -> count, a different convention than the one used in CompressedMentalImage
	unsigned numSamples;

	// Auxiliary parts
	std::vector<double> sensorActivityStateScores;

	const bool mVisualize;

	const unsigned numBits;

	const bool overwriteEvaluations;
	void updateOrgDatamap(std::shared_ptr<Organism> org, std::string entryName, double entryValue);

public:
	DistancesMentalImage(std::shared_ptr<std::string> curAstName,
	                     std::shared_ptr<AsteroidsDatasetParser> dsParser,
	                     std::shared_ptr<AbstractSensors> sPtr,
	                     bool overwriteEvaluations);

	void reset(int visualize) override;
	void resetAfterWorldStateChange(int visualize) override;

	void updateWithInputs(std::vector<double> inputs) override;

	void recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) override;
	void recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void recordSampleScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) override;

	int numInputs() override;

	void* logTimeSeries(const std::string& label) override;

private:
	void readLabel();
	void updateDistanceStats();
};
