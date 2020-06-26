#include <fstream>
#include <cstdlib>
#include <cmath>

#include "CompressedMentalImage.h"
#include "decoders.h"
#include "../../../Utilities/texture.h"

using namespace std;

/*****************************************/
/********** Auxiliary functions **********/
/*****************************************/

template<class KeyClass, typename NumType>
void incrementMapField(map<KeyClass,NumType>& mymap, const KeyClass& key, NumType theIncrement = 1) {
	if(mymap.find(key)==mymap.end())
		mymap[key] = theIncrement;
	else
		mymap[key] += theIncrement;
}

template<class NumType>
void printMap(const map<string,NumType>& mymap) {
	unsigned st = 0;
	for(const auto& mpair : mymap)
		cout << (st++==0?"":" ") << mpair.first << ":" << mpair.second;
	cout << endl;
}

template<class NumType>
void printMap(const map<pair<string,string>,NumType>& mymap) {
	unsigned st = 0;
	for(const auto& mpair : mymap)
		cout << (st++==0?"(":" (") << mpair.first.first << "," << mpair.first.second << "):" << mpair.second;
	cout << endl;
}

template<class NumType>
void printPatternConditionals(const map<pair<string,string>,NumType>& joint) {
	map<string,map<string,NumType>> patternConditionals;
	for(const auto& mpair : joint) {
		string pattern = mpair.first.second;
		auto itCurConditional = patternConditionals.find(pattern);
		if(itCurConditional==patternConditionals.end()) {
			patternConditionals[pattern] = {};
			itCurConditional = patternConditionals.find(pattern);
		}
		string label = mpair.first.first;
		auto itCurField = itCurConditional->second.find(label);
		if(itCurField==itCurConditional->second.end()) {
			itCurConditional->second.emplace(label, 0);
			itCurField = itCurConditional->second.find(label);
		}
		itCurField->second += mpair.second;
	}

	for(const auto& patpair : patternConditionals) {
		cout << patpair.first << ":";
		for(const auto& labpair : patpair.second)
			cout << " " << labpair.first << "-" << labpair.second;
		cout << endl;
	}
}

double computeFuzzySharedEntropy(const map<pair<string,string>,unsigned>& jointCounts,
                                 const map<string,unsigned>& patternCounts,
                                 const map<string,unsigned>& labelCounts,
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

double computeSharedEntropy(const map<pair<string,string>,unsigned>& jointCounts,
                            const map<string,unsigned>& patternCounts,
                            const map<string,unsigned>& labelCounts,
                            unsigned numSamples) {
//	cout << "computeSharedEntropy called on JC at " << &jointCounts << " PC at " << &patternCounts << " and LC at " << &labelCounts << ", numSamples is " << numSamples << endl;
	map<string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);
//	printMap(labelDistribution); cout << endl;
	map<string,double> patternDistribution;
	for(const auto& ppair : patternCounts)
		patternDistribution[ppair.first] = static_cast<double>(ppair.second)/static_cast<double>(numSamples);
//	printMap(patternDistribution); cout << endl;
//	printMap(jointCounts); cout << endl;
	double patternLabelInfo = 0.;
	for(const auto& labpatpair : jointCounts) {
		string label, pattern;
		tie(label, pattern) = labpatpair.first;
		double jp = static_cast<double>(labpatpair.second)/static_cast<double>(numSamples);
		patternLabelInfo += jp*log10(jp/(labelDistribution[label]*patternDistribution[pattern]));
	}
	return patternLabelInfo;
}

map<string,double> computeLabelConditionalEntropy(const map<pair<string,string>,unsigned>& jointCounts,
                                                            const map<string,unsigned>& labelCounts) {
	map<string,double> lcEntropy;
//	cout << "Class conditional pattern distributions (histograms):" << endl;
	for(const auto& lcpair : labelCounts) {
		map<string,unsigned> condPatternCounts;
		for(const auto& jcpair : jointCounts)
			if(jcpair.first.first==lcpair.first)
				incrementMapField(condPatternCounts, jcpair.first.second, jcpair.second);

//		cout << "For label " << lcpair.first << ": ";
//		printMap(condPatternCounts);

		lcEntropy[lcpair.first] = 0.;
		for(const auto& cpcpair : condPatternCounts) {
			double patProbGivenLabel = static_cast<double>(cpcpair.second)/static_cast<double>(lcpair.second);
			lcEntropy[lcpair.first] -= patProbGivenLabel*log10(patProbGivenLabel);
		}
	}
//	cout << endl;
	return lcEntropy;
}

