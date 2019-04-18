#include "SpikesOnCubeMentalImage.h"
#include "decoders.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdlib>

void printCommand(CommandType com) {
	unsigned d;
	std::tie(d) = com;
	std::cout << d;
}

void printCommandsVector(std::vector<CommandType> commands) {
	for(auto coords : commands) {
		std::cout << ' '; printCommand(coords); std::cout << ';';
	}
}

/***** Public SpikesOnCubeMentalImage class definitions *****/

SpikesOnCubeMentalImage::SpikesOnCubeMentalImage(std::shared_ptr<std::string> curAstNamePtr,
	                                               std::shared_ptr<AsteroidsDatasetParser> dsParserPtr) :
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	justReset(true),
	helperANN({ANN_INPUT_SIZE, ANN_HIDDEN_SIZE, ANN_OUTPUT_SIZE}),
	cl(Global::outputPrefixPL->get() + "commands.log"),
	mVisualize(Global::modePL->get() == "visualize") {

	readHelperANN();
	if(mVisualize)
		cl.open();
}

void SpikesOnCubeMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	stateScores.clear();
	correctCommandsStateScores.clear();
	sensorActivityStateScores.clear();
	currentCommands.clear();
	// originalCommands are taken care of in readOriginalCommands()
	justReset = true;
}

void SpikesOnCubeMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
	currentCommands.clear();
	// originalCommands are taken care of in readOriginalCommands()
	justReset = true;

	//if(mVisualize)
	//	cl.logMessage("resetAfterWorldStateChange called");
}

void SpikesOnCubeMentalImage::updateWithInputs(std::vector<double> inputs) {
	if(justReset) {
		justReset = false;
		return;
	}

	std::cerr << "updateWithInputs called from SpikesOnCubeMentalImage - should never happen in this branch" << std::endl;
	exit(EXIT_FAILURE);
}

void SpikesOnCubeMentalImage::recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) {
	if(stateTime == statePeriod-1) {

		readOriginalCommands();
		unsigned numOriginalCommands = originalCommands.size();

		ocApproximationAttempted.resize(numOriginalCommands);
		std::fill(ocApproximationAttempted.begin(), ocApproximationAttempted.end(), false);

		// Only the last numOriginalCommands outputs of the brain count
		unsigned numCorrectCommands = 0;
		for(auto it=originalCommands.begin(); it!=originalCommands.end(); it++)
			if(std::find(currentCommands.end()-numOriginalCommands, currentCommands.end(), *it) != currentCommands.end())
				numCorrectCommands++;
		correctCommandsStateScores.push_back(numCorrectCommands);

		double cumulativeDivergence = 0.;
		for(auto it=currentCommands.end()-numOriginalCommands; it!=currentCommands.end(); it++)
			cumulativeDivergence += evaluateCommand(*it);
		stateScores.push_back(cumulativeDivergence);

		std::vector<CommandType> lastCommands(currentCommands.end()-numOriginalCommands, currentCommands.end());

		if(mVisualize) cl.logMapping(originalCommands, std::vector<std::vector<CommandType>>({lastCommands}));

		if(mVisualize) {
			std::cout << "Brain generated commands for " << *currentAsteroidNamePtr  << ":" << std::endl;
			for(auto it=currentCommands.end()-numOriginalCommands; it!=currentCommands.end(); it++) {
				printCommand(*it);
				std::cout << std::endl;
			}
			std::cout << std::endl;

			std::cout << "Added " << cumulativeDivergence << " to evaluations of " << *currentAsteroidNamePtr << std::endl;
			std::cout << "Full evaluations:";
			for(auto sc : correctCommandsStateScores)
				std::cout << ' ' << sc;
			std::cout << std::endl;
		}
	}
}

void SpikesOnCubeMentalImage::recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void SpikesOnCubeMentalImage::recordSampleScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
	unsigned lineageID = org->dataMap.findKeyInData("lineageID")!=0 ? org->dataMap.getInt("lineageID") : org->ID;
	const auto& evaluationOrder = getEvaluationOrder(lineageID, stateScores.size());

