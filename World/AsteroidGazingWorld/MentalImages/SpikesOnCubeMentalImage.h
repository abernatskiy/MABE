#pragma once

#include <map>
#include <vector>

#include "../../AbstractMentalImage.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "ann.h"

#define ANN_INPUT_SIZE 5 // one input per each ALSD language statement field
#define ANN_HIDDEN_SIZE 10
#define ANN_OUTPUT_SIZE 5

#include "commandLogger.h"

typedef std::tuple<unsigned,unsigned,unsigned> CommandType; // location of the spike is determined by three numbers

class SpikesOnCubeMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<std::string> currentAsteroidNamePtr;
	std::shared_ptr<AsteroidsDatasetParser> datasetParserPtr;

	std::vector<CommandType> originalCommands;
	std::vector<CommandType> currentCommands;
	std::vector<bool> ocApproximationAttempted;

	std::vector<double> stateScores;
	std::vector<unsigned> correctCommandsStateScores;

	const unsigned k = 2;
	const unsigned q = 1<<k;

	// "One-hot" encoding version
	const unsigned bitsForFace = 6;
	const unsigned bitsForCoordinate = q+1;

	// Standard positional encoding version
	//const unsigned bitsForFace = 3;
	//const unsigned bitsForCoordinate = k+1;

	bool justReset;

	ArtificialNeuralNetwork helperANN;

	CommandLogger cl;
	bool mVisualize;

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
	void readHelperANN();

	inline double propEnc(unsigned val, unsigned max) { return static_cast<double>(val)/static_cast<double>(max); };
	inline std::vector<double> encodeStatement(const CommandType& st);

	double commandDivergence(const CommandType& from, const CommandType& to);
	double maxCommandDivergence();
	double evaluateCommand(const CommandType& c);
};
