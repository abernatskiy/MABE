#pragma once

#include <map>
#include <vector>

#include "../../AbstractMentalImage.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "ann.h"

#define ANN_INPUT_SIZE 10 // five inputs for each of the two AL statements

#ifndef ANN_HIDDEN_SIZE
#define ANN_HIDDEN_SIZE 20
#endif // ANN_HIDDEN_SIZE

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

	const unsigned q = 8;

	const unsigned bitsForFace = 6;
	const unsigned bitsForCoordinate = q+1;

	bool justReset;

	ArtificialNeuralNetwork helperANN;

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
	inline void encodeStatement(const CommandType& st, std::vector<double>& stor, unsigned shiftBy=0);
	inline std::vector<double> encodeStatementPair(const CommandType& lhs, const CommandType& rhs);

	double commandDivergence(const CommandType& from, const CommandType& to);
	double evaluateCommand(const CommandType& c);
};
