#pragma once

#include <set>
#include <vector>
#include <string>
#include <map>
#include <utility>

#include "../../AbstractMentalImage.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../../AbstractSensors.h"
#include "../../../Utilities/hngen/hngen.h" // enumerator of spheres in Hamming space
#include "labeledHNNG.h" // purpose-built fast Hamming neighborhood explorer that uses multi-index hashing

class CompressedMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<std::string> currentAsteroidNamePtr;
	std::shared_ptr<AsteroidsDatasetParser> datasetParserPtr;
	std::shared_ptr<AbstractSensors> sensorsPtr;

	std::string curStateString;
	std::string curLabelString;

	// Parts for state counting
	std::set<std::string> stateStrings;
	std::set<std::string> labeledStateStrings;
	long unsigned lostStates;
	long unsigned lostLabels;

	// Parts for information-theoretic machinery
	std::map<std::string,unsigned> labelCounts;
	std::map<std::string,unsigned> patternCounts;
	std::map<std::pair<std::string,std::string>,unsigned> jointCounts;
	unsigned numSamples;

	// Auxiliary parts
	std::vector<double> sensorActivityStateScores;

	const bool mVisualize;

	const unsigned numBits;

	HammingNeighborhoodGenerator hngen;

	LabeledHNNG neighborsdb;

	//// Stuff that might become useful in the future
	// std::vector<std::string> stateTS;
	// unsigned numTriggerBits;
	// bool requireTriggering;
	// bool integrateFitness;
	// bool answerGiven;
	// bool answerReceived;

public:
	CompressedMentalImage(std::shared_ptr<std::string> curAstName,
	                      std::shared_ptr<AsteroidsDatasetParser> dsParser,
	                      std::shared_ptr<AbstractSensors> sPtr,
	                      unsigned numBits);

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
	double computeFastRepellingSharedEntropy();
};
