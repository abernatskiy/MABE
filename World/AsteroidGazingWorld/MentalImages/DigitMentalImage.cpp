#include "DigitMentalImage.h"
#include "decoders.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstdlib>

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
	s << "[" << std::get<0>(com) << "]";
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

	return std::make_tuple(eval/1., preciseHit);
}

/***** Public DigitMentalImage class definitions *****/

DigitMentalImage::DigitMentalImage(std::shared_ptr<std::string> curAstNamePtr,
                                   std::shared_ptr<AsteroidsDatasetParser> dsParserPtr,
                                   std::shared_ptr<AbstractSensors> sPtr,
                                   int nTriggerBits,
                                   bool intFitness) :
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	totalBitsStateScore(0),
	justReset(true),
	cl(Global::outputPrefixPL->get() + "commands.log"),
	mVisualize(Global::modePL->get() == "visualize"),
	sensorsPtr(sPtr),
	numTriggerBits(nTriggerBits<0 ? static_cast<unsigned>(-1*nTriggerBits) : static_cast<unsigned>(nTriggerBits)),
	requireTriggering(nTriggerBits<0),
	integrateFitness(intFitness),
	answerGiven(false),
	answerReceived(false) {

	if(mVisualize)
		cl.open();
}

void DigitMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	stateScores.clear();
	correctCommandsStateScores.clear();
	sensorActivityStateScores.clear();
	activeBitsStateScores.clear();
	totalBitsStateScore = 0;
	justReset = true;
	currentCommandRanges.clear();
	answerGiven = false;
	answerReceived = false;
	// originalCommands are taken care of in readOriginalCommands()
}

void DigitMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
	justReset = true;
	currentCommandRanges.clear();
	answerGiven = false;
	answerReceived = false;
	// originalCommands are taken care of in readOriginalCommands()
	if(visualize) {
		commandRangesTS.clear();
//		cl.logMessage("resetAfterWorldStateChange called");
	}
}

void DigitMentalImage::updateWithInputs(std::vector<double> inputs) {
	if(answerGiven)
		return;

	if(justReset)
		justReset = false;

	currentCommandRanges.clear();
	auto it = inputs.begin();
	auto digitRange = decodeMHVUInt(it, it+mnistNumBits);
	currentCommandRanges.push_back(std::make_tuple(digitRange));

	answerGiven = decodeTriggerBits(it+mnistNumBits, it+mnistNumBits+numTriggerBits);

	if(mVisualize)
		commandRangesTS.push_back(currentCommandRanges);
}

void DigitMentalImage::recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) {
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
			for(unsigned i=0; i<1; i++)
				std::cout << commandToStr(originalCommands[i]) << ( i==0 ? "" : ", " );
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

		if(integrateFitness)
			stateScores.back() += cumulativeScore/currentCommandRanges.size();
		else
			stateScores.back() = cumulativeScore/currentCommandRanges.size();

		curEval = cumulativeScore/currentCommandRanges.size();
	}

	if( answerGiven || ( (!requireTriggering) && stateTime == statePeriod-1) ) {
		correctCommandsStateScores.back() = numCorrectCommands;

		if(answerGiven)
			stateScores.back() = curEval;
		else if(integrateFitness)
			stateScores.back() /= statePeriod-1;

		sensorActivityStateScores.push_back(static_cast<double>(sensorsPtr->numSaccades())/static_cast<double>(statePeriod));
		activeBitsStateScores.push_back(sensorsPtr->numActiveStatesInRecording());
		totalBitsStateScore += sensorsPtr->numStatesInRecording();

		answerReceived = true;

		if(mVisualize) {
			std::cout << "Brain generated command ranges at the end of evaluation:";
			for(const auto& curRange : currentCommandRanges)
				std::cout << " " << commandRangeToStr(curRange);
			std::cout << std::endl;
			std::cout << "Final score: " << stateScores.back() << " final hits: " << correctCommandsStateScores.back() << std::endl << std::endl;
		}
	}
	else if(requireTriggering) {
		correctCommandsStateScores.back() = 0;
		stateScores.back() = 0.;
	}
}

void DigitMentalImage::recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void DigitMentalImage::recordSampleScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
	unsigned lineageID = org->dataMap.findKeyInData("lineageID")!=0 ? org->dataMap.getInt("lineageID") : org->ID;
	const auto& evaluationOrder = getEvaluationOrder(lineageID, stateScores.size());
	unsigned ncc = 0;
	double totSensoryActivity = 0;
	const unsigned astsTotal = stateScores.size();
	double score = 0;
	for(unsigned i=0; i<astsTotal; i++) {
		unsigned curAstIdx = evaluationOrder[i];
		double stsc = stateScores[curAstIdx];
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
	sampleScoresMap->append("fullPerceptEntropy", entropy1d(static_cast<double>(std::accumulate(activeBitsStateScores.begin(), activeBitsStateScores.end(), 0) /
	                                                                            static_cast<double>(totalBitsStateScore))));
}

void DigitMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	double score = sampleScoresMap->getAverage("score");
	double numCorrectCommands = sampleScoresMap->getAverage("numCorrectCommands");
	double sensorActivity = sampleScoresMap->getAverage("sensorActivity");
	unsigned tieredSensorActivity = static_cast<unsigned>(sensorActivity*10);
	double fullPerceptEntropy = sampleScoresMap->getAverage("fullPerceptEntropy");
	org->dataMap.append("score", score);
	org->dataMap.append("guidingFunction", -score);
	org->dataMap.append("numCorrectCommands", numCorrectCommands);
	org->dataMap.append("sensorActivity", sensorActivity);
	org->dataMap.append("tieredSensorActivity", static_cast<double>(tieredSensorActivity));
	org->dataMap.append("fullPerceptEntropy", fullPerceptEntropy);
}

int DigitMentalImage::numInputs() {
	return mnistNumBits + numTriggerBits;
}

void* DigitMentalImage::logTimeSeries(const std::string& label) {
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

/***** Private DigitMentalImage class definitions *****/

void DigitMentalImage::readOriginalCommands() {
	originalCommands.clear();
	const std::vector<std::vector<unsigned>>& commands = datasetParserPtr->cachingGetDescription(*currentAsteroidNamePtr);

	for(const auto& com : commands)
		originalCommands.push_back(std::make_tuple(com[0]));
}

const std::vector<unsigned>& DigitMentalImage::getEvaluationOrder(unsigned lineageID, unsigned numAsteroids) {
	std::map<unsigned,std::vector<unsigned>>::iterator itToEvalOrder;
	bool orderIsNew;
	std::tie(itToEvalOrder,orderIsNew) = lineageToEvaluationOrder.emplace(lineageID, numAsteroids);

	if(orderIsNew) {
		for(unsigned i=0; i<numAsteroids; i++)
			itToEvalOrder->second.at(i) = i;

		std::mt19937 rng(42*lineageID); // common RNG works only in single-threaded setting
		std::shuffle(itToEvalOrder->second.begin(), itToEvalOrder->second.end(), rng);
	}

	return itToEvalOrder->second;
}

std::tuple<double,bool> DigitMentalImage::evaluateRangeVSSet(const CommandRangeType& guessesRange) {
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
