#pragma once

#include <map>

#include "../../AbstractSensors.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../Utilities/AsteroidSnapshot.h"
#include "../../../Utilities/serializeableArray.h"
#include "../../../Utilities/texture.h"
#include "rangeDecoders.h"

typedef std::tuple<std::string,unsigned> CompleteAsteroidViewParameters; // asteroid name, then phase
typedef std::map<CompleteAsteroidViewParameters,AsteroidSnapshot> CompleteAsteroidSnapshotsLibrary;

class CompleteViewSensors : public AbstractSensors {

public:
	CompleteViewSensors(std::shared_ptr<std::string> curAstName,
	                    std::shared_ptr<AsteroidsDatasetParser> datasetParser,
	                    std::shared_ptr<ParametersTable> PT);
	~CompleteViewSensors();
	void update(int visualize) override;
	void reset(int visualize) override; // not currently doing anything
	void doHeavyInit() override;
	void* getDataForBrain() override { return perceptPtr; };

	int numOutputs() override { return 0; };
	int numInputs() override { return 0; };

private:
	std::shared_ptr<std::string> currentAsteroidName;
	std::shared_ptr<AsteroidsDatasetParser> datasetParser;

	Texture* perceptPtr;
	std::string storedPerceptIdentifier; // name of the asteroid giving rise to actual info at perceptPtr,
	                                     // as opposed to the name of the asteroid that the World wants perceived (*currentAsteroidName)

	const unsigned frameRes;
	const unsigned numPhases;
	std::vector<unsigned> phases;
	const std::uint8_t baseThreshold;

	// Actual state
	CompleteAsteroidSnapshotsLibrary asteroidSnapshots;

	// Parameter links
	static std::shared_ptr<ParameterLink<int>> frameResolutionPL;
	static std::shared_ptr<ParameterLink<int>> numPhasesPL;
	static std::shared_ptr<ParameterLink<int>> binarizationThresholdPL;

	// Dataset caching
	static SerializeableArray<char> snapshotsCache;
	bool readPersistentSnapshotsCache(); // returns true if cache has been loaded successfully
	void writePersistentSnapshotsCache();
	void readSnapshotsIntoCache();
	void loadSnapshotsFromCache();
};