double computeAverageLabelConditionalEntropy(const map<pair<string,string>,unsigned>& jointCounts,
                                          const map<string,unsigned>& labelCounts,
                                          unsigned numSamples) {
	map<string,double> labelConditionalEntropy = computeLabelConditionalEntropy(jointCounts, labelCounts);

//	cout << "Computed label conditional entropies:" << endl;
//	printMap(labelConditionalEntropy);
//	cout << endl;

	double fullLabelConditional = 0.;
	for(const auto& lcpair : labelCounts)
		fullLabelConditional += static_cast<double>(lcpair.second)*labelConditionalEntropy[lcpair.first]/static_cast<double>(numSamples);
	return fullLabelConditional;
}

map<string,string> makeNaiveClassifier(const map<pair<string,string>,unsigned>& jointCounts,
                                                      const map<string,unsigned>& patternCounts) {
	map<string,string> decipherer;
	long unsigned tieBreaker = 0;
	for(const auto& ppair : patternCounts) {
		string pattern = ppair.first;
		map<string,unsigned> condLabelCounts;
		for(const auto& jppair : jointCounts)
			if(jppair.first.second == pattern)
				incrementMapField(condLabelCounts, jppair.first.first, jppair.second);

//		cout << pattern << ":" << endl;
//		printMap(condLabelCounts);
//		cout << endl;

		unsigned maxCount = 0;
		string bestLabel;
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

void saveClassifier(const map<string,string>& patternsToLabels,
                    string filename) {
	ofstream classifierFile(filename, ofstream::out);
	for(const auto& plpair : patternsToLabels)
		classifierFile << plpair.first << " " << plpair.second << endl;
	classifierFile.close();
}

map<string,string> loadClassifier(string filename) {
	map<string,string> theClassifier;
	string pattern, label;
	ifstream classifierFile(filename);
	while(classifierFile >> pattern >> label)
		theClassifier[pattern] = label;
	return theClassifier;
}

string labelOfClosestNeighbor(string pattern, const map<string,string>& patternsToLabels) {
	// Hamming distance is used
	unsigned minDist = pattern.size()*4;
	string outPattern;
	for(const auto& plpair : patternsToLabels) {
		unsigned curDist = hexStringHammingDistance(pattern, plpair.first);
		if(curDist<=minDist) { // TODO: maybe add meaningful tie breaking here, idk
			minDist = curDist;
			outPattern = plpair.second;
		}
	}
	return outPattern;
}

tuple<unsigned, long unsigned> oneHotMatches(int curLabel, Texture& pattern, size_t popsize=1) {
	// the first returned value is one if the pattern matched the label exactly and zero otherwise
	// the second returned value is the number of matched bits
	if(curLabel<0 || curLabel>=10) throw invalid_argument("Bit matching function caught an invalid label " + to_string(curLabel));
	if(pattern.shape()[0]!=1 ||
	   pattern.shape()[1]!=1 ||
	   pattern.shape()[2]!=1 ) throw invalid_argument("Bit matches are counted for a texture with spatiotemporal dimensions greater than one");
	if(pattern.shape()[3]!=10*popsize) throw invalid_argument("Bit matches are counted with a texture with a number of channels other than ten time the number of populations, w.r.t. one hot encoding, exiting");

//	cout << "matching " << curLabel << " and texture" << endl << readableRepr(pattern)";

	unsigned bitMatches = 0;
	unsigned popMatches = 0;
	for(size_t bi=0; bi<10; bi++) { // bit index
		int consensus = 0;
		for(size_t pi=0; pi<popsize; pi++) {
			// Positive output favored
			consensus += pattern[0][0][0][bi*popsize+pi]==0 ? -1 : 1;
			bitMatches += curLabel==bi ? pattern[0][0][0][bi*popsize+pi]!=0 : pattern[0][0][0][bi*popsize+pi]==0;

			// Negative output favored
			// consensus += pattern[0][0][0][bi*popsize+pi]==0 ? 1 : -1;
			// bitMatches += curLabel==bi ? pattern[0][0][0][bi*popsize+pi]==0 : pattern[0][0][0][bi*popsize+pi]!=0;
		}
		popMatches += curLabel==bi ? consensus>0 : consensus<=0;
	}

//	cout << endl << "result: " << bitMatches << "bit" << endl;

	return tuple<unsigned, long unsigned>(popMatches==10, bitMatches);
}

/********************************************************************/
/********** Public CompressedMentalImage class definitions **********/
/********************************************************************/

CompressedMentalImage::CompressedMentalImage(shared_ptr<string> curAstNamePtr,
                                             shared_ptr<AsteroidsDatasetParser> dsParserPtr,
                                             shared_ptr<AbstractSensors> sPtr,
                                             unsigned nBits,
                                             bool computeFRPLInfo,
                                             unsigned patternChunkSize,
                                             unsigned nNeighbors,
                                             double leakBaseMult,
                                             double leakDecayRad,
                                             bool textureInput,
                                             bool overwriteEvals) :
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	sensorsPtr(sPtr),
	curLabel(-1),
	lostStates(0),
	lostLabels(0),
	matchedBits(0),
	matchedPatterns(0),
	mVisualize(Global::modePL->get() == "visualize"),
	numBits(nBits),
	computeFastRepellingPLInfo(computeFRPLInfo),
	hngen(nBits),
	neighborsdb(nBits, patternChunkSize),
	numNeighbors(nNeighbors),
	leakBaseMultiplier(leakBaseMult),
	leakDecayRadius(leakDecayRad),
	inputIsATexture(textureInput),
	numErasures(0) {

	overwriteEvaluations = overwriteEvals;

	if(leakDecayRadius==0) {
		cerr << "Leak decay radius cannot be zero" << endl;
		exit(EXIT_FAILURE);
	}
}
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

	matchedBits = 0;
	matchedPatterns = 0;

	labelCounts.clear();
	patternCounts.clear();
	jointCounts.clear();
	numSamples = 0;

	numErasures = 0;

	sensorActivityStateScores.clear();
//	answerGiven = false;
//	answerReceived = false;
}

void CompressedMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
//	answerGiven = false;
//	answerReceived = false;
//	stateTS.clear();
}

