#include "SpikesOnCubeFullMentalImage.h"
#include "decoders.h"

/***** Public SpikesOnCubeFullMentalImage class definitions *****/

SpikesOnCubeFullMentalImage::SpikesOnCubeFullMentalImage(std::shared_ptr<std::string> curAstNamePtr,
	                                                       std::shared_ptr<AsteroidsDatasetParser> dsParserPtr) :
	SpikesOnCubeMentalImage(curAstNamePtr, dsParserPtr) {}


void SpikesOnCubeFullMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	SpikesOnCubeMentalImage::reset(visualize);
	if(mVisualize)
		currentGuesses.clear();
}

void SpikesOnCubeFullMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
	SpikesOnCubeMentalImage::resetAfterWorldStateChange(visualize);
	if(mVisualize)
		currentGuesses.clear();
}

void SpikesOnCubeFullMentalImage::updateWithInputs(std::vector<double> inputs) {
	if(justReset) {
		justReset = false;
		return;
	}

	currentCommands.clear();
	for(unsigned ci=0; ci<3; ci++) {
		unsigned face, i, j;
		auto it = inputs.begin() + ci*(lBitsForFace+2*lBitsForCoordinate);

		face = decodeSPUInt(it, it+lBitsForFace);
//		face = decodeOHUInt(it, it+lBitsForFace);
		it += lBitsForFace;

		i = decodeSPUInt(it, it+lBitsForCoordinate) + 1; // +1 is to account for the forbidden nature of edges
//		i = decodeOHUInt(it, it+lBitsForCoordinate);
		it += lBitsForCoordinate;

		j = decodeSPUInt(it, it+lBitsForCoordinate);
//		j = decodeOHUInt(it, it+lBitsForCoordinate);

		currentCommands.push_back(std::make_tuple(face,i,j));
	}
}

void SpikesOnCubeFullMentalImage::recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) {

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

		if(mVisualize)
			currentGuesses.push_back(std::vector<CommandType>(currentCommands));
	}

	if(stateTime == statePeriod-1) {
		stateScores.back() /= statePeriod;
		if(mVisualize)
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

int SpikesOnCubeFullMentalImage::numInputs() {
	return 3*(lBitsForFace + 2*lBitsForCoordinate);
}
