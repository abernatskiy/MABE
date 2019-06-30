#include "AsteroidGazingWorld.h"

#include "MentalImages/DigitMentalImage.h"
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
                                 "how many trigger bits should be used? Multiple bits will be ANDed. If a negative number is given, trigger pressing will be required to get any fitness (default: 0)");

int AsteroidGazingWorld::initialConditionsInitialized = 0;
std::map<std::string,std::vector<Range2d>> AsteroidGazingWorld::commonRelativeSensorsInitialConditions;

AsteroidGazingWorld::AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {
	// Localizing settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	datasetPath = datasetPathPL->get(PT_);
	datasetParser = std::make_shared<AsteroidsDatasetParser>(datasetPath);

	// Drawing the rest of the owl
	currentAsteroidName = std::make_shared<std::string>("");

	std::string sensorType = sensorTypePL->get(PT_);
	if(sensorType=="absolute") {
		sensors = std::make_shared<AbsoluteFocusingSaccadingEyesSensors>(currentAsteroidName, datasetParser, PT_);
		stateSchedule = std::make_shared<ExhaustiveAsteroidGazingSchedule>(currentAsteroidName, datasetParser);
	}
	else if(sensorType=="relative") {
		auto rawSensorsPointer = std::make_shared<PeripheralAndRelativeSaccadingEyesSensors>(currentAsteroidName, datasetParser, PT_);
		if(!initialConditionsInitialized) {
			for(const auto& astName : datasetParser->getAsteroidsNames())
				commonRelativeSensorsInitialConditions[astName] = { rawSensorsPointer->generateDefaultInitialState() };
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

	mentalImage = std::make_shared<DigitMentalImage>(currentAsteroidName,
	                                                 datasetParser,
	                                                 sensors,
	                                                 numTriggerBitsPL->get(PT_),
	                                                 integrateFitnessPL->get(PT_));
	makeMotors();
};
