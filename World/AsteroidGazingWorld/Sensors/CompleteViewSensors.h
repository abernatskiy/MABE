#pragma once

#include <map>
#include "boost/multi_array.hpp"

#include "../../AbstractSensors.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../Utilities/AsteroidSnapshot.h"
#include "../../../Utilities/serializeableArray.h"
#include "rangeDecoders.h"

typedef std::tuple<std::string> AsteroidViewParameters;
typedef std::map<AsteroidViewParameters,AsteroidSnapshot> AsteroidSnapshotsLibrary;
typedef boost::multi_array<uint8_t,4> Percept; // x, y, t, channel

class CompleteViewSensors : public AbstractSensors {

public:
	CompleteViewSensors(std::shared_ptr<std::string> curAstName,
	                    std::shared_ptr<AsteroidsDatasetParser> datasetParser,
	                    std::shared_ptr<ParametersTable> PT);
	~CompleteViewSensors();
	void update(int visualize) override;
	void reset(int visualize) override;
	void doHeavyInit() override;

	int numOutputs() override { return numSensors; };
	int numInputs() override { return numMotors; };

	const std::vector<bool>& getLastPercept() override { return savedPercept; };
	void* logTimeSeries(const std::string& label) override;

	unsigned numSaccades() override { return 0; };
	unsigned numActiveStatesInRecording() override;
	unsigned numStatesInRecording() override { return numSensors*perceptTimeSeries.size(); };
	double sensoryMotorEntropy(int shift) override { return 0.; };

private:
	std::shared_ptr<std::string> currentAsteroidName;
	std::shared_ptr<AsteroidsDatasetParser> datasetParser;

	Percept* perceptPtr;
	std::string storedPerceptIdentifier; // if we're cycling through asteroids and each one gives a percept, then it is the asteroid name

	const unsigned frameRes;
	const unsigned numPhases = 1;
	const std::uint8_t baseThreshold = 128;

	// Derived vars and facilities
	std::shared_ptr<AbstractRangeDecoder> rangeDecoder;
	const unsigned numSensors;
	const unsigned numMotors;

	// Actual state
	AsteroidSnapshotsLibrary asteroidSnapshots;

	// Parameter links
	static std::shared_ptr<ParameterLink<int>> frameResolutionPL;

	// Dataset caching
	static SerializeableArray<char> snapshotsCache;
	bool readPersistentSnapshotsCache(); // returns true if cache has been loaded successfully
	void writePersistentSnapshotsCache();
	void readSnapshotsIntoCache();
	void loadSnapshotsFromCache();
};
