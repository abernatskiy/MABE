#include "CompressedMentalImage.h"
#include "decoders.h"

#include <fstream>
#include <cstdlib>
#include <cmath>

template<class T>
void incrementMapField(std::map<T,unsigned>& mymap, const T& key, int theIncrement = 1) {
	if(mymap.find(key)==mymap.end())
		mymap[key] = theIncrement;
	else
		mymap[key] += theIncrement;
}

template<class NumType>
void printMap(const std::map<std::string,NumType>& mymap) {
	unsigned st = 0;
	for(const auto& mpair : mymap)
		std::cout << (st++==0?"":" ") << mpair.first << ":" << mpair.second;
	std::cout << std::endl;
}

template<class NumType>
void printMap(const std::map<std::pair<std::string,std::string>,NumType>& mymap) {
	unsigned st = 0;
	for(const auto& mpair : mymap)
		std::cout << (st++==0?"(":" (") << mpair.first.first << "," << mpair.first.second << "):" << mpair.second;
	std::cout << std::endl;
}

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

	labelCounts.clear();
	patternCounts.clear();
	jointCounts.clear();
	numSamples = 0;

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
/*		std::cout << "state sets before adding " << curStateString << " with label " << curLabelString << std::endl;
		for(const auto& ss : stateStrings)
			std::cout << " " << ss;
		std::cout << std::endl;
		for(const auto& lss : labeledStateStrings)
			std::cout << " " << lss;
		std::cout << std::endl;
*/
		unsigned curNumStates = stateStrings.size();
		unsigned curNumLabeledStates = labeledStateStrings.size();
		stateStrings.insert(curStateString);
		labeledStateStrings.insert(curStateString + curLabelString);
		if(stateStrings.size()==curNumStates) {
			lostStates++;
//			std::cout << "Incremented lost states, now it is " << lostStates << std::endl;
			if(labeledStateStrings.size()!=curNumLabeledStates) {
				lostLabels++;
//				std::cout << "Incremented lost labels, now it is " << lostLabels << std::endl;
			}
		}

		incrementMapField(labelCounts, curLabelString);
		incrementMapField(patternCounts, curStateString);
		incrementMapField(jointCounts, std::make_pair(curLabelString, curStateString));
		numSamples++;

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

	std::map<std::string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);
//	printMap(labelDistribution); std::cout << std::endl;
	std::map<std::string,double> patternDistribution;
	for(const auto& ppair : patternCounts)
		patternDistribution[ppair.first] = static_cast<double>(ppair.second)/static_cast<double>(numSamples);
//	printMap(patternDistribution); std::cout << std::endl;
//	printMap(jointCounts); std::cout << std::endl;
	double patternLabelInfo = 0.;
	for(const auto& labpatpair : jointCounts) {
		std::string label, pattern;
		std::tie(label, pattern) = labpatpair.first;
		double jp = static_cast<double>(labpatpair.second)/static_cast<double>(numSamples);
		patternLabelInfo += jp*log10(jp/(labelDistribution[label]*patternDistribution[pattern]));
	}
	sampleScoresMap->append("patternLabelInformation", patternLabelInfo);
	if(mVisualize) {
		std::map<std::string,std::string> decipherer;
		long unsigned tieBreaker = 0;
		for(const auto& ppair : patternCounts) {
			std::string pattern = ppair.first;
			std::map<std::string,unsigned> condLabelCounts;
			for(const auto& jppair : jointCounts)
				if(jppair.first.second == pattern)
					incrementMapField(condLabelCounts, jppair.first.first, jppair.second);

//			std::cout << pattern << ":" << std::endl;
//			printMap(condLabelCounts);
//			std::cout << std::endl;

			unsigned maxCount = 0;
			std::string bestLabel;
			for(const auto& clcpair : condLabelCounts) {
				if(clcpair.second>maxCount) {
					maxCount = clcpair.second;
					bestLabel = clcpair.first;
				}
				else if(tieBreaker%2==0 && clcpair.second==maxCount) { // I alternate the direction of tie breaking to minimize biases while preserving determinism
					bestLabel = clcpair.first;
					tieBreaker++;
				}
			}
			decipherer[pattern] = bestLabel;
		}

		long unsigned successfulTrials = 0;
		long unsigned totalTrials = 0;
		for(const auto& jppair : jointCounts) {
			std::string label, pattern;
			std::tie(label, pattern) = jppair.first;
			if(decipherer[pattern]==label)
				successfulTrials += jppair.second;
			totalTrials += jppair.second;
		}

		std::cout << successfulTrials << " trials successful out of " << totalTrials << std::endl;
	}

	sampleScoresMap->append("sensorActivity", totSensoryActivity);
}

void CompressedMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
//	std::cout << "Writing evals for org " << org->ID << std::endl;
	org->dataMap.append("lostStates", sampleScoresMap->getAverage("lostStates"));
	org->dataMap.append("lostLabels", sampleScoresMap->getAverage("lostLabels"));

	org->dataMap.append("patternLabelInformation", sampleScoresMap->getAverage("patternLabelInformation"));

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
