#include "CompressedMentalImage.h"
#include "decoders.h"

#include <fstream>
#include <cstdlib>
#include <cmath>

/***** Auxiliary functions *****/

template<class KeyClass, typename NumType>
void incrementMapField(std::map<KeyClass,NumType>& mymap, const KeyClass& key, NumType theIncrement = 1) {
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

double computeFuzzySharedEntropy(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                                 const std::map<std::string,unsigned>& patternCounts,
                                 const std::map<std::string,unsigned>& labelCounts,
                                 unsigned numSamples,
                                 HammingNeighborhoodGenerator& hng) {
	using namespace std;
	map<string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);
//	printMap(labelDistribution); cout << endl;

	map<pair<string,string>,double> fuzzyJoint;
	double normalizationConstant = 0.;
	for(const auto& jcpair : jointCounts) {
		string label, pattern;
		tie(label, pattern) = jcpair.first;

		incrementMapField(fuzzyJoint, jcpair.first, static_cast<double>(jcpair.second));
		normalizationConstant += static_cast<double>(jcpair.second);

		for(unsigned d=1; d<3; d++) {
			vector<string> neighborhood = hng.getNeighbors(pattern, d);
			double increment = static_cast<double>(jcpair.second)/static_cast<double>(neighborhood.size());
			for(const auto& fn : neighborhood) {
				incrementMapField(fuzzyJoint, make_pair(label, fn), increment);
				normalizationConstant += increment;
			}
		}
	}
//	cout << "Normalization const is " << scientific << normalizationConstant << " while num samples is " << numSamples << endl;
	for(auto& fjpair : fuzzyJoint)
		fjpair.second /= normalizationConstant;
//	printMap(fuzzyJoint); cout << endl;


	map<string,double> fuzzyPatterns;
	for(const auto& fjpair : fuzzyJoint)
		incrementMapField(fuzzyPatterns, fjpair.first.second, fjpair.second);
//	printMap(fuzzyPatterns); cout << endl;

	double patternLabelInfo = 0.;
	for(const auto& fjpair : fuzzyJoint) {
		string label, pattern;
		tie(label, pattern) = fjpair.first;
		patternLabelInfo += fjpair.second*log10(fjpair.second/(labelDistribution[label]*fuzzyPatterns[pattern]));
	}
	return patternLabelInfo;
}

double computeRepellingSharedEntropy(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                                     const std::map<std::string,unsigned>& patternCounts,
                                     const std::map<std::string,unsigned>& labelCounts,
                                     unsigned numSamples) {
	using namespace std;
	map<string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);
//	printMap(labelDistribution); cout << endl;

	map<pair<string,string>,double> repellingJoint;
	double normalizationConstant = 0.;
	for(const auto& jcpair : jointCounts) {
		string label, pattern;
		tie(label, pattern) = jcpair.first;

		incrementMapField(repellingJoint, jcpair.first, static_cast<double>(jcpair.second));
		normalizationConstant += static_cast<double>(jcpair.second);

		for(const auto& otherJCPair : jointCounts) {
			string otherLabel, otherPattern;
			tie(otherLabel, otherPattern) = otherJCPair.first;
			if(otherPattern==pattern)
				continue;

			double dist = static_cast<double>(hexStringHammingDistance(pattern, otherPattern));
			double crosstalk = static_cast<double>(jcpair.second)*exp(-dist);
			incrementMapField(repellingJoint, make_pair(label, otherPattern), crosstalk);
			normalizationConstant += crosstalk;
		}
	}
//	cout << "Normalization const is " << scientific << normalizationConstant << " while num samples is " << numSamples << endl;
	for(auto& rjpair : repellingJoint)
		rjpair.second /= normalizationConstant;
//	printMap(repellingJoint); cout << endl;


	map<string,double> repellingPatterns;
	for(const auto& rjpair : repellingJoint)
		incrementMapField(repellingPatterns, rjpair.first.second, rjpair.second);
//	printMap(repellingPatterns); cout << endl;

	double patternLabelInfo = 0.;
	for(const auto& rjpair : repellingJoint) {
		string label, pattern;
		tie(label, pattern) = rjpair.first;
		patternLabelInfo += rjpair.second*log10(rjpair.second/(labelDistribution[label]*repellingPatterns[pattern]));
	}
	return patternLabelInfo;
}

double computeSharedEntropy(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                            const std::map<std::string,unsigned>& patternCounts,
                            const std::map<std::string,unsigned>& labelCounts,
                            unsigned numSamples) {
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
	return patternLabelInfo;
}

std::map<std::string,double> computeLabelConditionalEntropy(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                                                            const std::map<std::string,unsigned>& labelCounts) {
	std::map<std::string,double> lcEntropy;
//	std::cout << "Class conditional pattern distributions (histograms):" << std::endl;
	for(const auto& lcpair : labelCounts) {
		std::map<std::string,unsigned> condPatternCounts;
		for(const auto& jcpair : jointCounts)
			if(jcpair.first.first==lcpair.first)
				incrementMapField(condPatternCounts, jcpair.first.second, jcpair.second);

//		std::cout << "For label " << lcpair.first << ": ";
//		printMap(condPatternCounts);

		lcEntropy[lcpair.first] = 0.;
		for(const auto& cpcpair : condPatternCounts) {
			double patProbGivenLabel = static_cast<double>(cpcpair.second)/static_cast<double>(lcpair.second);
			lcEntropy[lcpair.first] -= patProbGivenLabel*log10(patProbGivenLabel);
		}
	}
//	std::cout << std::endl;
	return lcEntropy;
}

