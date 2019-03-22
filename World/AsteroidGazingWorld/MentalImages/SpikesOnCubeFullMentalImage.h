#pragma once

#include <map>
#include <vector>

#include "SpikesOnCubeMentalImage.h"

typedef std::tuple<std::vector<unsigned>,std::vector<unsigned>,std::vector<unsigned>> CommandRangeType;

class SpikesOnCubeFullMentalImage : public SpikesOnCubeMentalImage {

private:
	const unsigned lBitsForFace = 6*1;
	const unsigned lBitsForCoordinate = 1*(q-1); // again, edge spikes are disallowed and the coordinate can actually take up to q-1 values

	std::vector<CommandRangeType> currentCommandRanges;

public:
	SpikesOnCubeFullMentalImage(std::shared_ptr<std::string> curAstName, std::shared_ptr<AsteroidsDatasetParser> dsParser);

	void reset(int visualize) override;
	void resetAfterWorldStateChange(int visualize) override;

	void updateWithInputs(std::vector<double> inputs) override;

	void recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) override;

	int numInputs() override;

private:
	std::tuple<double,bool> evaluateRangeVSSet(const CommandRangeType& guessesRange);
};
