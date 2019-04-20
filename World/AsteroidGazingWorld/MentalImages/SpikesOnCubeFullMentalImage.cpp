#include "SpikesOnCubeFullMentalImage.h"
#include "decoders.h"

/***** Utility funcs *****/

std::string commandRangeToStr(const CommandRangeType& crange) {
	const std::vector<unsigned>& frange = std::get<0>(crange);
	std::ostringstream s;
	s << "[{";
	for(auto it=frange.cbegin(); it!=frange.cend(); it++)
		s << *it << ( it!=frange.cend()-1 ? "," : "" );
	s << "}]";
	return s.str();
}

std::string commandToStr(const CommandType& com) {
	std::ostringstream s;
	s << "[" << std::get<0>(com) // << ","
//	         << std::get<1>(com) << ","
//	         << std::get<2>(com)
	         << "]";
	return s.str();
}

std::string bitRangeToStr(std::vector<double>::iterator startAt, unsigned bits) {
	std::ostringstream s;
	for(auto it=startAt; it!=startAt+bits; it++)
		s << ( *it==0. ? 0 : 1 );
	return s.str();
}

std::tuple<double,bool> evaluateRange(const CommandRangeType& guessesRange, const CommandType& originalCommand) {
	// returns a score that shows how close the range is to the original command and a Boolean telling if it's a direct hit
	unsigned od;
	std::tie(od) = originalCommand;

	double eval = 0.;
	bool preciseHit = true;

	if(std::find(std::get<0>(guessesRange).begin(),
	             std::get<0>(guessesRange).end(), od) != std::get<0>(guessesRange).end()) {
		eval += 1./static_cast<double>(std::get<0>(guessesRange).size()); // Arend suggests to square the size. I'll try without it
		if(std::get<0>(guessesRange).size()!=1)
			preciseHit = false;
	}
	else
		preciseHit = false;

//	std::cout << "Range " << commandRangeToStr(guessesRange) << " evaluated with respect to command ";
//	printCommand(originalCommand);
//	std::cout << ", yielding " << eval/3. << " and " << ( preciseHit ? "a precise hit" : "no precise hit") << std::endl;

//	if(preciseHit) { std::cout << "Precise hit achieved by range " << commandRangeToStr(guessesRange) << " on command "; printCommand(originalCommand); std::cout << std::endl; }

	return std::make_tuple(eval/1., preciseHit);
}

/***** Public SpikesOnCubeFullMentalImage class definitions *****/

SpikesOnCubeFullMentalImage::SpikesOnCubeFullMentalImage(std::shared_ptr<std::string> curAstNamePtr,
	                                                       std::shared_ptr<AsteroidsDatasetParser> dsParserPtr,
	                                                       std::shared_ptr<AbsoluteFocusingSaccadingEyesSensors> sPtr,
                                                         unsigned nTriggerBits,
                                                         bool intFitness) :
	SpikesOnCubeMentalImage(curAstNamePtr, dsParserPtr),
	sensorsPtr(sPtr),
	numTriggerBits(nTriggerBits),
	integrateFitness(intFitness),
	answerGiven(false),
	answerReceived(false) {}

void SpikesOnCubeFullMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	SpikesOnCubeMentalImage::reset(visualize);
	currentCommandRanges.clear();
	answerGiven = false;
	answerReceived = false;
}

void SpikesOnCubeFullMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
	SpikesOnCubeMentalImage::resetAfterWorldStateChange(visualize);
	currentCommandRanges.clear();
	answerGiven = false;
	answerReceived = false;
	if(visualize)
		commandRangesTS.clear();
}