//	std::cout << "id " << org->ID << " lineage " << lineageID << ":";
//	for(const auto& evalIdx : evaluationOrder)
//		std::cout << " " << evalIdx;
//	std::cout << std::endl;

//	std::cout << "id " << org->ID << " lineage " << lineageID << std::endl;
//	std::cout << "asteroid scores:" << std::setprecision(2);
//	for(const auto& astSc : stateScores)
//		std::cout << " " << astSc;
//	std::cout << std::endl;
//	std::cout << "max correct commands:";
//	for(const auto& ncc : correctCommandsStateScores)
//		std::cout << " " << ncc;
//	std::cout << std::endl;

	unsigned ncc = 0;
	double totSensoryActivity = 0;
	const unsigned astsTotal = stateScores.size();
	double score = 0;
	for(unsigned i=0; i<astsTotal; i++) {
		unsigned curAstIdx = evaluationOrder[i];
		double stsc = stateScores[curAstIdx];
//		std::cout << i << "th asteroid is " << curAstIdx << ", evaluation is " << stsc << ", correct commands are " << correctCommandsStateScores[curAstIdx] << std::endl;
		score += stsc;
		ncc += correctCommandsStateScores[curAstIdx];
		if(sensorActivityStateScores.size()!=0)
			totSensoryActivity += sensorActivityStateScores[curAstIdx];
//		if(correctCommandsStateScores[curAstIdx]<3)
//			break;
	}

	sampleScoresMap->append("score", score);
	sampleScoresMap->append("numCorrectCommands", static_cast<double>(ncc));
	sampleScoresMap->append("sensorActivity", totSensoryActivity/static_cast<double>(astsTotal));
}

void SpikesOnCubeMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	double score = sampleScoresMap->getAverage("score");
	double numCorrectCommands = sampleScoresMap->getAverage("numCorrectCommands");
	double sensorActivity = sampleScoresMap->getAverage("sensorActivity");
	unsigned tieredSensorActivity = static_cast<unsigned>(sensorActivity*10);
	org->dataMap.append("score", score );
	org->dataMap.append("guidingFunction", -score);
	org->dataMap.append("numCorrectCommands", numCorrectCommands);
	org->dataMap.append("sensorActivity", sensorActivity);
	org->dataMap.append("tieredSensorActivity", static_cast<double>(tieredSensorActivity));
}

int SpikesOnCubeMentalImage::numInputs() {
	return mnistNumBits;
}

/***** Private SpikesOnCubeMentalImage class definitions *****/

void SpikesOnCubeMentalImage::readOriginalCommands() {
	originalCommands.clear();
	const std::vector<std::vector<unsigned>>& commands = datasetParserPtr->cachingGetDescription(*currentAsteroidNamePtr);

	for(const auto& com : commands)
		originalCommands.push_back(std::make_tuple(com[0]));
}

void SpikesOnCubeMentalImage::readHelperANN() {
	std::string helperANNFilePath("helperann");

	std::ifstream helperANNFileStream(helperANNFilePath);
	if(!helperANNFileStream.good()) {
		std::cerr << "Couldnt read helper ANN from " << helperANNFilePath << ", exiting" << std::endl << std::flush;
		exit(EXIT_FAILURE);
	}
	std::string helperANNString;
	std::getline(helperANNFileStream, helperANNString);
	helperANNFileStream.close();

	helperANN.loadCompactStr(helperANNString);

// 	helperANN.drawWeights();
}

inline std::vector<double> SpikesOnCubeMentalImage::encodeStatement(const CommandType& st) {
	std::vector<double> nw_input(ANN_INPUT_SIZE, 0);

//	unsigned d;
//	std::tie(d) = st;

//	nw_input[0] = 0.;
//	nw_input[1] = propEnc(f, 6-1);
//	nw_input[2] = propEnc(1, 3); // MAX_FEATURE_SIZE is erroneously set to 3 in the craterless ALSD runs,
//	                             // but the corresponding ANN input never changes (stays equal to 1/3) so it should be fine
//	nw_input[3] = propEnc(i, q);
//	nw_input[4] = propEnc(j, q);

	return nw_input;
}

