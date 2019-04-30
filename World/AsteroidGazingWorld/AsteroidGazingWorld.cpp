#include "AsteroidGazingWorld.h"

#include "MentalImages/DigitMentalImage.h"
//#include "MentalImages/SpikesOnCubeMentalImage.h"
//#include "MentalImages/SpikesOnCubeFullMentalImage.h"
//#include "MentalImages/IdentityMentalImage.h"

#include "Sensors/PeripheralAndRelativeSaccadingEyesSensors.h"
//#include "Sensors/AbsoluteFocusingSaccadingEyesSensors.h"
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
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::activeThresholdingDepthPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-activeThresholdingDepth", -1,
                                 "number of bisections of the 0..255 interval that the sensors can make to get thresholds (e.g. 0..127 yields a threshold of 63), negatives meaning fixed threshold of 160");
std::shared_ptr<ParameterLink<bool>> AsteroidGazingWorld::lockAtMaxZoomPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-lockAtMaxZoom", false,
                                 "should sensors be locked at max zoom level? If true, zooming out is impossible/disabled");
std::shared_ptr<ParameterLink<bool>> AsteroidGazingWorld::startZoomedInPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-startZoomedIn", false,
                                 "should the default state of the nodes (0000...) correspond to max zoom level instead of the min level (no zoom)?");
std::shared_ptr<ParameterLink<bool>> AsteroidGazingWorld::integrateFitnessPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-integrateFitness", true,
                                 "should the fitness be integrated over simulation time? (default: yes)");
std::shared_ptr<ParameterLink<int>> AsteroidGazingWorld::numTriggerBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING-numTriggerBits", 0,
                                 "how many trigger bits should be used? the bits will be ANDed (default: none)");

AsteroidGazingWorld::AsteroidGazingWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {
	// Localizing settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	datasetPath = datasetPathPL->get(PT_);
	datasetParser = std::make_shared<AsteroidsDatasetParser>(datasetPath);

	// Drawing the rest of the owl
	currentAsteroidName = std::make_shared<std::string>("");
	stateSchedule = std::make_shared<ExhaustiveAsteroidGazingSchedule>(currentAsteroidName, datasetParser);

/*
	sensors = std::make_shared<AbsoluteFocusingSaccadingEyesSensors>(currentAsteroidName,
	                                                                 datasetParser,
	                                                                 foveaResolutionPL->get(PT_),
	                                                                 maxZoomPL->get(PT_),
	                                                                 splittingFactorPL->get(PT_),
	                                                                 activeThresholdingDepthPL->get(PT_),
	                                                                 lockAtMaxZoomPL->get(PT_),
	                                                                 startZoomedInPL->get(PT_));
*/
	sensors = std::make_shared<PeripheralAndRelativeSaccadingEyesSensors>(currentAsteroidName,
	                                                                      datasetParser,
	                                                                      28, // frameResolution
	                                                                      4, // peripheralFOVResolution
	                                                                      2, // foveaResolutionPL->get(PT_),
	                                                                      0, // jumpType
	                                                                      3); // jumpGradations
	mentalImage = std::make_shared<DigitMentalImage>(currentAsteroidName,
	                                                 datasetParser,
	                                                 sensors,
	                                                 numTriggerBitsPL->get(PT_),
	                                                 integrateFitnessPL->get(PT_));
	makeMotors();
};
