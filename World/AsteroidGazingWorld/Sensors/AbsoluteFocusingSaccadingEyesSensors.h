#pragma once

#include "../../AbstractSensors.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../Utilities/AsteroidSnapshot.h"

#include <map>

typedef std::map<std::string,std::map<unsigned,std::map<unsigned,std::map<unsigned,AsteroidSnapshot>>>> asteroid_snapshots_library_type;

class AbsoluteFocusingSaccadingEyesSensors : public AbstractSensors {

public:
	AbsoluteFocusingSaccadingEyesSensors(std::shared_ptr<std::string> curAstName, std::shared_ptr<AsteroidsDatasetParser> datasetParser, unsigned res);
	void update(int visualize) override;

	void reset(int visualize) override { AbstractSensors::reset(visualize); }; // sensors themselves are stateless
	int numOutputs() override { return numSensors; };
	int numInputs() override { return 1; };

private:
	const unsigned resolution;
	const unsigned numSensors;
	std::string asteroidsDatasetPath;
	std::shared_ptr<std::string> currentAsteroidName;

	asteroid_snapshots_library_type asteroidSnapshots;

	unsigned getNumInputChannels();

};
