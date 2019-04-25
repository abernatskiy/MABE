#pragma once

#include <map>
#include <vector>

#include "../../AbstractMentalImage.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "ann.h"

#define ANN_INPUT_SIZE 5 // one input per each ALSD language statement field
#define ANN_HIDDEN_SIZE 10
#define ANN_OUTPUT_SIZE 5

#include "spikesCommandLogger.h"

typedef std::tuple<unsigned,unsigned,unsigned> CommandType; // location of the spike is determined by three numbers

void printCommand(CommandType com);
void printCommandsVector(std::vector<CommandType> commands);

class SpikesOnCubeMentalImage : public AbstractMentalImage {

protected:
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

	std::map<unsigned,std::vector<unsigned>> lineageToEvaluationOrder;

public:
	SpikesOnCubeMentalImage(std::shared_ptr<std::string> curAstName, std::shared_ptr<AsteroidsDatasetParser> dsParser);

	virtual void reset(int visualize) override;
	virtual void resetAfterWorldStateChange(int visualize) override;

	virtual void updateWithInputs(std::vector<double> inputs) override;

	virtual void recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) override;
	void recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void recordSampleScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) override;

	virtual int numInputs() override;

protected:
	void readOriginalCommands();
	void readHelperANN();

	inline double propEnc(unsigned val, unsigned max) { return static_cast<double>(val)/static_cast<double>(max); };
	inline std::vector<double> encodeStatement(const CommandType& st);

	double commandDivergence(const CommandType& from, const CommandType& to);
	double maxCommandDivergence();
	double evaluateCommand(const CommandType& c);

	const std::vector<unsigned>& getEvaluationOrder(unsigned lineageID, unsigned numAsteroids);
};
