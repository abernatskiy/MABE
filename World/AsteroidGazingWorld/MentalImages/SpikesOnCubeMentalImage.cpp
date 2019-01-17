#include "SpikesOnCubeMentalImage.h"
#include "decoders.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdlib>

void printCommand(CommandType com) {
	unsigned f,i,j;
	std::tie(f,i,j) = com;
	std::cout << f << ' ' << i << ' ' << j;
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
		helperANN({ANN_INPUT_SIZE, ANN_HIDDEN_SIZE, 1}) {

	readHelperANN();
}

void SpikesOnCubeMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	stateScores.clear();
	correctCommandsStateScores.clear();
	currentCommands.clear();
	// originalCommands are taken care of in readOriginalCommands()
	justReset = true;
}

void SpikesOnCubeMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
	currentCommands.clear();
	// originalCommands are taken care of in readOriginalCommands()
	justReset = true;
}

void SpikesOnCubeMentalImage::updateWithInputs(std::vector<double> inputs) {
	if(justReset) {
		justReset = false;
		return;
	}

	unsigned face, i, j;
	auto it = inputs.begin();

	face = decodeUInt(it, it+bitsForFace);
	it += bitsForFace;

	i = decodeUInt(it, it+bitsForCoordinate);
	it += bitsForCoordinate;

	j = decodeUInt(it, it+bitsForCoordinate);

	currentCommands.push_back(std::make_tuple(face,i,j));
}

void SpikesOnCubeMentalImage::recordRunningScoresWithinState(int stateTime, int statePeriod) {
	if(stateTime == statePeriod-1) {
		readOriginalCommands();
		ocApproximationAttempted.resize(originalCommands.size());
		std::fill(ocApproximationAttempted.begin(), ocApproximationAttempted.end(), false);

		unsigned numCorrectCommands = 0;
		for(auto it=originalCommands.begin(); it!=originalCommands.end(); it++)
			if(std::find(currentCommands.begin(), currentCommands.end(), *it) != currentCommands.end())
				numCorrectCommands++;
		correctCommandsStateScores.push_back(numCorrectCommands);

		double cumulativeDivergence = 0.;
		for(auto curComm : currentCommands)
			cumulativeDivergence += evaluateCommand(curComm);
		stateScores.push_back(cumulativeDivergence);

//		std::cout << "Evaluation of the current individual was " << cumulativeDivergence << std::endl << std::endl;

/*
		std::cout << "Brain generated commands for " << *currentAsteroidNamePtr  << ":"; // FIXME
		for(auto com : currentCommands) {
			unsigned f,i,j;
			std::tie(f,i,j) = com;
			std::cout << ' ' << f << ' ' << i << ' ' << j << ';';
		}
		std::cout << std::endl;

		std::cout << "Added " << cumulativeDivergence << " to evaluations of " << *currentAsteroidNamePtr << std::endl;
		std::cout << "Full evaluations:";
		for(auto sc : correctCommandsStateScores)
			std::cout << ' ' << sc;
		std::cout << std::endl;
*/

	}
}

void SpikesOnCubeMentalImage::recordRunningScores(std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void SpikesOnCubeMentalImage::recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
	sampleScoresMap->append("score", static_cast<double>(std::accumulate(stateScores.begin(), stateScores.end(), 0.)));
	sampleScoresMap->append("numCorrectCommands", static_cast<double>(std::accumulate(correctCommandsStateScores.begin(), correctCommandsStateScores.end(), 0)));
}

void SpikesOnCubeMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	double error = sampleScoresMap->getAverage("score");
	double numCorrectCommands = sampleScoresMap->getAverage("numCorrectCommands");
//	std::cout << "Assigning score of " << score << " to organism " << org << std::endl;
	org->dataMap.append("annError", error);
	org->dataMap.append("score", static_cast<double>(numCorrectCommands) );
	org->dataMap.append("numCorrectCommands", numCorrectCommands);
}

int SpikesOnCubeMentalImage::numInputs() {
	return bitsForFace + 2*bitsForCoordinate;
}

/***** Private SpikesOnCubeMentalImage class definitions *****/

void SpikesOnCubeMentalImage::readOriginalCommands() {
	originalCommands.clear();
	std::string commandsFilePath = datasetParserPtr->getDescriptionPath(*currentAsteroidNamePtr);
	std::ifstream commandsFstream(commandsFilePath);
	std::string cline;
	while( std::getline(commandsFstream, cline) ) {
		unsigned face, i, j;
		std::stringstream cstream(cline);
		cstream >> face >> i >> j; // TODO: edit to match Lindsey commands format
		originalCommands.push_back(std::make_tuple(face,i,j));
	}
	commandsFstream.close();

/*
	std::cout << "Read original commands for " << *currentAsteroidNamePtr <<" from " << commandsFilePath << std::endl;
	std::cout << "Original commands:";
	for(auto coords : originalCommands) {
		unsigned f,i,j;
		std::tie(f,i,j) = coords;
		std::cout << ' ' << f << ' ' << i << ' ' << j << ';';
	}
	std::cout << std::endl;
*/
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

inline void SpikesOnCubeMentalImage::encodeStatement(const CommandType& st, std::vector<double>& stor, unsigned shiftBy) {
	unsigned f,i,j;
	std::tie(f,i,j) = st;

	stor[shiftBy+0] = 0.;
	stor[shiftBy+1] = propEnc(f, 6-1);
	stor[shiftBy+2] = propEnc(1, 3); // MAX_FEATURE_SIZE is erroneously set to 3 in the craterless ALSD runs,
	                                 // but the corresponding ANN input never changes (stays equal to 1/3) so it should be fine
	stor[shiftBy+3] = propEnc(i, q);
	stor[shiftBy+4] = propEnc(j, q);
}

inline std::vector<double> SpikesOnCubeMentalImage::encodeStatementPair(const CommandType& lhs, const CommandType& rhs) {
	std::vector<double> nw_input(ANN_INPUT_SIZE);
	encodeStatement(lhs, nw_input, 0);
	encodeStatement(rhs, nw_input, ANN_INPUT_SIZE/2);
	return nw_input;
}

double SpikesOnCubeMentalImage::commandDivergence(const CommandType& lhs, const CommandType& rhs) {

/*	if(lhs == rhs)
		return 0.;
	else {
		double annOutput = helperANN.forward(encodeStatementPair(lhs, rhs))[0];
		double annZero = helperANN.forward(encodeStatementPair(lhs, lhs))[0];
		return annOutput>annZero ? annOutput-annZero : 1.;
	}
*/

/*	unsigned f0, i0, j0, f1, i1, j1;
	std::tie(f0, i0, j0) = lhs;
	std::tie(f1, i1, j1) = rhs;
	return static_cast<double>( (f0>f1 ? f0-f1 : f1-f0) +
	                            (i0>i1 ? i0-i1 : i1-i0) +
	                            (j0>j1 ? j0-j1 : j1-j0) );
*/
	return 0.;
}

double SpikesOnCubeMentalImage::evaluateCommand(const CommandType& command) {

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
}
