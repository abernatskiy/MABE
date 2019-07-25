#include "AsteroidGazingWorld.h"

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
std::shared_ptr<ParameterLink<std::string>> AsteroidGazingWorld::sensorTypePL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-sensorType", (std::string) "absolute",
                                 "type of sensors to use (either absolute or relative)");
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

int AsteroidGazingWorld::initialConditionsInitialized = 0;
std::map<std::string,std::vector<Range2d>> AsteroidGazingWorld::commonRelativeSensorsInitialConditions;

AsteroidGazingWorld::AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {

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
		std::cerr << "AsteroidGazingWorld: Unsupported sensor type " << sensorType << std::endl;
		exit(EXIT_FAILURE);
	}

	sensors->writeSensorStats();
/*
	mentalImage = std::make_shared<DigitMentalImage>(currentAsteroidName,
	                                                 datasetParser,
	                                                 sensors,
	                                                 numTriggerBitsPL->get(PT_),
	                                                 integrateFitnessPL->get(PT_));
*/
	mentalImage = std::make_shared<CompressedMentalImage>(currentAsteroidName, datasetParser, sensors, 20);
	makeMotors();
};
