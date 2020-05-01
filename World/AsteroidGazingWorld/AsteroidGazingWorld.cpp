#include "AsteroidGazingWorld.h"

#include <limits>
#include "../../Utilities/Random.h"

//#include "MentalImages/DistancesMentalImage.h"
#include "MentalImages/CompressedMentalImage.h"
//#include "MentalImages/DigitMentalImage.h"
//#include "MentalImages/SpikesOnCubeMentalImage.h"
//#include "MentalImages/SpikesOnCubeFullMentalImage.h"
//#include "MentalImages/IdentityMentalImage.h"

#include "Schedules/AsteroidGazingSchedules.h"

std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::brainUpdatesPerAsteroidPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-brainUpdatesPerAsteroid", 100,
                                 "number of time steps for which the brain is allowed to gaze at each asteroid");
std::shared_ptr<ParameterLink<std::string>> AsteroidGazingWorld::datasetPathPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-datasetPath", (std::string) "./asteroids",
                                 "path to the folder containing the asteroids shapes and snapshots dataset");
std::shared_ptr<ParameterLink<bool>> AsteroidGazingWorld::integrateFitnessPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-integrateFitness", true,
                                 "should the fitness be integrated over simulation time? (default: yes)");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::numTriggerBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-numTriggerBits", 0,
                                 "how many trigger bits should be used? Multiple bits will be ANDed. If a negative number\n"
	                               "is given, trigger pressing will be required to get any fitness (default: 0)");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::numRandomInitialConditionsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-numRandomInitialConditions", -1,
                                 "how many random initial conditions of sensors per asteroid view should be evaluated.\n"
	                               "Note: only relatve saccading sensors support this ATM. Non-positive values correspond to\n"
	                               "one evaluation per view with default initial conditions (retina in the upper left corner for\n"
	                               "relative saccading sensors). (default: -1)");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::compressToBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-compressToBits", 20,
                                 "if CompressedMentalImage is used, how many bits should it compress the input to? (default: 20)");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::fastRepellingPLInfoNumNeighborsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-fastRepellingPLInfoNumNeighbors", 10,
                                 "if CompressedMentalImage is used AND fastRepellingPatternLabelInformation is being computed,\n"
	                               "how many neighbors should the computation take into account? (default: 10)");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::mihPatternChunkSizeBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-mihPatternChunkSizeBits", 16,
                                 "if CompressedMentalImage is used AND fastRepellingPatternLabelInformation is being computed,\n"
	                               "the nearest neighbors will be looked up with multi-index hashing algorithm. What is the chunk\n"
	                               "size that should be used for that algorithm? Max efficiency is reached at log2(N), where\n"
	                               "N is the number of asteroids/slides, assuming that output patterns are uniformly distributed.\n"
	                               "Must be 4, 8, 12, 16, 20, 24, 28 or 32. (default: 16)\n");
std::shared_ptr<ParameterLink<double>> AsteroidGazingWorld::leakBaseMultiplierPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-leakBaseMultiplier", 1.,
                                 "if CompressedMentalImage is used AND any kind of repelling mutual information is being computed,\n"
	                               "the leaks between pattern-label pairs will be computed as b*exp(-d/a), where d is the Hammming\n"
	                               "distance from the neighbor of the point. What the value of b should be? (default: 1.)");
std::shared_ptr<ParameterLink<double>> AsteroidGazingWorld::leakDecayRadiusPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-leakDecayRadius", 1.,
                                 "if CompressedMentalImage is used AND any kind of repelling mutual information is being computed,\n"
	                               "the leaks between pattern-label pairs will be computed as b*exp(-d/a), where d is the Hammming\n"
	                               "distance from the neighbor of the point. What the value of a should be? Cannot be zero. (default: 1.)");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::minibatchSizePL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-minibatchSize", 0,
                                 "if nonzero, this many datapoints will be selected from the dataset at every generation for\n"
	                               "evaluation purposes (default: 0)");
std::shared_ptr<ParameterLink<bool>> AsteroidGazingWorld::balanceMinibatchesPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-balanceMinibatches", true,
                                 "should the minibatches be balanced wrt labels? (default: yes)");
std::shared_ptr<ParameterLink<bool>> AsteroidGazingWorld::overwriteEvaluationsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-overwriteEvaluations", true,
                                 "should the world overwrite evaluations (as opposed to appending new ones) in the organism's datamap? (default: yes)");

int AsteroidGazingWorld::initialConditionsInitialized = 0;
std::map<std::string,std::vector<Range2d>> AsteroidGazingWorld::commonRelativeSensorsInitialConditions;
int AsteroidGazingWorld::schedulesRandomSeed = -1;

AsteroidGazingWorld::AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {

	// Localizing settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	datasetPath = datasetPathPL->get(PT_);
	bool randomizeInitialConditions = (numRandomInitialConditionsPL->get(PT_) > 0);
	unsigned numRandomInitialConditions = static_cast<unsigned>(numRandomInitialConditionsPL->get(PT_));

	// Drawing the rest of the owl
	datasetParser = std::make_shared<AsteroidsDatasetParser>(datasetPath);
	currentAsteroidName = std::make_shared<std::string>("");

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
	if(schedulesRandomSeed == -1) {
		schedulesRandomSeed = Random::getInt(std::numeric_limits<int>::max());
		std::cout << "Made a common random seed for schedules: " << schedulesRandomSeed << std::endl << std::flush;
	}

	int minibatchSize = minibatchSizePL->get(PT_);
	if(minibatchSize == 0) {
		stateSchedule = std::make_shared<ExhaustiveAsteroidGazingScheduleWithRelativeSensorInitialStates>(
		                  currentAsteroidName,
		                  datasetParser,
		                  rawSensorsPointer->getPointerToInitialState(),
		                  commonRelativeSensorsInitialConditions);
	}
	else {
		stateSchedule = std::make_shared<MinibatchAsteroidGazingScheduleWithRelativeSensorInitialStates>(
		                  currentAsteroidName,
		                  datasetParser,
		                  rawSensorsPointer->getPointerToInitialState(),
		                  commonRelativeSensorsInitialConditions,
		                  minibatchSize,
		                  balanceMinibatchesPL->get(PT_),
		                  schedulesRandomSeed);
	}

	sensors = rawSensorsPointer; // downcast

	sensors->writeSensorStats();

	sensors->doHeavyInit();
/*
	mentalImage = std::make_shared<DigitMentalImage>(currentAsteroidName,
	                                                 datasetParser,
	                                                 sensors,
	                                                 numTriggerBitsPL->get(PT_),
	                                                 integrateFitnessPL->get(PT_));
*/
	mentalImage = std::make_shared<CompressedMentalImage>(currentAsteroidName,
	                                                      datasetParser,
	                                                      sensors,
	                                                      compressToBitsPL->get(PT_),
	                                                      false,
	                                                      mihPatternChunkSizeBitsPL->get(PT_),
	                                                      fastRepellingPLInfoNumNeighborsPL->get(PT_),
	                                                      leakBaseMultiplierPL->get(PT_),
	                                                      leakDecayRadiusPL->get(PT_));
/*
	mentalImage = std::make_shared<DistancesMentalImage>(currentAsteroidName,
	                                                     datasetParser,
	                                                     sensors, // gets the number of output nodes data from Brain/LayeredBrain/topology.h
	                                                     overwriteEvaluationsPL->get(PT_));
*/
	makeMotors();
};
