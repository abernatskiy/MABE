#pragma once

#include <map>
#include <vector>

#include "../../AbstractMentalImage.h"
#include "../Utilities/AsteroidsDatasetParser.h"

typedef std::tuple<unsigned,unsigned,unsigned> CommandType; // location of the spike is determined by three numbers

class SpikesOnCubeMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<std::string> currentAsteroidNamePtr;
	std::shared_ptr<AsteroidsDatasetParser> datasetParserPtr;
	std::set<CommandType> currentCommands;
	std::set<CommandType> originalCommands;
	std::vector<unsigned> stateScores;

	const unsigned q = 8;
	const unsigned bitsForFace = 6;
	const unsigned bitsForCoordinate = q+1;

	bool justReset;

public:
	SpikesOnCubeMentalImage(std::shared_ptr<std::string> curAstName, std::shared_ptr<AsteroidsDatasetParser> dsParser);

	void reset(int visualize) override;
	void resetAfterWorldStateChange(int visualize) override;

	void updateWithInputs(std::vector<double> inputs) override;

	void recordRunningScoresWithinState(int stateTime, int statePeriod) override;
	void recordRunningScores(std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) override;

	int numInputs() override;

private:
	void readOriginalCommands();
};
