#pragma once

#include "../../AbstractSensors.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../Utilities/AsteroidSnapshot.h"
#include "rangeDecoders.h"

#include <map>

typedef std::tuple<std::string> AsteroidViewParameters;
typedef std::map<AsteroidViewParameters,AsteroidSnapshot> AsteroidSnapshotsLibrary;

class PeripheralAndRelativeSaccadingEyesSensors : public AbstractSensors {

public:
	PeripheralAndRelativeSaccadingEyesSensors(std::shared_ptr<std::string> curAstName,
	                                          std::shared_ptr<AsteroidsDatasetParser> datasetParser,
	                                          std::shared_ptr<ParametersTable> PT);
	void update(int visualize) override;
	void reset(int visualize) override;
	void doHeavyInit() override;

	int numOutputs() override { return numSensors; };
	int numInputs() override { return numMotors; };

	const std::vector<bool>& getLastPercept() override { return savedPercept; };
	void* logTimeSeries(const std::string& label) override;

	unsigned numSaccades() override;
	unsigned numActiveStatesInRecording() override;
	unsigned numStatesInRecording() override { return numSensors*perceptTimeSeries.size(); };
	double sensoryMotorEntropy(int shift) override;

	Range2d generateDefaultInitialState() { return Range2d(Range1d(0, foveaRes), Range1d(0, foveaRes)); };
	Range2d generateRandomInitialState();
	std::shared_ptr<Range2d> getPointerToInitialState() { return initialFoveaPositionPtr; };

private:
	std::shared_ptr<std::string> currentAsteroidName;
	std::shared_ptr<AsteroidsDatasetParser> datasetParser;

	const unsigned frameRes;
	const bool usePeripheralFOV;
	const unsigned peripheralFOVRes;
	const unsigned foveaRes;
	const unsigned jumpType;
	const unsigned jumpGradations;
	const bool forbidRest;

	const unsigned conditionControls = 0; // we're assuming that the spacecraft has pictures from one angle only for now (TODO: make tunable conditions)
	const unsigned distanceControls = 0; // neglecting distance control for now (TODO: make tunable distances)
	const unsigned numPhases = 1;
	const unsigned phaseControls = 0; // no controls for one phase
	const std::uint8_t baseThreshold = 128;
	const int numThresholdsToTry;

	// Derived vars and facilities
	std::shared_ptr<AbstractRangeDecoder> rangeDecoder;
	const unsigned foveaPositionControls;
	const unsigned numSensors;
	const unsigned numMotors;

	// Actual state
	AsteroidSnapshotsLibrary asteroidSnapshots;
	std::map<AsteroidViewParameters,std::uint8_t> peripheralFOVThresholds;
	std::shared_ptr<Range2d> initialFoveaPositionPtr;
	Range2d foveaPosition;
	Range2d foveaPositionOnGrid;
	std::vector<bool> controls;
	std::vector<bool> savedPercept;
	std::vector<Range2d> foveaPositionTimeSeries;
	std::vector<std::vector<bool>> perceptTimeSeries;
	std::vector<std::vector<bool>> controlsTimeSeries;
	std::vector<std::string> sensorStateDescriptionTimeSeries;

	// Private methods
	void resetFoveaPosition();
	void analyzeDataset();
	std::string getSensorStateDescription();

	// Parameter links
	static std::shared_ptr<ParameterLink<int>> frameResolutionPL;
	static std::shared_ptr<ParameterLink<bool>> usePeripheralFOVPL;
	static std::shared_ptr<ParameterLink<int>> peripheralFOVResolutionPL;
	static std::shared_ptr<ParameterLink<int>> peripheralFOVNumThresholdsToTryPL;
	static std::shared_ptr<ParameterLink<int>> foveaResolutionPL;
	static std::shared_ptr<ParameterLink<int>> jumpTypePL;
	static std::shared_ptr<ParameterLink<int>> jumpGradationsPL;
	static std::shared_ptr<ParameterLink<bool>> forbidRestPL;
};
