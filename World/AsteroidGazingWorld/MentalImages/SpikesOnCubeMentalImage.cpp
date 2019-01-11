#include "SpikesOnCubeMentalImage.h"
#include "decoders.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdlib>

/***** Public SpikesOnCubeMentalImage class definitions *****/

SpikesOnCubeMentalImage::SpikesOnCubeMentalImage(std::shared_ptr<std::string> curAstNamePtr,
	                                               std::shared_ptr<AsteroidsDatasetParser> dsParserPtr) :
		currentAsteroidNamePtr(curAstNamePtr),
		datasetParserPtr(dsParserPtr),
		justReset(true) {

	readHelperANN();
}

void SpikesOnCubeMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	stateScores.clear();
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

	currentCommands.insert(std::make_tuple(face,i,j));
}

void SpikesOnCubeMentalImage::recordRunningScoresWithinState(int stateTime, int statePeriod) {
	if(stateTime == statePeriod-1) {
		readOriginalCommands();
		ocApproximationAttempted.resize(originalCommands.size());
		std::fill(ocApproximationAttempted.begin(), ocApproximationAttempted.end(), false);

/*
		unsigned numCorrectCommands = 0;
		for(auto it=originalCommands.begin(); it!=originalCommands.end(); it++) {
			if(currentCommands.count(*it) != 0)
				numCorrectCommands++;
		}
		stateScores.push_back(numCorrectCommands);
*/

		double cumulativeDivergence = 0.;
		for(auto curComm : currentCommands)
			cumulativeDivergence += evaluateCommand(curComm);

		stateScores.push_back(cumulativeDivergence);

/*
		std::cout << "Brain generated commands for " << *currentAsteroidNamePtr  << ":"; // FIXME
		for(auto com : currentCommands) {
			unsigned f,i,j;
			std::tie(f,i,j) = com;
			std::cout << ' ' << f << ' ' << i << ' ' << j << ';';
		}
		std::cout << std::endl;
*/

//		std::cout << "Added " << numCorrectCommands << " to evaluations of " << *currentAsteroidNamePtr << std::endl;
//		std::cout << "Full evaluations:";
//		for(auto sc : stateScores)
//			std::cout << ' ' << sc;
//		std::cout << std::endl;
	}
}

void SpikesOnCubeMentalImage::recordRunningScores(std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void SpikesOnCubeMentalImage::recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
	sampleScoresMap->append("score", static_cast<double>(std::accumulate(stateScores.begin(), stateScores.end(), 0.)));
}

void SpikesOnCubeMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	double score = sampleScoresMap->getAverage("score");
	if(visualize) std::cout << "Assigning score of " << score << " to organism " << org << std::endl;
	org->dataMap.append("score", score);
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

//	std::cout << "Read original commands for " << *currentAsteroidNamePtr <<" from " << commandsFilePath << std::endl;
/*	std::cout << "Original commands:";
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
}

inline void SpikesOnCubeMentalImage::encodeStatement(const CommandType& st, std::vector<double>& stor, unsigned shiftBy=0) {
	unsigned f,i,j;
	std::tie(f,i,j) = st;

	stor[shiftBy+0] = 0.;
	stor[shiftBy+1] = propEnc(f, 6-1);
	stor[shiftBy+2] = propEnc(1, 3); // MAX_FEATURE_SIZE is erroneously set to 3 in the craterless ALSD runs,
	                                 // but the corresponding ANN input never changes (stays equal to 1/3) so it should be fine
	stor[shiftBy+3] = propEnc(i, q);
	stor[shiftBy+4] = propEnc(j, q);
};

double commandDivergence(const CommandType& lhs, const CommandType& rhs) {

	std::vector<double> nw_input(10);
	encodeStatement(lhs, nw_input, 0);
	encodeStatement(rhs, nw_input, 5);
	double annOutput = helperANN.forward(encodeStatements(lhs, rhs))[0];
	double annZero = helperANN.forward(encodeStatements(lhs, lhs))[0];

	return annOutput - annZero;
}

double SpikesOnCubeMentalImage::evaluateCommand(const CommandType& command) {

	if(originalCommands.empty()) {
		std::cerr << "Evalution of a command asked before list of original comands was read, exiting" << std::endl << std::flush;
		exit(EXIT_FAILURE);
	}

	const unsigned numOriginalCommands = originalCommands.size();

	double minDivergence = 0.;
	int minDivergenceCommandIdx = -1;
	for(unsigned i=0; i<numOriginalCommands; i++) {
		if(!ocApproximationAttempted) {
			double currentdiv = commandDivergence(originalCommands[i], command);
			if(minDivergenceCommandIdx==-1 || (minDivergenceCommandIdx>=0 && currentdiv<minDivergence) ) {
				minDivergence = currentdiv;
				minDivergenceCommandIdx = i;
			}
		}
	}

	ocApproximationAttempted[minDivergenceCommandIdx] = true;

	return minDivergence;
}
