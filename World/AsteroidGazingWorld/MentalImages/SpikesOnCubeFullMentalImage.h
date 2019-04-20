#pragma once

#include <map>
#include <vector>

#include "SpikesOnCubeMentalImage.h"
#include "../Sensors/AbsoluteFocusingSaccadingEyesSensors.h"

typedef std::tuple<std::vector<unsigned>> CommandRangeType;

class SpikesOnCubeFullMentalImage : public SpikesOnCubeMentalImage {

private:
	std::vector<CommandRangeType> currentCommandRanges;
	std::vector<std::vector<CommandRangeType>> commandRangesTS;

public:
	SpikesOnCubeFullMentalImage(std::shared_ptr<std::string> curAstName,
	                            std::shared_ptr<AsteroidsDatasetParser> dsParser,
	                            std::shared_ptr<AbsoluteFocusingSaccadingEyesSensors> sPtr,
	                            unsigned numTriggerBits,
	                            bool integrateFitness);

	void reset(int visualize) override;
	void resetAfterWorldStateChange(int visualize) override;

	void updateWithInputs(std::vector<double> inputs) override;

	void recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) override;

	int numInputs() override;

	void* logTimeSeries(const std::string& label) override;

private:
	std::tuple<double,bool> evaluateRangeVSSet(const CommandRangeType& guessesRange);
	std::shared_ptr<AbsoluteFocusingSaccadingEyesSensors> sensorsPtr;

	unsigned numTriggerBits;
	bool integrateFitness;

	bool answerGiven;
	bool answerReceived;
};
