#pragma once

#include "../AbstractSlideshowWorld.h"

class AsteroidGazingWorld : public AbstractSlideshowWorld {

	// The agent here must construct a mental image of the asteroid based on snapshots.
	// Snapshots are read by the agent via its saccading eye, and the shape is
	// reconstructed in form of a sequence of instructions for applying spherical
	// harmonics to a sphere

private:
	static std::shared_ptr<ParameterLink<int>> brainUpdatesPerAsteroidPL;       int brainUpdatesPerAsteroid;
	static std::shared_ptr<ParameterLink<std::string>> asteroidsDatasetPathPL;  std::string asteroidsDatasetPath;
	static std::shared_ptr<ParameterLink<int>> sensorResolutionPL;

	bool resetAgentBetweenStates() override { return true; };
	int brainUpdatesPerWorldState() override { return brainUpdatesPerAsteroid; };

	std::shared_ptr<std::string> currentAsteroidName;

public:
	AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_);
};