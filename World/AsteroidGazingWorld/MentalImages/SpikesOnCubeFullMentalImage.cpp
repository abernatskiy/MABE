#include "SpikesOnCubeFullMentalImage.h"
#include "decoders.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdlib>

void printCommand1(CommandType com) {
	unsigned f,i,j;
	std::tie(f,i,j) = com;
	std::cout << f << ' ' << i << ' ' << j;
}

void printCommandsVector1(std::vector<CommandType> commands) {
	for(auto coords : commands) {
		std::cout << ' '; printCommand1(coords); std::cout << ';';
	}
}

/***** Public SpikesOnCubeFullMentalImage class definitions *****/

SpikesOnCubeFullMentalImage::SpikesOnCubeFullMentalImage(std::shared_ptr<std::string> curAstNamePtr,
	                                               std::shared_ptr<AsteroidsDatasetParser> dsParserPtr) :
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	justReset(true),
	helperANN({ANN_INPUT_SIZE, ANN_HIDDEN_SIZE, ANN_OUTPUT_SIZE}),
	cl(Global::outputPrefixPL->get() + "commands.log"),
	visualize(Global::modePL->get() == "visualize") {

	readHelperANN();
	if(visualize)
		cl.open();
}

void SpikesOnCubeFullMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	stateScores.clear();
	correctCommandsStateScores.clear();
	currentCommands.clear();
	// originalCommands are taken care of in readOriginalCommands()
	justReset = true;
}

void SpikesOnCubeFullMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
	currentCommands.clear();
	// originalCommands are taken care of in readOriginalCommands()
	justReset = true;

	if(visualize) {
//		cl.logMessage("resetAfterWorldStateChange called");
		currentGuesses.clear();
	}
}

void SpikesOnCubeFullMentalImage::updateWithInputs(std::vector<double> inputs) {
	if(justReset) {
		justReset = false;
		return;
	}

	currentCommands.clear();

	for(unsigned ci=0; ci<3; ci++) {

		unsigned face, i, j;
		auto it = inputs.begin() + ci*(bitsForFace+2*bitsForCoordinate);

		face = decodeSPUInt(it, it+bitsForFace);
//		face = decodeOHUInt(it, it+bitsForFace);
		it += bitsForFace;

		i = decodeSPUInt(it, it+bitsForCoordinate) + 1; // +1 is to account for the forbidden nature of edges
//		i = decodeOHUInt(it, it+bitsForCoordinate);
		it += bitsForCoordinate;

		j = decodeSPUInt(it, it+bitsForCoordinate);
//		j = decodeOHUInt(it, it+bitsForCoordinate);

		currentCommands.push_back(std::make_tuple(face,i,j));
	}
}

