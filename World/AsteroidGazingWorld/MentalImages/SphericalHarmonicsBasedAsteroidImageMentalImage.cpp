#include "SphericalHarmonicsBasedAsteroidImageMentalImage.h"
#include "decoders.h"

SphericalHarmonicsBasedAsteroidImageMentalImage::SphericalHarmonicsBasedAsteroidImageMentalImage(std::shared_ptr<std::string> curAstName,
	                                                                                               std::string datasetPath) :
		currentAsteroidName(curAstName), asteroidDatasetPath(datasetPath), bitsForL(10), bitsForM(10), bitsForR(10) {
}

void SphericalHarmonicsBasedAsteroidImageMentalImage::updateWithInputs(std::vector<double> inputs) {
	unsigned l = decodeOHUInt(inputs.begin(), inputs.begin()+bitsForL);
	int m = decodeOHSInt(inputs.begin()+bitsForL, inputs.begin()+bitsForL+bitsForM);
	double r = decodeOHDouble(inputs.begin()+bitsForL+bitsForM, inputs.begin()+bitsForL+bitsForM+bitsForR);
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
