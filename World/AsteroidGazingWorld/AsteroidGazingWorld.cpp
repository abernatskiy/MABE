#include "AsteroidGazingWorld.h"

#include "SphericalHarmonicsBasedAsteroidImageMentalImage.h"
#include "AbsoluteFocusingSaccadingEyesSensors.h"
#include "AsteroidGazingSchedules.h"

std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::brainUpdatesPerAsteroidPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-brainUpdatesPerAsteroid", 100,
                                 "number of time steps for which the brain is allowed to gaze at each asteroid");
std::shared_ptr<ParameterLink<std::string>> AsteroidGazingWorld::asteroidsDatasetPathPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-asteroidsDatasetPath", (std::string) "./asteroids",
                                 "path to the folder containing the asteroids shapes and snapshots dataset");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::sensorResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-sensorResolution", 0,
                                 "sensor resoluton level r, determining the number of sensor outputs as 4^r");

AsteroidGazingWorld::AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {
	// Localizing settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	asteroidsDatasetPath = asteroidsDatasetPathPL->get(PT_);

	// Drawing the rest of the owl
	currentAsteroidName = std::make_shared<std::string>("");
	stateSchedule = std::make_shared<ExhaustiveAsteroidGazingSchedule>(currentAsteroidName, asteroidsDatasetPath);
	sensors = std::make_shared<AbsoluteFocusingSaccadingEyesSensors>(currentAsteroidName, asteroidsDatasetPath, brain, sensorResolutionPL->get(PT_));
	mentalImage = std::make_shared<SphericalHarmonicsBasedAsteroidImageMentalImage>(currentAsteroidName, asteroidsDatasetPath);

	makeMotors();
};