void SpikesOnCubeFullMentalImage::recordRunningScoresWithinState(int stateTime, int statePeriod) {

	if(stateTime == 0) {

		readOriginalCommands();
		ocApproximationAttempted.resize(originalCommands.size());
		correctCommandsStateScores.push_back(0);
		stateScores.push_back(0.);
	}

	if(currentCommands.size() != 0) {

		unsigned numCorrectCommands = 0;
		for(auto it=originalCommands.begin(); it!=originalCommands.end(); it++)
			if(std::find(currentCommands.begin(), currentCommands.end(), *it) != currentCommands.end())
				numCorrectCommands++;
		if( correctCommandsStateScores.back() < numCorrectCommands )
			correctCommandsStateScores.back() = numCorrectCommands;

		double cumulativeDivergence = 0.;
		std::fill(ocApproximationAttempted.begin(), ocApproximationAttempted.end(), false);
		for(auto it=currentCommands.begin(); it!=currentCommands.end(); it++)
			cumulativeDivergence += evaluateCommand(*it);
		stateScores.back() += cumulativeDivergence/currentCommands.size();

		if(visualize)
			currentGuesses.push_back(std::vector<CommandType>(currentCommands));
	}

	if(stateTime == statePeriod-1) {

		stateScores.back() /= statePeriod;

		if(visualize)
			cl.logMapping(originalCommands, currentGuesses);

//		std::cout << "Evaluation of the current individual was " << stateScores.back() << std::endl << std::endl;

/*
		// REWRITE
		std::cout << "Brain generated commands for " << *currentAsteroidNamePtr  << ":" << std::endl;
		for(auto it=currentCommands.end()-numOriginalCommands; it!=currentCommands.end(); it++) {
			printCommand1(*it);
			std::cout << std::endl;
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

void SpikesOnCubeFullMentalImage::recordRunningScores(std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void SpikesOnCubeFullMentalImage::recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
	sampleScoresMap->append( "guidingFunction",
	                         static_cast<double>(std::accumulate(stateScores.begin(), stateScores.end(), 0.)) / stateScores.size() );
	sampleScoresMap->append( "numCorrectCommands",
	                         static_cast<double>(std::accumulate(correctCommandsStateScores.begin(), correctCommandsStateScores.end(), 0)));
}

void SpikesOnCubeFullMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	double gerror = sampleScoresMap->getAverage("guidingFunction");
	double numCorrectCommands = sampleScoresMap->getAverage("numCorrectCommands");
	org->dataMap.append("score", 1./(1.+gerror));
	org->dataMap.append("guidingFunction", gerror);
	org->dataMap.append("numCorrectCommands", numCorrectCommands);
}

int SpikesOnCubeFullMentalImage::numInputs() {
	return 3*(bitsForFace + 2*bitsForCoordinate);
}

/***** Private SpikesOnCubeFullMentalImage class definitions *****/

void SpikesOnCubeFullMentalImage::readOriginalCommands() {
	originalCommands.clear();
	std::string commandsFilePath = datasetParserPtr->getDescriptionPath(*currentAsteroidNamePtr);
	std::ifstream commandsFstream(commandsFilePath);
	std::string cline;
	while( std::getline(commandsFstream, cline) ) {
		unsigned face, i, j;
		std::stringstream cstream(cline);
		cstream >> face >> i >> j;
		originalCommands.push_back(std::make_tuple(face,i,j));
	}
	commandsFstream.close();

/*
	std::cout << "Read original commands for " << *currentAsteroidNamePtr <<" from " << commandsFilePath << ":" << std::endl;
	for(auto command : originalCommands) {
		printCommand1(command);
		std::cout << std::endl;
	}
	std::cout << std::endl;
*/
}

void SpikesOnCubeFullMentalImage::readHelperANN() {
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

inline std::vector<double> SpikesOnCubeFullMentalImage::encodeStatement(const CommandType& st) {
	std::vector<double> nw_input(ANN_INPUT_SIZE);

	unsigned f,i,j;
	std::tie(f,i,j) = st;

	nw_input[0] = 0.;
	nw_input[1] = propEnc(f, 6-1);
	nw_input[2] = propEnc(1, 3); // MAX_FEATURE_SIZE is erroneously set to 3 in the craterless ALSD runs,
	                             // but the corresponding ANN input never changes (stays equal to 1/3) so it should be fine
	nw_input[3] = propEnc(i, q);
	nw_input[4] = propEnc(j, q);

	return nw_input;
}

double SpikesOnCubeFullMentalImage::commandDivergence(const CommandType& lhs, const CommandType& rhs) {

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
	unsigned f0, i0, j0, f1, i1, j1;
	std::tie(f0, i0, j0) = lhs;
	std::tie(f1, i1, j1) = rhs;
	return static_cast<double>( (f0>f1 ? f0-f1 : f1-f0) +
	                            (i0>i1 ? i0-i1 : i1-i0) +
	                            (j0>j1 ? j0-j1 : j1-j0) ) / (5+2*q);
}

double SpikesOnCubeFullMentalImage::evaluateCommand(const CommandType& command) {

	if(originalCommands.empty()) {
		std::cerr << "Evalution of a command asked before list of original comands was read, exiting" << std::endl << std::flush;
		exit(EXIT_FAILURE);
	}

//	std::cout << "Evaluating command "; printCommand1(command);
//	std::cout << " against original commands: "; printCommandsVector1(originalCommands); std::cout << std::endl << std::flush;
//	std::cout << "Mask in the beginning:"; for(auto v : ocApproximationAttempted) std::cout << ' ' << (v ? 1 : 0); std::cout << std::endl;

	const unsigned numOriginalCommands = originalCommands.size();

	double minDivergence = 0.;
	int minDivergenceCommandIdx = -1;
	for(unsigned i=0; i<numOriginalCommands; i++) {
		if(!ocApproximationAttempted[i]) {
			double currentdiv = commandDivergence(originalCommands[i], command);

//			std::cout << "i=" << i << ": ANN divergence from "; printCommand1(originalCommands[i]); std::cout << " to "; printCommand1(command); std::cout << " is " << currentdiv << std::endl;

			if(minDivergenceCommandIdx==-1 || (minDivergenceCommandIdx>=0 && currentdiv<minDivergence) ) {
				minDivergence = currentdiv;
				minDivergenceCommandIdx = i;
			}
		}
	}

	ocApproximationAttempted[minDivergenceCommandIdx] = true;

//	std::cout << "Marking statement #" << minDivergenceCommandIdx << " ("; printCommand1(originalCommands[minDivergenceCommandIdx]); std::cout << ", divergence " << minDivergence << ") as attempted" << std::endl;

	return minDivergence;
}
