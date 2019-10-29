#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
#include <cstdlib>

#include "../AbstractSlideshowWorld.h"
#include "../AsteroidGazingWorld/Utilities/AsteroidsDatasetParser.h"
#include "../AsteroidGazingWorld/Sensors/AbsoluteFocusingSaccadingEyesSensors.h"
#include "../AsteroidGazingWorld/Sensors/PeripheralAndRelativeSaccadingEyesSensors.h"

#include "dag.h"

typedef std::vector<std::vector<double>> StateTimeSeries;

class AsteroidTeamGazingWorld : public AbstractSlideshowWorld {

	// DANGER: DO NOT INHERIT FROM THIS CLASS UNDER ANY CIRCUMSTANCES
	// This class breaks a few assumptions that are made by its parent classes.
	// Inherit from it, and you'll quickly lose track of what your overrides are doing.

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
	static std::shared_ptr<ParameterLink<int>> fastRepellingPLInfoNumNeighborsPL;
	static std::shared_ptr<ParameterLink<int>> mihPatternChunkSizeBitsPL;
	static std::shared_ptr<ParameterLink<double>> leakBaseMultiplierPL;
	static std::shared_ptr<ParameterLink<double>> leakDecayRadiusPL;
	static std::shared_ptr<ParameterLink<std::string>> pathToBrainGraphJSONPL;

	std::shared_ptr<AsteroidsDatasetParser> datasetParser;
	std::shared_ptr<std::string> currentAsteroidName;
	static int initialConditionsInitialized;
	static std::map<std::string,std::vector<Range2d>> commonRelativeSensorsInitialConditions;

	unsigned numBrains;
	DAGraph<int> brainsDiagram;
	std::vector<std::shared_ptr<AbstractBrain>> brains; // maybe replace with map-to-structures at some point
	std::vector<std::string> brainsIDs;
	std::vector<unsigned> brainsEvolvable;
	std::vector<unsigned> brainsNumOutputs;
	std::vector<unsigned> brainsNumHidden;
	std::vector<unsigned> brainsInputCached; // format: 0 = do not cache, 1 = input to be cached, 2 = input cached
	std::vector<std::unordered_map<std::string,StateTimeSeries>> brainsInputCache; // caching inputs prioritizes fast evolvable end point brains over the combintations of variable end point brains with constant components' outputs
	bool cachingComplete;
	std::vector<unsigned> exposedOutputs;
	std::vector<unsigned> brainsConnectedToOculomotors;
	unsigned numBrainsOutputs;

	bool resetAgentBetweenStates() override { std::cerr << "resetAgentBetweenStates() should not be called for AsteroidTeamGazingWorld" << std::endl; exit(EXIT_FAILURE); return true; };
	int brainUpdatesPerWorldState() override { return brainUpdatesPerAsteroid; };

	StateTimeSeries executeBrainComponent(unsigned idx, int visualize);

public:
	AsteroidTeamGazingWorld(std::shared_ptr<ParametersTable> PT_);
	void resetWorld(int visualize) override;
	void evaluateOnce(std::shared_ptr<Organism> org, unsigned repIdx, int visualize) override;
	std::unordered_map<std::string, std::unordered_set<std::string>> requiredGroups() override;
};

template<typename T>
inline void printVector(std::vector<T> theVec, std::string vecName) {
	std::cout << vecName << ":";
	for(const auto& v : theVec)
		std::cout << " " << v;
	std::cout << std::endl;
}
