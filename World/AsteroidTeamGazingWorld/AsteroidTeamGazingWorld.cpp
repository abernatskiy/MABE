#include "AsteroidTeamGazingWorld.h"

#include "../AsteroidGazingWorld/MentalImages/CompressedMentalImage.h"
//#include "MentalImages/DigitMentalImage.h"
//#include "MentalImages/SpikesOnCubeMentalImage.h"
//#include "MentalImages/SpikesOnCubeFullMentalImage.h"
//#include "MentalImages/IdentityMentalImage.h"

#include "../AsteroidGazingWorld/Schedules/AsteroidGazingSchedules.h"

#include "../../Brain/DEMarkovBrain/DEMarkovBrain.h"

#include "../../Utilities/nlohmann/json.hpp" // https://github.com/nlohmann/json/ v3.6.1

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
std::shared_ptr<ParameterLink<std::string>> AsteroidTeamGazingWorld::pathToBrainGraphJSONPL =
  Parameters::register_parameter("WORLD_ASTEROID_TEAM_GAZING-pathToBrainGraphJSON", (std::string) "./braindag.json",
                                 "path to the JSON file describing how the multiple brains should be connected and where to read the constant ones");

int AsteroidTeamGazingWorld::initialConditionsInitialized = 0;
std::map<std::string,std::vector<Range2d>> AsteroidTeamGazingWorld::commonRelativeSensorsInitialConditions;

