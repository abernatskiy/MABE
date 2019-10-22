#include "AsteroidTeamGazingWorld.h"

#include "../AsteroidGazingWorld/MentalImages/CompressedMentalImage.h"
//#include "MentalImages/DigitMentalImage.h"
//#include "MentalImages/SpikesOnCubeMentalImage.h"
//#include "MentalImages/SpikesOnCubeFullMentalImage.h"
//#include "MentalImages/IdentityMentalImage.h"

#include "../AsteroidGazingWorld/Schedules/AsteroidGazingSchedules.h"

std::shared_ptr<ParameterLink<int>> AsteroidTeamGazingWorld::brainUpdatesPerAsteroidPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-brainUpdatesPerAsteroid", 100,
                                 "number of time steps for which the brain is allowed to gaze at each asteroid");
std::shared_ptr<ParameterLink<std::string>> AsteroidTeamGazingWorld::datasetPathPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-datasetPath", (std::string) "./asteroids",
                                 "path to the folder containing the asteroids shapes and snapshots dataset");
std::shared_ptr<ParameterLink<std::string>> AsteroidTeamGazingWorld::sensorTypePL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-sensorType", (std::string) "absolute",
                                 "type of sensors to use (either absolute or relative)");
std::shared_ptr<ParameterLink<bool>> AsteroidTeamGazingWorld::integrateFitnessPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-integrateFitness", true,
                                 "should the fitness be integrated over simulation time? (default: yes)");
std::shared_ptr<ParameterLink<int>> AsteroidTeamGazingWorld::numTriggerBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-numTriggerBits", 0,
                                 "how many trigger bits should be used? Multiple bits will be ANDed. If a negative number\n"
	                               "is given, trigger pressing will be required to get any fitness (default: 0)");
std::shared_ptr<ParameterLink<int>> AsteroidTeamGazingWorld::numRandomInitialConditionsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-numRandomInitialConditions", -1,
                                 "how many random initial conditions of sensors per asteroid view should be evaluated.\n"
	                               "Note: only relatve saccading sensors support this ATM. Non-positive values correspond to\n"
	                               "one evaluation per view with default initial conditions (retina in the upper left corner for\n"
	                               "relative saccading sensors). (default: -1)");
std::shared_ptr<ParameterLink<int>> AsteroidTeamGazingWorld::compressToBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-compressToBits", 20,
                                 "if CompressedMentalImage is used, how many bits should it compress the input to? (default: 20)");
std::shared_ptr<ParameterLink<int>> AsteroidTeamGazingWorld::fastRepellingPLInfoNumNeighborsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-fastRepellingPLInfoNumNeighbors", 10,
                                 "if CompressedMentalImage is used AND fastRepellingPatternLabelInformation is being computed,\n"
	                               "how many neighbors should the computation take into account? (default: 10)");
std::shared_ptr<ParameterLink<int>> AsteroidTeamGazingWorld::mihPatternChunkSizeBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-mihPatternChunkSizeBits", 16,
                                 "if CompressedMentalImage is used AND fastRepellingPatternLabelInformation is being computed,\n"
	                               "the nearest neighbors will be looked up with multi-index hashing algorithm. What is the chunk\n"
	                               "size that should be used for that algorithm? Max efficiency is reached at log2(N), where\n"
	                               "N is the number of asteroids/slides, assuming that output patterns are uniformly distributed.\n"
	                               "Must be 4, 8, 12, 16, 20, 24, 28 or 32. (default: 16)\n");
std::shared_ptr<ParameterLink<double>> AsteroidTeamGazingWorld::leakBaseMultiplierPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-leakBaseMultiplier", 1.,
                                 "if CompressedMentalImage is used AND any kind of repelling mutual information is being computed,\n"
	                               "the leaks between pattern-label pairs will be computed as b*exp(-d/a), where d is the Hammming\n"
	                               "distance from the neighbor of the point. What the value of b should be? (default: 1.)");
std::shared_ptr<ParameterLink<double>> AsteroidTeamGazingWorld::leakDecayRadiusPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-leakDecayRadius", 1.,
                                 "if CompressedMentalImage is used AND any kind of repelling mutual information is being computed,\n"
	                               "the leaks between pattern-label pairs will be computed as b*exp(-d/a), where d is the Hammming\n"
	                               "distance from the neighbor of the point. What the value of a should be? Cannot be zero. (default: 1.)");

int AsteroidTeamGazingWorld::initialConditionsInitialized = 0;
std::map<std::string,std::vector<Range2d>> AsteroidTeamGazingWorld::commonRelativeSensorsInitialConditions;

AsteroidTeamGazingWorld::AsteroidTeamGazingWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {

	// Localizing settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	datasetPath = datasetPathPL->get(PT_);
	std::string sensorType = sensorTypePL->get(PT_);
	bool randomizeInitialConditions = (numRandomInitialConditionsPL->get(PT_) > 0);
	unsigned numRandomInitialConditions = static_cast<unsigned>(numRandomInitialConditionsPL->get(PT_));

	// Validating settings
	if(randomizeInitialConditions && sensorType!="relative") {
		std::cerr << "Sensors of type " << sensorType << " do not support initial conditions randomization" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Drawing the rest of the owl
	datasetParser = std::make_shared<AsteroidsDatasetParser>(datasetPath);
	currentAsteroidName = std::make_shared<std::string>("");

	if(sensorType=="absolute") {
		sensors = std::make_shared<AbsoluteFocusingSaccadingEyesSensors>(currentAsteroidName, datasetParser, PT_);
		stateSchedule = std::make_shared<ExhaustiveAsteroidGazingSchedule>(currentAsteroidName, datasetParser);
	}
	else if(sensorType=="relative") {
		auto rawSensorsPointer = std::make_shared<PeripheralAndRelativeSaccadingEyesSensors>(currentAsteroidName, datasetParser, PT_);
		if(!initialConditionsInitialized) {
			if(randomizeInitialConditions) {
				for(const auto& astName : datasetParser->getAsteroidsNames()) {
					commonRelativeSensorsInitialConditions[astName] = {};
					for(unsigned i=0; i<numRandomInitialConditions; i++)
						commonRelativeSensorsInitialConditions[astName].push_back(rawSensorsPointer->generateRandomInitialState());
				}
			}
			else {
				for(const auto& astName : datasetParser->getAsteroidsNames())
					commonRelativeSensorsInitialConditions[astName] = { rawSensorsPointer->generateDefaultInitialState() };
			}
			initialConditionsInitialized = 1;
		}
		stateSchedule = std::make_shared<ExhaustiveAsteroidGazingScheduleWithRelativeSensorInitialStates>(
		                  currentAsteroidName,
		                  datasetParser,
		                  rawSensorsPointer->getPointerToInitialState(),
		                  commonRelativeSensorsInitialConditions);
		sensors = rawSensorsPointer; // downcast
	}
	else {
		std::cerr << "AsteroidTeamGazingWorld: Unsupported sensor type " << sensorType << std::endl;
		exit(EXIT_FAILURE);
	}

	sensors->writeSensorStats();
	mentalImage = std::make_shared<CompressedMentalImage>(currentAsteroidName,
	                                                      datasetParser,
	                                                      sensors,
	                                                      compressToBitsPL->get(PT_),
	                                                      mihPatternChunkSizeBitsPL->get(PT_),
	                                                      fastRepellingPLInfoNumNeighborsPL->get(PT_),
	                                                      leakBaseMultiplierPL->get(PT_),
	                                                      leakDecayRadiusPL->get(PT_));
	makeMotors();
};
