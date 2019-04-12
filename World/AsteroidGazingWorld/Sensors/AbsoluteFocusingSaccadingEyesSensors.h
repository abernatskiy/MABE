#pragma once

#include "../../AbstractSensors.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../Utilities/AsteroidSnapshot.h"

#include <map>

typedef std::map<std::string,std::map<unsigned,std::map<unsigned,std::map<unsigned,AsteroidSnapshot>>>> asteroid_snapshots_library_type; // asteroidName,condition,distance,phase -> snapshot
inline std::string getAnAsteroid(const asteroid_snapshots_library_type& ast) { return ast.begin()->first; };
inline unsigned getACondition(const asteroid_snapshots_library_type& ast) { return (ast.begin()->second).begin()->first; };
inline unsigned getADistance(const asteroid_snapshots_library_type& ast) { return ((ast.begin()->second).begin()->second).begin()->first; };

class AbsoluteFocusingSaccadingEyesSensors : public AbstractSensors {

public:
	AbsoluteFocusingSaccadingEyesSensors(std::shared_ptr<std::string> curAstName,
	                                     std::shared_ptr<AsteroidsDatasetParser> datasetParser,
	                                     unsigned foveaResolution,
	                                     unsigned maxZoom,
	                                     unsigned splittingFactor,
	                                     int activeThresholdingDepth,
	                                     bool lockAtMaxZoom,
	                                     bool startZoomedIn);
	void update(int visualize) override;

	void reset(int visualize) override;
	int numOutputs() override { return numSensors; };
	int numInputs() override { return numMotors; };

	const std::vector<bool>& getLastPercept() { return savedPercept; };
	void* logTimeSeries(const std::string& label) override;

private:
	// Primary settings
	const unsigned foveaResolution;
	const unsigned maxZoom;
	const unsigned splittingFactor;
//	const unsigned numPhases = 16;
	const unsigned numPhases = 1;
	const bool useConstantThreshold;
	const unsigned activeThresholdingDepth;
	const bool lockAtMaxZoom;
	const bool startZoomedIn;

	const unsigned conditionControls = 0; // we're assuming that the spacecraft has pictures from one angle only for now (TODO: make tunable conditions)
	const unsigned distanceControls = 0; // neglecting distance control for now (TODO: make tunable distances)
	const unsigned phaseControls; // = bitsFor(numPhases)
	const unsigned zoomLevelControls; // = maxZoom : parallel zero-biased bus
	const unsigned zoomPositionControls; // = 2*maxZoom*bitsFor(splittingFactor) : for a splitting factor of N, position of the fovea is encoded in maxZoom Nary numbers

	const unsigned constantBinarizationThreshold = 160; // 127 is the middle of the dynamic range

	// Derived vars
	const unsigned numSensors;
	const unsigned numMotors;

	// Temporaries ultimately to be replaced
	unsigned constCondition, constDistance;

	std::shared_ptr<AsteroidsDatasetParser> datasetParser;
	std::string asteroidsDatasetPath;
	std::shared_ptr<std::string> currentAsteroidName;

	asteroid_snapshots_library_type asteroidSnapshots;

	std::uint8_t getThreshold(std::vector<bool>::iterator begin, std::vector<bool>::iterator end);
	unsigned getNumSensoryChannels();
	unsigned getNumControls();

	void analyzeDataset();

	// Even more temporary values
	std::vector<bool> savedPercept;
	std::vector<std::vector<unsigned>> perceptionControlsTimeSeries;
};
