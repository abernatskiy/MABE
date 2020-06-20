#include "AsteroidTextureGazingWorld.h"

#include <limits>
#include "../../Utilities/Random.h"

//#include "../AsteroidGazingWorld/MentalImages/CompressedMentalImage.h"
#include "../AsteroidGazingWorld/MentalImages/ShapeMentalImage.h"

#include "../AsteroidGazingWorld/Sensors/CompleteViewSensors.h"
#include "../AsteroidGazingWorld/Schedules/AsteroidGazingSchedules.h"

using namespace std;

shared_ptr<ParameterLink<int>> AsteroidTextureGazingWorld::brainUpdatesPerAsteroidPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-brainUpdatesPerAsteroid", 1,
                                 "number of time steps for which the brain is allowed to gaze at each asteroid");
shared_ptr<ParameterLink<string>> AsteroidTextureGazingWorld::datasetPathPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-datasetPath", (string) "./asteroids",
                                 "path to the folder containing the asteroids shapes and snapshots dataset");
shared_ptr<ParameterLink<int>> AsteroidTextureGazingWorld::compressToBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-compressToBits", 20,
                                 "if CompressedMentalImage is used, how many bits should it compress the input to? (default: 20)");
shared_ptr<ParameterLink<int>> AsteroidTextureGazingWorld::fastRepellingPLInfoNumNeighborsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-fastRepellingPLInfoNumNeighbors", 10,
                                 "if CompressedMentalImage is used AND fastRepellingPatternLabelInformation is being computed,\n"
	                               "how many neighbors should the computation take into account? (default: 10)");
shared_ptr<ParameterLink<int>> AsteroidTextureGazingWorld::mihPatternChunkSizeBitsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-mihPatternChunkSizeBits", 16,
                                 "if CompressedMentalImage is used AND fastRepellingPatternLabelInformation is being computed,\n"
	                               "the nearest neighbors will be looked up with multi-index hashing algorithm. What is the chunk\n"
	                               "size that should be used for that algorithm? Max efficiency is reached at log2(N), where\n"
	                               "N is the number of asteroids/slides, assuming that output patterns are uniformly distributed.\n"
	                               "Must be 4, 8, 12, 16, 20, 24, 28 or 32. (default: 16)\n");
shared_ptr<ParameterLink<double>> AsteroidTextureGazingWorld::leakBaseMultiplierPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-leakBaseMultiplier", 1.,
                                 "if CompressedMentalImage is used AND any kind of repelling mutual information is being computed,\n"
	                               "the leaks between pattern-label pairs will be computed as b*exp(-d/a), where d is the Hammming\n"
	                               "distance from the neighbor of the point. What the value of b should be? (default: 1.)");
shared_ptr<ParameterLink<double>> AsteroidTextureGazingWorld::leakDecayRadiusPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-leakDecayRadius", 1.,
                                 "if CompressedMentalImage is used AND any kind of repelling mutual information is being computed,\n"
	                               "the leaks between pattern-label pairs will be computed as b*exp(-d/a), where d is the Hammming\n"
	                               "distance from the neighbor of the point. What the value of a should be? Cannot be zero. (default: 1.)");
shared_ptr<ParameterLink<int>> AsteroidTextureGazingWorld::minibatchSizePL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-minibatchSize", 0,
                                 "if nonzero, this many datapoints will be selected from the dataset at every generation for\n"
	                               "evaluation purposes (default: 0)");
shared_ptr<ParameterLink<bool>> AsteroidTextureGazingWorld::balanceMinibatchesPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-balanceMinibatches", true,
                                 "should the minibatches be balanced wrt labels? (default: yes)");
shared_ptr<ParameterLink<bool>> AsteroidTextureGazingWorld::overwriteEvaluationsPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEXTURE_GAZING-overwriteEvaluations", true,
                                 "should the world overwrite evaluations (as opposed to appending new ones) in the organism's datamap? (default: yes)");

int AsteroidTextureGazingWorld::schedulesRandomSeed = -1;

AsteroidTextureGazingWorld::AsteroidTextureGazingWorld(shared_ptr<ParametersTable> PT_) :
	AbstractSlideshowWorld(PT_) {

	// Localizing settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	datasetPath = datasetPathPL->get(PT_);

	// Drawing the rest of the owl
	datasetParser = make_shared<AsteroidsDatasetParser>(datasetPath);
	currentAsteroidName = make_shared<string>("");

	if(schedulesRandomSeed == -1) {
		schedulesRandomSeed = Random::getInt(numeric_limits<int>::max());
		cout << "Made a common random seed for schedules: " << schedulesRandomSeed << endl << flush;
	}

	int minibatchSize = minibatchSizePL->get(PT_);
	if(minibatchSize == 0) {
		stateSchedule = make_shared<ExhaustiveAsteroidGazingSchedule>(currentAsteroidName, datasetParser);
	}
	else {
		stateSchedule = make_shared<MinibatchAsteroidGazingSchedule>(currentAsteroidName,
		                                                             datasetParser,
		                                                             minibatchSize,
		                                                             balanceMinibatchesPL->get(PT_),
		                                                             schedulesRandomSeed);
	}

	sensors = make_shared<CompleteViewSensors>(currentAsteroidName, datasetParser, PT_);
	sensors->writeSensorStats();
	sensors->doHeavyInit();

	mentalImage = make_shared<ShapeMentalImage>(currentAsteroidName, datasetParser);
/*
	mentalImage = make_shared<CompressedMentalImage>(currentAsteroidName,
	                                                 datasetParser,
	                                                 sensors,
	                                                 compressToBitsPL->get(PT_),
	                                                 false,
	                                                 mihPatternChunkSizeBitsPL->get(PT_),
	                                                 fastRepellingPLInfoNumNeighborsPL->get(PT_),
	                                                 leakBaseMultiplierPL->get(PT_),
	                                                 leakDecayRadiusPL->get(PT_),
	                                                 true,
	                                                 overwriteEvaluationsPL->get(PT_));
*/
	makeMotors();
};
