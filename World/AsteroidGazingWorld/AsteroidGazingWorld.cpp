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
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::foveaResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-foveaResolution", 3,
                                 "number of rows and columns in the sensors fovea (resulting number of sensory inputs is r^2)");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::splittingFactorPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-splittingFactor", 3,
                                 "the factor z that determines how zoom works, in particular the snapshot is divided into z^2 sub-areas at each zoom level; acceptable values are 2 and 3");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::maxZoomPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-maxZoom", 3,
                                 "the number of allowed zoom levels");

AsteroidGazingWorld::AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {
	// Localizing settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	datasetPath = datasetPathPL->get(PT_);
	datasetParser = std::make_shared<AsteroidsDatasetParser>(datasetPath);

	// Drawing the rest of the owl
	currentAsteroidName = std::make_shared<std::string>("");
	stateSchedule = std::make_shared<ExhaustiveAsteroidGazingSchedule>(currentAsteroidName, datasetParser);
	sensors = std::make_shared<AbsoluteFocusingSaccadingEyesSensors>(currentAsteroidName, datasetParser, foveaResolutionPL->get(PT_), maxZoomPL->get(PT_), splittingFactorPL->get(PT_));
	mentalImage = std::make_shared<SphericalHarmonicsBasedAsteroidImageMentalImage>(currentAsteroidName, datasetParser);

	makeMotors();
};
