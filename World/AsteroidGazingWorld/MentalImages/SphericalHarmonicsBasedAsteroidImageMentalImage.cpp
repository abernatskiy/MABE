#include "SphericalHarmonicsBasedAsteroidImageMentalImage.h"

#include <algorithm>

// Position-based decoders

unsigned decodeUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	return std::distance(begin, std::max_element(begin, end));
}

int decodeSInt(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	int offsetArgmax = std::distance(begin+1, std::max_element(begin+1, end));
	return (*begin)<0.5 ? offsetArgmax : -1*offsetArgmax;
}

double decodeDouble(std::vector<double>::iterator begin, std::vector<double>::iterator end) {
	return static_cast<double>(std::distance(begin, std::max_element(begin, end))) / static_cast<double>(std::distance(begin, end));
}

SphericalHarmonicsBasedAsteroidImageMentalImage::SphericalHarmonicsBasedAsteroidImageMentalImage(std::shared_ptr<std::string> curAstName,
	                                                                                               std::string datasetPath) :
		currentAsteroidName(curAstName), asteroidDatasetPath(datasetPath), bitsForL(10), bitsForM(10), bitsForR(10) {
}

void SphericalHarmonicsBasedAsteroidImageMentalImage::updateWithInputs(std::vector<double> inputs) {
	unsigned l = decodeUInt(inputs.begin(), inputs.begin()+bitsForL);
	int m = decodeSInt(inputs.begin()+bitsForL, inputs.begin()+bitsForL+bitsForM);
	double r = decodeDouble(inputs.begin()+bitsForL+bitsForM, inputs.begin()+bitsForL+bitsForM+bitsForR);
	currentCommands.push_back(std::make_tuple(l,m,r));
}

void SphericalHarmonicsBasedAsteroidImageMentalImage::recordRunningScores(std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
}

void SphericalHarmonicsBasedAsteroidImageMentalImage::recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
}

void SphericalHarmonicsBasedAsteroidImageMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	double score = 1.0;
	if(visualize) std::cout << "Assigning score of " << score << std::endl;
//	org->dataMap.append("score", sampleScoresMap->getAverage("score"));
	org->dataMap.append("score", 0.);
}