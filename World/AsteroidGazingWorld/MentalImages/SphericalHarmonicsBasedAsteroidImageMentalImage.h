#pragma once

#include <algorithm>

#include "../../AbstractMentalImage.h"

// Position-based decoders

unsigned decodeUInt(std::vector<double>::iterator begin, std::vector<double>::iterator end);
int decodeSInt(std::vector<double>::iterator begin, std::vector<double>::iterator end);
double decodeDouble(std::vector<double>::iterator begin, std::vector<double>::iterator end);

// The class itself

class SphericalHarmonicsBasedAsteroidImageMentalImage : public AbstractMentalImage {

private:
	std::shared_ptr<std::string> currentAsteroidName;
	std::string asteroidDatasetPath;
	std::vector<std::tuple<unsigned,int,double>> currentCommands;

	const unsigned bitsForL;
	const unsigned bitsForM;
	const unsigned bitsForR;

public:
	SphericalHarmonicsBasedAsteroidImageMentalImage(std::shared_ptr<std::string> curAstName, std::string datasetPath);

	void reset(int visualize) override { currentCommands.clear(); };

	void updateWithInputs(std::vector<double> inputs) override;

	void recordRunningScores(std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) override;
	void evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) override;

	int numInputs() override { return bitsForL+bitsForM+bitsForR; };
};
