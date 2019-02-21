#pragma once

#include "../../AbstractMentalImage.h"
#include "../Sensors/AbsoluteFocusingSaccadingEyesSensors.h"

class IdentityMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<AbsoluteFocusingSaccadingEyesSensors> sensorsPtr;
	std::vector<double> stateScores;
	std::vector<double> stateRunningScores;
	bool justReset;
	unsigned sensoryChannels;
	unsigned hits;

public:
	IdentityMentalImage(std::shared_ptr<AbsoluteFocusingSaccadingEyesSensors> pointerToSensors);
	void reset(int visualize) override { justReset = true; stateScores.clear(); stateRunningScores.clear(); hits = 0; };
	void resetAfterWorldStateChange(int visualize) override { justReset = true; stateRunningScores.clear(); hits = 0; };
	void updateWithInputs(std::vector<double> inputs) override;

	void recordRunningScoresWithinState(int stateTime, int statePeriod) override;
	void recordRunningScores(std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override {};
	void recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) override;

	int numInputs() override { return sensorsPtr->numOutputs(); };
};