void CompressedMentalImage::updateWithInputs(vector<double> inputs) {
//	if(answerGiven)
//		return;
	if(!inputIsATexture)
		curInput = inputs;
}

void CompressedMentalImage::recordRunningScoresWithinState(shared_ptr<Organism> org, int stateTime, int statePeriod) {
//	if(answerReceived)
//		return;

	if(stateTime == statePeriod-1) {
		readLabel();

		curStateStrings.clear();
		if(inputIsATexture) {
			auto outTexturesPtr = reinterpret_cast<vector<void*>*>(brain->getDataForMotors());
			for(size_t i=0; i<outTexturesPtr->size(); i++) {
				auto texturePtr = reinterpret_cast<Texture*>(outTexturesPtr->at(i));
				curStateStrings.push_back(textureToHexStr(texturePtr));
//				if(i==0) {
//					const unsigned labelPopulationSize = 1;
//					tuple<unsigned, long unsigned> matchCounts = oneHotMatches(curLabel, *texturePtr, labelPopulationSize);
//					matchedPatterns += get<0>(matchCounts);
//					matchedBits += get<1>(matchCounts);
//				}
			}
		}
		else
			curStateStrings.push_back(bitRangeToHexStr(curInput.begin(), curInput.size()));

		unsigned curNumStates = stateStrings.size();
		unsigned curNumLabeledStates = labeledStateStrings.size();
		stateStrings.insert(curStateStrings.front());
		labeledStateStrings.insert(curStateStrings.front() + curLabelString);
		if(stateStrings.size()==curNumStates) {
			lostStates++;
//			cout << "Incremented lost states, now it is " << lostStates << endl;
			if(labeledStateStrings.size()!=curNumLabeledStates) {
				lostLabels++;
//				cout << "Incremented lost labels, now it is " << lostLabels << endl;
			}
		}

		incrementMapField(labelCounts, curLabelString);

		unsigned numTextures = curStateStrings.size();
		if(patternCounts.size()<numTextures)
			patternCounts.resize(numTextures);
		if(jointCounts.size()<numTextures)
			jointCounts.resize(numTextures);
		for(unsigned i=0; i<numTextures; i++) {
			incrementMapField(patternCounts[i], curStateStrings[i]);
			incrementMapField(jointCounts[i], make_pair(curLabelString, curStateStrings[i]));
		}
		numSamples++;

		nlohmann::json brainStats = brain->getPostEvaluationStats();
		long stateErasures = stol(brainStats["erasures"].get<string>());
		numErasures += stateErasures;

		sensorActivityStateScores.push_back(static_cast<double>(sensorsPtr->numSaccades())/static_cast<double>(statePeriod));
//		answerReceived = true;
	}
//	else if(requireTriggering) {
//		correctCommandsStateScores.back() = 0;
//		stateScores.back() = 0.;
//	}
}

