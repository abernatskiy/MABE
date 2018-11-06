#include "AsteroidGazingWorld.h"

#include "MentalImages/SphericalHarmonicsBasedAsteroidImageMentalImage.h"
#include "Sensors/AbsoluteFocusingSaccadingEyesSensors.h"
#include "Schedules/AsteroidGazingSchedules.h"

std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::brainUpdatesPerAsteroidPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-brainUpdatesPerAsteroid", 100,
                                 "number of time steps for which the brain is allowed to gaze at each asteroid");
std::shared_ptr<ParameterLink<std::string>> AsteroidGazingWorld::datasetPathPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-datasetPath", (std::string) "./asteroids",
                                 "path to the folder containing the asteroids shapes and snapshots dataset");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::sensorResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-sensorResolution", 0,
                                 "sensor resoluton level r, determining the number of sensor outputs as 4^r");

AsteroidGazingWorld::AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {
	// Localizing settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	datasetPath = datasetPathPL->get(PT_);
	datasetParser = std::make_shared<AsteroidsDatasetParser>(datasetPath);

	// Drawing the rest of the owl
	currentAsteroidName = std::make_shared<std::string>("");
	stateSchedule = std::make_shared<ExhaustiveAsteroidGazingSchedule>(currentAsteroidName, datasetParser);
	sensors = std::make_shared<AbsoluteFocusingSaccadingEyesSensors>(currentAsteroidName, datasetParser, sensorResolutionPL->get(PT_));
	mentalImage = std::make_shared<SphericalHarmonicsBasedAsteroidImageMentalImage>(currentAsteroidName, datasetPath);

	makeMotors();
};