void SpikesOnCubeFullMentalImage::updateWithInputs(std::vector<double> inputs) {
	if(justReset) {
		justReset = false;
//		return; // NOTE: if the image has just been reset, we can return early here to save a few CPU cycles
	}

	currentCommandRanges.clear();
	auto it = inputs.begin();
	auto digitRange = decodeMHVUInt(it, it+mnistNumBits);
	currentCommandRanges.push_back(std::make_tuple(digitRange));
	if(mVisualize)
		commandRangesTS.push_back(currentCommandRanges);
//  std::cout << "Got range " << commandRangeToStr(currentCommandRanges.back()) << " from bits " << bitRangeToStr(inputs.begin(), mnistNumBits) << std::endl;
//  exit(0);

	answerGiven = decodeTriggerBits(it+mnistNumBits, it+mnistNumBits+numTriggerBits);
//  std::cout << "Got trigger value " << answerGiven << " from bits " << bitRangeToStr(it+mnistNumBits, numTriggerBits) << std::endl;
}

void SpikesOnCubeFullMentalImage::recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) {
	if(answerReceived)
		return;

	if(stateTime == 0) {
		readOriginalCommands();
		ocApproximationAttempted.resize(originalCommands.size());
		correctCommandsStateScores.push_back(0);
		stateScores.push_back(0.);

		if(mVisualize) {
			std::cout << "For shape " << *currentAsteroidNamePtr << ":" << std::endl
								<< "Original commands are ";
			for(unsigned i=0; i<1; i++) {
				printCommand(originalCommands[i]);
				std::cout << ( i==0 ? "" : ", " );
			}
			std::cout << std::endl;
		}
	}

	double curEval = -42;
	unsigned numCorrectCommands = 0;

	if(currentCommandRanges.size() != 0) {
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

		if(correctCommandsStateScores.back() < numCorrectCommands)
			correctCommandsStateScores.back() = numCorrectCommands;

		if(integrateFitness)
			stateScores.back() += cumulativeScore/currentCommandRanges.size();
		else
			stateScores.back() = cumulativeScore/currentCommandRanges.size();

		curEval = cumulativeScore/currentCommandRanges.size();
	}

	if(answerGiven || stateTime == statePeriod-1) {
		if(answerGiven) {
			std::cout << "Trigger pressed at " << stateTime << std::endl;
			stateScores.back() = curEval;
			correctCommandsStateScores.back() = numCorrectCommands;
		}
		else if(integrateFitness)
			stateScores.back() /= statePeriod-1;

		sensorActivityStateScores.push_back(static_cast<double>(sensorsPtr->numSaccades())/static_cast<double>(statePeriod));

		answerReceived = true;

		if(mVisualize) {
			std::cout << "Brain generated command ranges at the end of evaluation:";
			for(const auto& curRange : currentCommandRanges)
				std::cout << " " << commandRangeToStr(curRange);
			std::cout << std::endl;
			std::cout << "Final score: " << stateScores.back() << " final hits: " << correctCommandsStateScores.back() << std::endl << std::endl;
//			std::cout << "Full evaluations:";
//			for(auto sc : stateScores)
//				std::cout << ' ' << sc;
//			std::cout << std::endl;
		}

	}
}

int SpikesOnCubeFullMentalImage::numInputs() {
	return mnistNumBits + numTriggerBits;
}

void* SpikesOnCubeFullMentalImage::logTimeSeries(const std::string& label) {

	std::ofstream guessesLog(std::string("guesses_") + label + std::string(".log"));
	for(const auto& guess : commandRangesTS) {
		for(auto ocit=originalCommands.cbegin(); ocit!=originalCommands.cend(); ocit++)
			guessesLog << commandToStr(*ocit) << " ";
		for(auto gueit=guess.cbegin(); gueit!=guess.cend(); gueit++)
			guessesLog << commandRangeToStr(*gueit) << ( gueit!=guess.end()-1 ? " " : "" );
		guessesLog << std::endl;
	}
	guessesLog.close();
	return nullptr;
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

//	std::cout << "Range vs set yielded " << highestEval << " and " << ( preciseHit ? "a precise hit" : "no precise hit") << std::endl;

	return std::make_tuple(highestEval, preciseHit);
}