void CompressedMentalImage::recordRunningScores(shared_ptr<Organism> org, shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void CompressedMentalImage::recordSampleScores(shared_ptr<Organism> org,
                                               shared_ptr<DataMap> sampleScoresMap,
                                               shared_ptr<DataMap> runningScoresMap,
                                               int evalTime,
                                               int visualize) {
	double totSensoryActivity = 0;
	for(unsigned i=0; i<sensorActivityStateScores.size(); i++)
		totSensoryActivity += sensorActivityStateScores[i];
	totSensoryActivity /= static_cast<double>(sensorActivityStateScores.size());
	sampleScoresMap->append("lostStates", static_cast<double>(lostStates));
	sampleScoresMap->append("lostLabels", static_cast<double>(lostLabels));
	sampleScoresMap->append("matchedBits", static_cast<double>(matchedBits));
	sampleScoresMap->append("matchedPatterns", static_cast<double>(matchedPatterns));
	sampleScoresMap->append("numPatterns", static_cast<double>(stateStrings.size()));
	sampleScoresMap->append("numLabeledPatterns", static_cast<double>(labeledStateStrings.size()));
	sampleScoresMap->append("erasures", static_cast<double>(numErasures));

//	if(computeFastRepellingPLInfo) sampleScoresMap->append("fastRepellingPatternLabelInformation", computeFastRepellingSharedEntropy());
//	sampleScoresMap->append("repellingPatternLabelInformation", computeRepellingSharedEntropy());
//	sampleScoresMap->append("fuzzyPatternLabelInformation", computeFuzzySharedEntropy(jointCounts, patternCounts, labelCounts, numSamples, hngen));

	for(unsigned iri=0; iri<jointCounts.size(); iri++) {
		string infoName = "plInfo_layer" + to_string(iri);
		double info = computeSharedEntropy(jointCounts.at(iri), patternCounts.at(iri), labelCounts, numSamples);
		sampleScoresMap->append(infoName, info);

		string entroName = "lcpe_layer" + to_string(iri);
		//cout << "Computing lcpe for layer " << iri << endl;
		//printPatternConditionals(rangesJointCounts.at(iri));
		double entro = computeAverageLabelConditionalEntropy(jointCounts.at(iri), labelCounts, numSamples);
		sampleScoresMap->append(entroName, entro);
	}

	if(mVisualize) {
		map<string,string> currentDecipherer = makeNaiveClassifier(jointCounts.front(), patternCounts.front());

//		saveClassifier(currentDecipherer, "decipherer.log");
//		map<string,string> decipherer = loadClassifier("decipherer.log");
		map<string,string> decipherer = currentDecipherer;

		long unsigned successfulTrials = 0;
		long unsigned totalTrials = 0;
		long unsigned unknownPattern = 0;
		for(const auto& jppair : jointCounts.front()) {
			string label, pattern;
			tie(label, pattern) = jppair.first;
//			if(decipherer.find(pattern)==decipherer.end())
//				unknownPattern += jppair.second;
//			else if(decipherer[pattern]==label)
//				successfulTrials += jppair.second;
			if(labelOfClosestNeighbor(pattern, decipherer)==label)
				successfulTrials += jppair.second;
			totalTrials += jppair.second;
		}
		cout << "Out of " << totalTrials << " trials " << successfulTrials << " were successful and " << unknownPattern << " yielded unknown patterns" << endl;
	}

	sampleScoresMap->append("sensorActivity", totSensoryActivity);
}

void CompressedMentalImage::evaluateOrganism(shared_ptr<Organism> org, shared_ptr<DataMap> sampleScoresMap, int visualize) {
//	cout << "Writing evals for org " << org->ID << endl;
	updateOrgDatamap(org, "lostStates", sampleScoresMap->getAverage("lostStates"));
	updateOrgDatamap(org, "lostLabels", sampleScoresMap->getAverage("lostLabels"));
	updateOrgDatamap(org, "matchedBits", sampleScoresMap->getAverage("matchedBits"));
	updateOrgDatamap(org, "matchedPatterns", sampleScoresMap->getAverage("matchedPatterns"));
	updateOrgDatamap(org, "numPatterns", sampleScoresMap->getAverage("numPatterns"));
	updateOrgDatamap(org, "numLabeledPatterns", sampleScoresMap->getAverage("numLabeledPatterns"));
	updateOrgDatamap(org, "erasures", sampleScoresMap->getAverage("erasures"));

//	if(computeFastRepellingPLInfo) org->dataMap.append("fastRepellingPatternLabelInformation", sampleScoresMap->getAverage("fastRepellingPatternLabelInformation"));
//	updateOrgDatamap(org, "repellingPatternLabelInformation", sampleScoresMap->getAverage("repellingPatternLabelInformation"));
//	updateOrgDatamap(org, "fuzzyPatternLabelInformation", sampleScoresMap->getAverage("fuzzyPatternLabelInformation"));
	double irs = 0.;
	double ers = 0.;
	for(unsigned iri=0; iri<jointCounts.size(); iri++) {
		string infoName = "plInfo_layer" + to_string(iri);
		updateOrgDatamap(org, infoName, sampleScoresMap->getAverage(infoName));

		string entroName = "lcpe_layer" + to_string(iri);
		updateOrgDatamap(org, entroName, sampleScoresMap->getAverage(entroName));

		if(iri!=jointCounts.size()-1) {
			irs += sampleScoresMap->getAverage(infoName);
			ers += sampleScoresMap->getAverage(entroName);
		}
	}
	updateOrgDatamap(org, "plInfo_allProcessedLayers", irs);
	updateOrgDatamap(org, "lcpe_allProcessedLayers", ers);

	double sensorActivity = sampleScoresMap->getAverage("sensorActivity");
	unsigned tieredSensorActivity = static_cast<unsigned>(sensorActivity*10);
	updateOrgDatamap(org, "sensorActivity", sensorActivity);
	updateOrgDatamap(org, "tieredSensorActivity", static_cast<double>(tieredSensorActivity));
}

int CompressedMentalImage::numInputs() {
	if(inputIsATexture)
		return 0;
	else
		return numBits;
//	return mnistNumBits + numTriggerBits;
}

void* CompressedMentalImage::logTimeSeries(const string& label) {
/*
	ofstream statesLog(string("states_") + label + string(".log"));
//	for(const auto& state : stateTS)
//		stateLog << curLabelStr << bitRangeToHexStr(state.begin(), state.size()) << endl;
	statesLog << "not implemented" << endl;
	statesLog.close();
*/
	return nullptr;
}

/***** Private CompressedMentalImage class definitions *****/

void CompressedMentalImage::readLabel() {
	const vector<vector<double>>& commands = datasetParserPtr->cachingGetDescription(*currentAsteroidNamePtr);
	curLabel = static_cast<int>(commands[0][0]);
	curLabelString = to_string(curLabel);
}

double CompressedMentalImage::computeRepellingSharedEntropy() {
	using namespace std;

//	cout << "Raw pattern conditional label distributions:" << endl; printPatternConditionals(jointCounts); cout << endl;

	map<string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);
//	printMap(labelDistribution); cout << endl;

	map<pair<string,string>,double> repellingJoint;
	double normalizationConstant = 0.;
	for(const auto& jcpair : jointCounts.front()) {
		string label, pattern;
		tie(label, pattern) = jcpair.first;

		incrementMapField(repellingJoint, jcpair.first, static_cast<double>(jcpair.second));
		normalizationConstant += static_cast<double>(jcpair.second);

		for(const auto& otherJCPair : jointCounts.front()) {
			string otherLabel, otherPattern;
			tie(otherLabel, otherPattern) = otherJCPair.first;
			if(otherPattern==pattern && otherLabel==label) continue;

			double dist = static_cast<double>(hexStringHammingDistance(pattern, otherPattern));
			double crosstalk = static_cast<double>(jcpair.second) * leakBaseMultiplier * exp(-static_cast<double>(dist)/leakDecayRadius);
//			cout << "Crosstalk from (" << label << ", " << pattern << ") to (" << otherLabel << ", " << otherPattern << ") is " << crosstalk << " (added to " << label << ", " << otherPattern << ")" << endl;
			incrementMapField(repellingJoint, make_pair(label, otherPattern), crosstalk);
			normalizationConstant += crosstalk;
		}
	}

//	cout << "Leaky pattern conditional label distributions:" << endl; printPatternConditionals(repellingJoint); cout << endl;

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

double CompressedMentalImage::computeFastRepellingSharedEntropy() {
	using namespace std;

//	cout << "Raw pattern conditional label distributions:" << endl; printPatternConditionals(jointCounts); cout << endl;

	map<string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);

	neighborsdb.index(jointCounts.front());
	map<pair<string,string>,double> repellingJoint;
	double normalizationConstant = 0.;

	for(const auto& jcpair : jointCounts.front()) {
		string label, pattern;
		tie(label, pattern) = jcpair.first;

		incrementMapField(repellingJoint, jcpair.first, static_cast<double>(jcpair.second));
		normalizationConstant += static_cast<double>(jcpair.second);

		for(auto neighborInfo : neighborsdb.getSomeNeighbors(pattern, numNeighbors+1)) {
			string otherLabel, otherPattern;
			unsigned otherCount;
			size_t otherDistance;
			tie(otherLabel, otherPattern, otherCount, otherDistance) = neighborInfo;
			if(otherPattern==pattern && otherLabel==label) continue;

			double dist = static_cast<double>(otherDistance);
			double crosstalk = static_cast<double>(otherCount) * leakBaseMultiplier * exp(-static_cast<double>(dist)/leakDecayRadius);
//			cout << "Crosstalk from (" << label << ", " << pattern << ") to (" << otherLabel << ", " << otherPattern << ") is " << crosstalk << " (added to " << label << ", " << otherPattern << ")" << endl;
			incrementMapField(repellingJoint, make_pair(otherLabel, pattern), crosstalk);
			normalizationConstant += crosstalk;
		}
	}

//	cout << "Leaky pattern conditional label distributions:" << endl; printPatternConditionals(repellingJoint); cout << endl;

	for(auto& rjpair : repellingJoint)
		rjpair.second /= normalizationConstant;

	map<string,double> repellingPatterns;
	for(const auto& rjpair : repellingJoint)
		incrementMapField(repellingPatterns, rjpair.first.second, rjpair.second);

	double patternLabelInfo = 0.;
	for(const auto& rjpair : repellingJoint) {
		string label, pattern;
		tie(label, pattern) = rjpair.first;
		patternLabelInfo += rjpair.second*log10(rjpair.second/(labelDistribution[label]*repellingPatterns[pattern]));
	}
	return patternLabelInfo;
}
