#include "SpikesOnCubeFullMentalImage.h"
#include "decoders.h"

/***** Utility funcs *****/

std::tuple<double,bool> evaluateRange(const CommandRangeType& guessesRange, const CommandType& originalCommand) {

	// returns a score that shows how close the range is to the original command and a Boolean telling if it's a direct hit

	unsigned of, oi, oj;
	std::tie(of, oi, oj) = originalCommand;

	double eval = 0.;
	bool preciseHit = true;

	if(std::find(std::get<0>(guessesRange).begin(),
	             std::get<0>(guessesRange).end(), of) != std::get<0>(guessesRange).end()) {
		eval += 1./static_cast<double>(std::get<0>(guessesRange).size()); // Arend suggests to square the size. I'll try without it
		if(std::get<0>(guessesRange).size()!=1)
			preciseHit = false;
	}
	else
		preciseHit = false;

	if(std::find(std::get<1>(guessesRange).begin(),
	             std::get<1>(guessesRange).end(), oi) != std::get<1>(guessesRange).end()) {
		eval += 1./static_cast<double>(std::get<1>(guessesRange).size());
		if(std::get<1>(guessesRange).size()!=1)
			preciseHit = false;
	}
	else
		preciseHit = false;

	if(std::find(std::get<2>(guessesRange).begin(),
	             std::get<2>(guessesRange).end(), oj) != std::get<2>(guessesRange).end()) {
		eval += 1./static_cast<double>(std::get<2>(guessesRange).size());
		if(std::get<2>(guessesRange).size()!=1)
			preciseHit = false;
	}
	else
		preciseHit = false;

	return std::make_tuple(eval/3., preciseHit);
}

/***** Public SpikesOnCubeFullMentalImage class definitions *****/

SpikesOnCubeFullMentalImage::SpikesOnCubeFullMentalImage(std::shared_ptr<std::string> curAstNamePtr,
	                                                       std::shared_ptr<AsteroidsDatasetParser> dsParserPtr) :
	SpikesOnCubeMentalImage(curAstNamePtr, dsParserPtr) {}


void SpikesOnCubeFullMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	SpikesOnCubeMentalImage::reset(visualize);
	currentCommandRanges.clear();
}

void SpikesOnCubeFullMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
	SpikesOnCubeMentalImage::resetAfterWorldStateChange(visualize);
	currentCommandRanges.clear();
}

void SpikesOnCubeFullMentalImage::updateWithInputs(std::vector<double> inputs) {
	if(justReset) {
		justReset = false;
		return;
	}

	currentCommandRanges.clear();
	for(unsigned ci=0; ci<3; ci++) {
		auto it = inputs.begin() + ci*(lBitsForFace+2*lBitsForCoordinate);

		auto faceRange = decodeMHVUInt(it, it+lBitsForFace);
		it += lBitsForFace;

		auto iRange = decodeMHVUInt(it, it+lBitsForCoordinate);
		it += lBitsForCoordinate;

		auto jRange = decodeMHVUInt(it, it+lBitsForCoordinate);

		currentCommandRanges.push_back(std::make_tuple(faceRange, iRange, jRange));
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
		double cumulativeScore = 0.;

		double curScore;
		bool curHit;

		std::fill(ocApproximationAttempted.begin(), ocApproximationAttempted.end(), false);
		for(const auto& curRange : currentCommandRanges) {
			std::tie(curScore, curHit) = evaluateRangeVSSet(curRange);
			cumulativeScore += curScore;
			if(curHit)
				numCorrectCommands++;
		}


		if( correctCommandsStateScores.back() < numCorrectCommands )
			correctCommandsStateScores.back() = numCorrectCommands;
		stateScores.back() += cumulativeScore/currentCommandRanges.size();

	}

	if(stateTime == statePeriod-1) {
		stateScores.back() /= statePeriod;
	}
}

int SpikesOnCubeFullMentalImage::numInputs() {
	return 3*(lBitsForFace + 2*lBitsForCoordinate);
}

/***** Private SpikesOnCubeFullMentalImage class definitions *****/

std::tuple<double,bool> SpikesOnCubeFullMentalImage::evaluateRangeVSSet(const CommandRangeType& guessesRange) {

	// returns a score that shows how close the range is to the closest original command,
	//         a Boolean telling if there were any direct hits,
	//     and the index of the closest original command
	// Takes into account and modifies the mask.

	double highestEval;
	bool preciseHit = false;
	int highestIdx = -1;

	for(unsigned i=0; i<originalCommands.size(); i++) {
		if(!ocApproximationAttempted[i]) {
			double curEval;
			bool curHit;

			std::tie(curEval, curHit) = evaluateRange(guessesRange, originalCommands.at(i));

			if(highestIdx==-1 || (highestIdx>=0 && curEval>highestEval) ) {
				highestEval = curEval;
				highestIdx = i;
			}

			if(curHit)
				preciseHit = true;
		}
	}
	ocApproximationAttempted[highestIdx] = true;

	return std::make_tuple(highestEval, preciseHit);
}
