#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>

#include "../AbstractSlideshowWorld.h"
#include "../AsteroidGazingWorld/Utilities/AsteroidsDatasetParser.h"
#include "../AsteroidGazingWorld/Sensors/AbsoluteFocusingSaccadingEyesSensors.h"
#include "../AsteroidGazingWorld/Sensors/PeripheralAndRelativeSaccadingEyesSensors.h"

#include "dag.h"

typedef std::vector<std::vector<double>> StateTimeSeries;

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
	static std::shared_ptr<ParameterLink<int>> fastRepellingPLInfoNumNeighborsPL;
	static std::shared_ptr<ParameterLink<int>> mihPatternChunkSizeBitsPL;
	static std::shared_ptr<ParameterLink<double>> leakBaseMultiplierPL;
	static std::shared_ptr<ParameterLink<double>> leakDecayRadiusPL;
	static std::shared_ptr<ParameterLink<std::string>> pathToBrainTreeJSONPL;

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
	std::vector<StateTimeSeries> brainsInputCache; // preference for fast evolvable end point brains over the fast combined constant-evolvable end point brains
	std::vector<unsigned> exposedOutputs;
	unsigned numBrainsOutputs;

	bool resetAgentBetweenStates() override { return true; };
	int brainUpdatesPerWorldState() override { return brainUpdatesPerAsteroid; };

public:
	AsteroidTeamGazingWorld(std::shared_ptr<ParametersTable> PT_);
};
