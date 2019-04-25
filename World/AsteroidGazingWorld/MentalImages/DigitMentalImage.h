#pragma once

#include <map>
#include <vector>

#include "../../AbstractMentalImage.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../Sensors/AbsoluteFocusingSaccadingEyesSensors.h"
#include "commandLogger.h"

typedef std::tuple<unsigned> CommandType; // digit is determined by a single number
typedef std::tuple<std::vector<unsigned>> CommandRangeType;

class DigitMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<std::string> currentAsteroidNamePtr;
	std::shared_ptr<AsteroidsDatasetParser> datasetParserPtr;

	std::vector<CommandType> originalCommands;
	std::vector<bool> ocApproximationAttempted;

	std::vector<double> stateScores;
	std::vector<unsigned> correctCommandsStateScores;
	std::vector<double> sensorActivityStateScores;

	const unsigned mnistNumDigits = 10;
	const unsigned mnistNumBits = 2*mnistNumDigits;

	bool justReset;

	CommandLogger cl;
	bool mVisualize;

	std::map<unsigned,std::vector<unsigned>> lineageToEvaluationOrder;

	std::vector<CommandRangeType> currentCommandRanges;
	std::vector<std::vector<CommandRangeType>> commandRangesTS;

	std::shared_ptr<AbsoluteFocusingSaccadingEyesSensors> sensorsPtr;

	unsigned numTriggerBits;
	bool integrateFitness;

	bool answerGiven;
	bool answerReceived;

public:
	DigitMentalImage(std::shared_ptr<std::string> curAstName,
	                 std::shared_ptr<AsteroidsDatasetParser> dsParser,
	                 std::shared_ptr<AbsoluteFocusingSaccadingEyesSensors> sPtr,
	                 unsigned numTriggerBits,
	                 bool integrateFitness);

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
	void readOriginalCommands();

	const std::vector<unsigned>& getEvaluationOrder(unsigned lineageID, unsigned numAsteroids); // NOTE: all this random evaluation order idea can be removed if it's never needed in the future

	std::tuple<double,bool> evaluateRangeVSSet(const CommandRangeType& guessesRange);
};