double SpikesOnCubeMentalImage::commandDivergence(const CommandType& lhs, const CommandType& rhs) {

	// L1 on transformed statement guidance is disabled for now
/*
	std::vector<State> transformedLHS = helperANN.forward(encodeStatement(lhs));
	std::vector<State> transformedRHS = helperANN.forward(encodeStatement(rhs));
	double l1dist = 0.;
	for(unsigned i=0; i<ANN_OUTPUT_SIZE; i++)
		l1dist += ( transformedRHS[i]>transformedLHS[i] ? transformedRHS[i]-transformedLHS[i] : transformedLHS[i]-transformedRHS[i] );
	return l1dist;
*/

	// Manually designed guiding function: Hamming distance
	unsigned d0, d1;
	std::tie(d0) = lhs;
	std::tie(d1) = rhs;
	return static_cast<double>( (d0>d1 ? d0-d1 : d1-d0) );
}

double SpikesOnCubeMentalImage::maxCommandDivergence() {

	// TODO: figure out what to do here for ANN-based guiding functions

	// For manually designed guiding funciton: Hamming distance
	return mnistNumDigits-1;
}

double SpikesOnCubeMentalImage::evaluateCommand(const CommandType& command) {

	std::cerr << "evaluateCommand called from SpikesOnCubeMentalImage - should never happen in this branch" << std::endl;
	exit(EXIT_FAILURE);

/*
	if(originalCommands.empty()) {
		std::cerr << "Evalution of a command asked before list of original comands was read, exiting" << std::endl << std::flush;
		exit(EXIT_FAILURE);
	}

//	std::cout << "Evaluating command "; printCommand(command);
//	std::cout << " against original commands: "; printCommandsVector(originalCommands); std::cout << std::endl << std::flush;
//	std::cout << "Mask in the beginning:"; for(auto v : ocApproximationAttempted) std::cout << ' ' << (v ? 1 : 0); std::cout << std::endl;

	const unsigned numOriginalCommands = originalCommands.size();

	double minDivergence = 0.;
	int minDivergenceCommandIdx = -1;
	for(unsigned i=0; i<numOriginalCommands; i++) {
		if(!ocApproximationAttempted[i]) {
			double currentdiv = commandDivergence(originalCommands[i], command);

//			std::cout << "i=" << i << ": ANN divergence from "; printCommand(originalCommands[i]); std::cout << " to "; printCommand(command); std::cout << " is " << currentdiv << std::endl;

			if(minDivergenceCommandIdx==-1 || (minDivergenceCommandIdx>=0 && currentdiv<minDivergence) ) {
				minDivergence = currentdiv;
				minDivergenceCommandIdx = i;
			}
		}
	}

	ocApproximationAttempted[minDivergenceCommandIdx] = true;

//	std::cout << "Marking statement #" << minDivergenceCommandIdx << " ("; printCommand(originalCommands[minDivergenceCommandIdx]); std::cout << ", divergence " << minDivergence << ") as attempted" << std::endl;

	return minDivergence;
*/
	return 0;
}

const std::vector<unsigned>& SpikesOnCubeMentalImage::getEvaluationOrder(unsigned lineageID, unsigned numAsteroids) {

	std::map<unsigned,std::vector<unsigned>>::iterator itToEvalOrder;
	bool orderIsNew;
	std::tie(itToEvalOrder,orderIsNew) = lineageToEvaluationOrder.emplace(lineageID, numAsteroids);

	if(orderIsNew) {
		for(unsigned i=0; i<numAsteroids; i++)
			itToEvalOrder->second.at(i) = i;

		std::mt19937 rng(42*lineageID);
		std::shuffle(itToEvalOrder->second.begin(), itToEvalOrder->second.end(), rng);
//		std::shuffle(itToEvalOrder->second.begin(), itToEvalOrder->second.end(), Random::getCommonGenerator());
	}

	return itToEvalOrder->second;
}