double computeAverageLabelConditionalEntropy(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                                          const std::map<std::string,unsigned>& labelCounts,
                                          unsigned numSamples) {
	std::map<std::string,double> labelConditionalEntropy = computeLabelConditionalEntropy(jointCounts, labelCounts);

//	std::cout << "Computed label conditional entropies:" << std::endl;
//	printMap(labelConditionalEntropy);
//	std::cout << std::endl;

	double fullLabelConditional = 0.;
	for(const auto& lcpair : labelCounts)
		fullLabelConditional += static_cast<double>(lcpair.second)*labelConditionalEntropy[lcpair.first]/static_cast<double>(numSamples);
	return fullLabelConditional;
}

std::map<std::string,std::string> makeNaiveClassifier(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                                                      const std::map<std::string,unsigned>& patternCounts) {
	std::map<std::string,std::string> decipherer;
	long unsigned tieBreaker = 0;
	for(const auto& ppair : patternCounts) {
		std::string pattern = ppair.first;
		std::map<std::string,unsigned> condLabelCounts;
		for(const auto& jppair : jointCounts)
			if(jppair.first.second == pattern)
				incrementMapField(condLabelCounts, jppair.first.first, jppair.second);

//		std::cout << pattern << ":" << std::endl;
//		printMap(condLabelCounts);
//		std::cout << std::endl;

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
	return decipherer;
}

void saveClassifier(const std::map<std::string,std::string>& patternsToLabels,
                    std::string filename) {
	std::ofstream classifierFile(filename, std::ofstream::out);
	for(const auto& plpair : patternsToLabels)
		classifierFile << plpair.first << " " << plpair.second << std::endl;
	classifierFile.close();
}

std::map<std::string,std::string> loadClassifier(std::string filename) {
	std::map<std::string,std::string> theClassifier;
	std::string pattern, label;
	std::ifstream classifierFile(filename);
	while(classifierFile >> pattern >> label)
		theClassifier[pattern] = label;
	return theClassifier;
}

std::string labelOfClosestNeighbor(std::string pattern, const std::map<std::string,std::string>& patternsToLabels) {
	// Hamming distance is used
	unsigned minDist = pattern.size()*4;
	std::string outPattern;
	for(const auto& plpair : patternsToLabels) {
		unsigned curDist = hexStringHammingDistance(pattern, plpair.first);
		if(curDist<=minDist) { // TODO: maybe add meaningful tie breaking here, idk
			minDist = curDist;
			outPattern = plpair.second;
		}
	}
	return outPattern;
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
	numBits(nBits),
	hngen(nBits) {}
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

	sampleScoresMap->append("repellingPatternLabelInformation", computeRepellingSharedEntropy(jointCounts, patternCounts, labelCounts, numSamples));
//	sampleScoresMap->append("fuzzyPatternLabelInformation", computeFuzzySharedEntropy(jointCounts, patternCounts, labelCounts, numSamples, hngen));
	sampleScoresMap->append("patternLabelInformation", computeSharedEntropy(jointCounts, patternCounts, labelCounts, numSamples));
	sampleScoresMap->append("averageLabelConditionalEntropy", computeAverageLabelConditionalEntropy(jointCounts, labelCounts, numSamples));
	if(mVisualize) {
		std::map<std::string,std::string> currentDecipherer = makeNaiveClassifier(jointCounts, patternCounts);

//		saveClassifier(currentDecipherer, "decipherer.log");
//		std::map<std::string,std::string> decipherer = loadClassifier("decipherer.log");
		std::map<std::string,std::string> decipherer = currentDecipherer;

		long unsigned successfulTrials = 0;
		long unsigned totalTrials = 0;
		long unsigned unknownPattern = 0;
		for(const auto& jppair : jointCounts) {
			std::string label, pattern;
			std::tie(label, pattern) = jppair.first;
//			if(decipherer.find(pattern)==decipherer.end())
//				unknownPattern += jppair.second;
//			else if(decipherer[pattern]==label)
//				successfulTrials += jppair.second;
			if(labelOfClosestNeighbor(pattern, decipherer)==label)
				successfulTrials += jppair.second;
			totalTrials += jppair.second;
		}
		std::cout << "Out of " << totalTrials << " trials " << successfulTrials << " were successful and " << unknownPattern << " yielded unknown patterns" << std::endl;
	}

	sampleScoresMap->append("sensorActivity", totSensoryActivity);
}

void CompressedMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
//	std::cout << "Writing evals for org " << org->ID << std::endl;
	org->dataMap.append("lostStates", sampleScoresMap->getAverage("lostStates"));
	org->dataMap.append("lostLabels", sampleScoresMap->getAverage("lostLabels"));

//	org->dataMap.append("repellingPatternLabelInformation", sampleScoresMap->getAverage("repellingPatternLabelInformation"));
//	org->dataMap.append("fuzzyPatternLabelInformation", sampleScoresMap->getAverage("fuzzyPatternLabelInformation"));
	org->dataMap.append("patternLabelInformation", sampleScoresMap->getAverage("patternLabelInformation"));
	org->dataMap.append("averageLabelConditionalEntropy", sampleScoresMap->getAverage("averageLabelConditionalEntropy"));

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
