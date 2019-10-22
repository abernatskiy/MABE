#pragma once

#include "../AbstractSlideshowWorld.h"
#include "../AsteroidGazingWorld/Utilities/AsteroidsDatasetParser.h"
#include "../AsteroidGazingWorld/Sensors/AbsoluteFocusingSaccadingEyesSensors.h"
#include "../AsteroidGazingWorld/Sensors/PeripheralAndRelativeSaccadingEyesSensors.h"

class AsteroidTeamGazingWorld : public AbstractSlideshowWorld {

	// The agent here must construct a mental image of the asteroid based on snapshots.
	// Snapshots are read by the agent via its saccading eye, and the shape is
	// reconstructed in some form. The signal from the eye passes through a DAG of brain modules
	// that can be evolved independently.
	// NOTE: coevolution of more than one brain per DAG is not currently supported

private:
	static std::shared_ptr<ParameterLink<int>> brainUpdatesPerAsteroidPL; int brainUpdatesPerAsteroid;
	static std::shared_ptr<ParameterLink<std::string>> datasetPathPL;     std::string datasetPath;
	static std::shared_ptr<ParameterLink<std::string>> sensorTypePL;
	static std::shared_ptr<ParameterLink<bool>> integrateFitnessPL;
	static std::shared_ptr<ParameterLink<int>> numTriggerBitsPL;
	static std::shared_ptr<ParameterLink<int>> numRandomInitialConditionsPL;
	static std::shared_ptr<ParameterLink<int>> compressToBitsPL;
	static std::shared_ptr<ParameterLink<int>> fastRepellingPLInfoNumNeighborsPL;
	static std::shared_ptr<ParameterLink<int>> mihPatternChunkSizeBitsPL;
	static std::shared_ptr<ParameterLink<double>> leakBaseMultiplierPL;
	static std::shared_ptr<ParameterLink<double>> leakDecayRadiusPL;

	std::shared_ptr<AsteroidsDatasetParser> datasetParser;

	bool resetAgentBetweenStates() override { return true; };
	int brainUpdatesPerWorldState() override { return brainUpdatesPerAsteroid; };

	std::shared_ptr<std::string> currentAsteroidName;

	static int initialConditionsInitialized;
	static std::map<std::string,std::vector<Range2d>> commonRelativeSensorsInitialConditions;

// 	std::vector<std::shared_ptr<AbstractBrain>> brains;

public:
	AsteroidTeamGazingWorld(std::shared_ptr<ParametersTable> PT_);
};