AsteroidTeamGazingWorld::AsteroidTeamGazingWorld(std::shared_ptr<ParametersTable> PT_) :
	AbstractSlideshowWorld(PT_),
	numBrains(0),
	numBrainsOutputs(0) {

	// Localizing and validating settings
	brainUpdatesPerAsteroid = brainUpdatesPerAsteroidPL->get(PT_);
	datasetPath = datasetPathPL->get(PT_);
	std::string sensorType = sensorTypePL->get(PT_);
	bool randomizeInitialConditions = (numRandomInitialConditionsPL->get(PT_) > 0);
	unsigned numRandomInitialConditions = static_cast<unsigned>(numRandomInitialConditionsPL->get(PT_));
	if(randomizeInitialConditions && sensorType!="relative") {
		std::cerr << "Sensors of type " << sensorType << " do not support initial conditions randomization" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Preparing the sensors and their environment
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

	// Preparing the brain graph
	std::ifstream brainsJSONStream(pathToBrainGraphJSONPL->get(PT_));
	nlohmann::json brainsJSON;
	brainsJSON << brainsJSONStream;

	brainsDiagram.insertNode(-1, {}); // the sensor

	unsigned numEvolvableBrains = 0;
	for(const auto& brainJSON : brainsJSON) { // for each brain description on the JSON
		if(brainJSON["type"]!="DEMarkovBrain") {
			std::cerr << "AsteroidTeamGazingWorld: components of types other than DEMarkovBrain are not surrently supported" << std::endl;
			exit(EXIT_FAILURE);
		}

		// make records where it is possible without conditioning on evolvability
		brainsIDs.push_back(brainJSON["id"]);
		brainsNumOutputs.push_back(brainJSON["numOutputs"]);
		brainsNumHidden.push_back(brainJSON["numHidden"]);
		brainsInputCached.push_back(brainJSON["cacheInputs"] ? 1 : 0);
		brainsInputCache.push_back({});
		if(brainJSON["exposeOutput"]) {
			exposedOutputs.push_back(numBrains);
			numBrainsOutputs += brainsNumOutputs.back();
		}

		// add a node to the map graph
		std::vector<int> componentParentIdxs;
		for(size_t ci=0; ci<numBrains; ci++) { // component index
			for(const std::string& parentID : brainJSON["feedsFrom"]) {
				if(parentID==brainsIDs[ci])
					componentParentIdxs.push_back(ci);
				else if(parentID=="thesensor")
					componentParentIdxs.push_back(-1);
			}
		}

		brainsDiagram.insertNode(numBrains, componentParentIdxs);

		// depending on whether the brain is evolvable or not, we do quite a few things differently
		if(brainJSON["evolvable"]) {
			brainsEvolvable.push_back(1);
			numEvolvableBrains++;
			if(numEvolvableBrains>1) {
				std::cerr << "AsteroidTeamGazingWorld: DAG brains with more than one evolvable component are not supported at the moment" << std::endl;
				exit(EXIT_FAILURE);
			}
			if(PT_->lookupInt("BRAIN_DEMARKOV-hiddenNodes") != brainsNumHidden.back()) {
				std::cerr << "AsteroidTeamGazingWorld: number of hidden nodes in MABE config (" << (PT_->lookupInt("BRAIN_DEMARKOV-hiddenNodes"))
				          << ") must be equal to the number of hidden nodes for the evolvable component listed in the brain DAG JSON description ("
				          << brainsNumHidden.back() << ", read from " << pathToBrainGraphJSONPL->get(PT_) << ")" << std::endl;
				exit(EXIT_FAILURE);
			}

			brains.push_back(nullptr);
		}
		else {
			brainsEvolvable.push_back(0);

			// constructing the constant component

			shared_ptr<ParametersTable> curPT = make_shared<ParametersTable>("", nullptr);
			// component-specific parameters
			curPT->setParameter("BRAIN_DEMARKOV-hiddenNodes", static_cast<int>(brainsNumHidden.back()));
			///// Parameter forwarding /////
			curPT->setParameter("BRAIN_DEMARKOV_ADVANCED-recordIOMap",
			                    PT_->lookupBool("BRAIN_DEMARKOV_ADVANCED-recordIOMap"));
			curPT->setParameter("BRAIN_MARKOV_GATES_DETERMINISTIC-IO_Ranges",
			                    PT_->lookupString("BRAIN_MARKOV_GATES_DETERMINISTIC-IO_Ranges"));
			const vector<string> forwardedDoubleParamNames = { "BRAIN_DEMARKOV-gateInsertionProbability",
			                                                   "BRAIN_DEMARKOV-gateDuplicationProbability",
			                                                   "BRAIN_DEMARKOV-gateDeletionProbability",
			                                                   "BRAIN_DEMARKOV-connectionToTableChangeRatio" };
			for(const auto& param : forwardedDoubleParamNames)
				curPT->setParameter(param, PT_->lookupDouble(param));
			const vector<string> forwardedIntParamNames = { "BRAIN_DEMARKOV-initialGateCount",
			                                                "BRAIN_DEMARKOV-minGateCount" };
			for(const auto& param : forwardedIntParamNames)
				curPT->setParameter(param, PT_->lookupInt(param));
			///// Parameter forwarding END /////

			unsigned numComponentInputs = 0;
			for(int parentIdx : brainsDiagram.getParents(numBrains)) {
				if(parentIdx<0)
					numComponentInputs += sensors->numOutputs();
				else
					numComponentInputs += brainsNumOutputs[parentIdx];
			}

			std::shared_ptr<AbstractBrain> component = DEMarkovBrain_brainFactory(numComponentInputs, brainsNumOutputs.back(), curPT);

			// deserializing the description file
			std::ifstream componentFile(brainJSON["loadFrom"]);
			std::string componentJSONStr;
			std::getline(componentFile, componentJSONStr);

			std::string name("currentComponent");
			std::unordered_map<std::string,std::string> surrogateOrgData { { std::string("BRAIN_") + name + "_json", std::string("'") + componentJSONStr + std::string("'") } };
      component->deserialize(curPT, surrogateOrgData, name);

			brains.push_back(component);
		}

		numBrains++;
	}

	// Preparing the mental Image and adding motors that will be shaping it
	mentalImage = std::make_shared<CompressedMentalImage>(currentAsteroidName,
	                                                      datasetParser,
	                                                      sensors,
	                                                      numBrainsOutputs,
	                                                      mihPatternChunkSizeBitsPL->get(PT_),
	                                                      fastRepellingPLInfoNumNeighborsPL->get(PT_),
	                                                      leakBaseMultiplierPL->get(PT_),
	                                                      leakDecayRadiusPL->get(PT_));
	makeMotors();

	std::cout << "Done constructing AsteroidTeamGazingWorld" << std::endl;
	printVector(brains, "brains");
	printVector(brainsIDs, "brainIDs");
	printVector(brainsEvolvable, "brainsEvolvable");
	printVector(brainsNumOutputs, "brainsNumOutputs");
	printVector(brainsNumHidden, "brainsNumHidden");
	printVector(brainsInputCached, "brainsInputCached"); // format: 0 = do not cache, 1 = input to be cached, 2 = input cached
//	printVector(brainsInputCache);
	printVector(exposedOutputs, "exposedOutputs");
	std::cout << "Total exposed outputs: " << numBrainsOutputs << std::endl;
	std::cout << "Brain graph:" << std::endl; brainsDiagram.printGraph();
};
