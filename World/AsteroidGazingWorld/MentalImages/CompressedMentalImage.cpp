#include "CompressedMentalImage.h"
#include "decoders.h"

#include <fstream>
#include <cstdlib>

/***** Public CompressedMentalImage class definitions *****/

CompressedMentalImage::CompressedMentalImage(std::shared_ptr<std::string> curAstNamePtr,
                                             std::shared_ptr<AsteroidsDatasetParser> dsParserPtr,
                                             std::shared_ptr<AbstractSensors> sPtr,
                                             unsigned nBits) :
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	sensorsPtr(sPtr),
	mVisualize(Global::modePL->get() == "visualize"),
	numBits(nBits) {}
//	numTriggerBits(nTriggerBits<0 ? static_cast<unsigned>(-1*nTriggerBits) : static_cast<unsigned>(nTriggerBits)),
//	requireTriggering(nTriggerBits<0),
//	integrateFitness(intFitness),
//	answerGiven(false),
//	answerReceived(false)

void CompressedMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	stateStrings.clear();
	labeledStateStrings.clear();
	lostStates = 0;
	lostLabels = 0;
	sensorActivityStateScores.clear();
//	answerGiven = false;
//	answerReceived = false;
}

void CompressedMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
//	answerGiven = false;
//	answerReceived = false;
//	stateTS.clear();
}

void CompressedMentalImage::updateWithInputs(std::vector<double> inputs) {
//	if(answerGiven)
//		return;

	curStateString = bitRangeToHexStr(inputs.begin(), inputs.size());

	// stateTS.push_back(stateStr);
}

void CompressedMentalImage::recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) {
//	if(answerReceived)
//		return;

	if(stateTime == 0)
		readLabel();

//	std::cout << "stateTime = " << stateTime << " statePeriod = " << statePeriod;
//	std::cout << " label = " << curLabelString << " state = " << curStateString;
//	std::cout << std::endl;

	// Debug "throws"
	if(curStateString.empty()) {
		std::cerr << "Mental image evaluator got an empty state string, exiting" << std::endl;
		exit(EXIT_FAILURE);
	}

//	if( answerGiven || ( (!requireTriggering) && stateTime == statePeriod-1) ) {
	if(stateTime == statePeriod-1) {
		unsigned curNumStates = stateStrings.size();
		unsigned curNumLabeledStates = labeledStateStrings.size();
		stateStrings.insert(curStateString);
		labeledStateStrings.insert(curStateString + curLabelString);
		if(stateStrings.size()==curNumStates) {
			lostStates++;
			if(labeledStateStrings.size()!=curNumLabeledStates)
				lostLabels++;
		}

		sensorActivityStateScores.push_back(static_cast<double>(sensorsPtr->numSaccades())/static_cast<double>(statePeriod));
//		answerReceived = true;
	}
//	else if(requireTriggering) {
//		correctCommandsStateScores.back() = 0;
//		stateScores.back() = 0.;
//	}
}

void CompressedMentalImage::recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void CompressedMentalImage::recordSampleScores(std::shared_ptr<Organism> org,
                                               std::shared_ptr<DataMap> sampleScoresMap,
                                               std::shared_ptr<DataMap> runningScoresMap,
                                               int evalTime,
                                               int visualize) {
	double totSensoryActivity = 0;
	for(unsigned i=0; i<sensorActivityStateScores.size(); i++)
		totSensoryActivity += sensorActivityStateScores[i];
	totSensoryActivity /= static_cast<double>(sensorActivityStateScores.size());
	sampleScoresMap->append("lostStates", static_cast<double>(lostStates));
	sampleScoresMap->append("lostLabels", static_cast<double>(lostLabels));
	sampleScoresMap->append("sensorActivity", totSensoryActivity);
//	std::cout << "Averaged evals for org " << org->ID << std::endl;
}

void CompressedMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
//	std::cout << "Writing evals for org " << org->ID << std::endl;
	org->dataMap.append("lostStates", sampleScoresMap->getAverage("lostStates"));
	org->dataMap.append("lostLabels", sampleScoresMap->getAverage("lostLabels"));
	double sensorActivity = sampleScoresMap->getAverage("sensorActivity");
	unsigned tieredSensorActivity = static_cast<unsigned>(sensorActivity*10);
	org->dataMap.append("sensorActivity", sensorActivity);
	org->dataMap.append("tieredSensorActivity", static_cast<double>(tieredSensorActivity));
}

int CompressedMentalImage::numInputs() {
	return numBits;
//	return mnistNumBits + numTriggerBits;
}

void* CompressedMentalImage::logTimeSeries(const std::string& label) {
	std::ofstream statesLog(std::string("states_") + label + std::string(".log"));
//	for(const auto& state : stateTS)
//		stateLog << curLabelStr << bitRangeToHexStr(state.begin(), state.size()) << std::endl;
	statesLog << "not implemented" << std::endl;
	statesLog.close();
	return nullptr;
}

/***** Private DigitMentalImage class definitions *****/

void CompressedMentalImage::readLabel() {
	const std::vector<std::vector<unsigned>>& commands = datasetParserPtr->cachingGetDescription(*currentAsteroidNamePtr);
	curLabelString = std::to_string(commands[0][0]); // OK for ten digits; for higher number of labels should be fine, too, if less readable
}
