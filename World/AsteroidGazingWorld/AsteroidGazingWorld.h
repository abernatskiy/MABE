#pragma once

#include "../AbstractSlideshowWorld.h"
#include "Utilities/AsteroidsDatasetParser.h"
#include "Sensors/PeripheralAndRelativeSaccadingEyesSensors.h"

class AsteroidGazingWorld : public AbstractSlideshowWorld {

	// The agent here must construct a mental image of the asteroid based on snapshots.
	// Snapshots are read by the agent via its saccading eye, and the shape is
	// reconstructed in form of a sequence of instructions for applying spherical
	// harmonics to a sphere

private:
	static std::shared_ptr<ParameterLink<int>> brainUpdatesPerAsteroidPL; int brainUpdatesPerAsteroid;
	static std::shared_ptr<ParameterLink<std::string>> datasetPathPL;     std::string datasetPath;
	static std::shared_ptr<ParameterLink<bool>> integrateFitnessPL;
	static std::shared_ptr<ParameterLink<int>> numTriggerBitsPL;
	static std::shared_ptr<ParameterLink<int>> numRandomInitialConditionsPL;
	static std::shared_ptr<ParameterLink<int>> compressToBitsPL;
	static std::shared_ptr<ParameterLink<int>> fastRepellingPLInfoNumNeighborsPL;
	static std::shared_ptr<ParameterLink<int>> mihPatternChunkSizeBitsPL;
	static std::shared_ptr<ParameterLink<double>> leakBaseMultiplierPL;
	static std::shared_ptr<ParameterLink<double>> leakDecayRadiusPL;
	static std::shared_ptr<ParameterLink<int>> minibatchSizePL;

	std::shared_ptr<AsteroidsDatasetParser> datasetParser;

	bool resetAgentBetweenStates() override { return true; };
	int brainUpdatesPerWorldState() override { return brainUpdatesPerAsteroid; };

	std::shared_ptr<std::string> currentAsteroidName;

	static int initialConditionsInitialized;
	static std::map<std::string,std::vector<Range2d>> commonRelativeSensorsInitialConditions;
	static int schedulesRandomSeed;

public:
	AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_);
};
