#pragma once

#include <vector>

#include "../Utilities/Data.h"
#include "../Organism/Organism.h"

class AbstractMentalImage {

public:
	virtual void reset(int visualize) = 0;
	virtual void resetAfterWorldStateChange(int visualize) {}; // only used when discrete state changes are employed
	virtual void updateWithInputs(std::vector<double> inputs) = 0;
	virtual void recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int stateChangePeriod) {}; // only used when discrete state changes are employed
	virtual void recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) = 0;
	virtual void recordSampleScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) = 0;
	virtual void evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) = 0;
	virtual int numInputs() = 0;
};
