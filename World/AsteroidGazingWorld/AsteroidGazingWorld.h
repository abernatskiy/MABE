#pragma once

#include "../AbstractSlideshowWorld.h"
#include "Utilities/AsteroidsDatasetParser.h"

class AsteroidGazingWorld : public AbstractSlideshowWorld {

	// The agent here must construct a mental image of the asteroid based on snapshots.
	// Snapshots are read by the agent via its saccading eye, and the shape is
	// reconstructed in form of a sequence of instructions for applying spherical
	// harmonics to a sphere

private:
	static std::shared_ptr<ParameterLink<int>> brainUpdatesPerAsteroidPL; int brainUpdatesPerAsteroid;
	static std::shared_ptr<ParameterLink<std::string>> datasetPathPL;     std::string datasetPath;
	static std::shared_ptr<ParameterLink<int>> foveaResolutionPL;
	static std::shared_ptr<ParameterLink<int>> splittingFactorPL;
	static std::shared_ptr<ParameterLink<int>> maxZoomPL;
	static std::shared_ptr<ParameterLink<int>> activeThresholdingDepthPL;
	static std::shared_ptr<ParameterLink<bool>> lockAtMaxZoomPL;
	static std::shared_ptr<ParameterLink<bool>> startZoomedInPL;
	static std::shared_ptr<ParameterLink<bool>> integrateFitnessPL;
	static std::shared_ptr<ParameterLink<int>> numTriggerBitsPL;
	std::shared_ptr<AsteroidsDatasetParser> datasetParser;

	bool resetAgentBetweenStates() override { return true; };
	int brainUpdatesPerWorldState() override { return brainUpdatesPerAsteroid; };

	std::shared_ptr<std::string> currentAsteroidName;

public:
	AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_);
};
