#include <algorithm>

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
	numBrainsOutputs(0),
	cachingComplete(false) {

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
	unsigned numBrainsConnectedToSensors = 0;
	unsigned numBrainsConnectedToOculomotors = 0;
	for(const auto& brainJSON : brainsJSON) { // for each brain description on the JSON
		if(brainJSON["type"]!="DEMarkovBrain") {
			std::cerr << "AsteroidTeamGazingWorld: components of types other than DEMarkovBrain are not surrently supported" << std::endl;
			exit(EXIT_FAILURE);
		}

		// make records where it is possible without conditioning on evolvability
		brainsIDs.push_back(brainJSON["id"]);
		brainsNumOutputs.push_back(brainJSON["numOutputs"]);
		brainsNumHidden.push_back(brainJSON["numHidden"]);
		if(brainJSON["exposeOutput"]) {
			exposedOutputs.push_back(numBrains);
			numBrainsOutputs += brainsNumOutputs.back();
		}
		brainsConnectedToOculomotors.push_back( (brainJSON.count("connectToOculomotors")==0 || !brainJSON["connectToOculomotors"]) ? 0 : 1 );
		if(brainsConnectedToOculomotors.back()==1) {
			numBrainsConnectedToOculomotors++;
			if(numBrainsConnectedToOculomotors>1) {
				std::cerr << "AsteroidTeamGazingWorld: more than one component requests a connection to oculomotors, which is not supported" << std::endl;
				exit(EXIT_FAILURE);
			}
			if(brainJSON["feedsFrom"].size()!=1 || brainJSON["feedsFrom"][0]!="thesensor") {
				std::cerr << "AsteroidTeamGazingWorld: current implementation requires that a component that is connected to oculomotors must feed only from the sensor. Component " << numBrains << " feeds instead from [";
				for(const auto& ff : brainJSON["feedsFrom"])
					std::cerr << " " << ff;
				std::cerr << " ]" << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		// add a node to the map graph
		std::vector<int> componentParentIdxs;
		for(const std::string& parentID : brainJSON["feedsFrom"]) {
			if(parentID=="thesensor") {
				numBrainsConnectedToSensors++;
				componentParentIdxs.push_back(-1);
				if(brainJSON["feedsFrom"].size()!=1) {
					std::cerr << "AsteroidTeamGazingWorld: currently, any component that feeds from the sensor must not feed from anything else. Component " << numBrains << " feeds from [";
					for(const auto& pp : brainJSON["feedsFrom"])
						std::cerr << " " << pp;
					std::cerr << " ]" << std::endl;
					exit(EXIT_FAILURE);
				}
				if(numBrainsConnectedToSensors>1 && (sensors->numInputs()!=0))
					std::cerr << "AsteroidTeamGazingWorld: WARNING! Multiple components are connected to active sensors. The measured number of saccades will only reflect the sensor usage by the last component utilizing them" << std::endl;
			}
			else {
				for(size_t ci=0; ci<numBrains; ci++) // component index
					if(parentID==brainsIDs[ci])
						componentParentIdxs.push_back(ci);
			}
		}
		std::sort(componentParentIdxs.begin(), componentParentIdxs.end());
		// std::cout << "For brain " << numBrains << " parents were"; for(int p : componentParentIdxs) std::cout << " " << p; std::cout << std::endl;
		brainsDiagram.insertNode(numBrains, componentParentIdxs);

		// preparing cache variables
		if(brainJSON["cacheOutputs"]) {
			if(!assumeDeterministicEvaluations) {
				std::cerr << "AsteroidTeamGazingWorld: caching only supported if deterministic evaluations are assumed. It was requested for outputs of component " << brainsIDs.back() << std::endl;
				exit(EXIT_FAILURE);
			}
			if(evaluationsPerGeneration!=1) {
				std::cerr << "AsteroidTeamGazingWorld: multiple evaluations per generation (" << evaluationsPerGeneration << ") are not supported alongside caching (requested for outputs of component " << brainsIDs.back() << ")" << std::endl;
				exit(EXIT_FAILURE);
			}
			if(brainJSON["evolvable"]) {
				std::cerr << "AsteroidTeamGazingWorld: caching of outputs of an evolvable component " << brainsIDs.back() << " requested, exiting" << std::endl;
				exit(EXIT_FAILURE);
			}
			for(int pi : brainsDiagram.getAllAncestors(numBrains)) { // parent index
				if(pi>=0 && brainsEvolvable[pi]) {
					std::cerr << "AsteroidTeamGazingWorld: cannot cache inputs for component " << brainsIDs.back() << " which has an evolvable ancestor " << brainsIDs[pi] << std::endl;
					exit(EXIT_FAILURE);
				}
			}
			brainsOutputCached.push_back(1);
		}
		else
			brainsOutputCached.push_back(0);
		brainsOutputCache.push_back({});

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

			unsigned numComponentOutputs = brainsNumOutputs.back();
			if(brainsConnectedToOculomotors.back())
				numComponentOutputs += sensors->numInputs();

			std::shared_ptr<AbstractBrain> component = DEMarkovBrain_brainFactory(numComponentInputs, numComponentOutputs, curPT);

			// deserializing the description file
			if(brainJSON.count("loadFrom")==0) {
				std::cerr << "AsteroidTeamGazingWorld: fixed brain component " << numBrains << " must be loaded from file and not loadFrom field has been supplied in the config" << std::endl;
				exit(EXIT_FAILURE);
			}
			std::ifstream componentFile(brainJSON["loadFrom"]);
			if(!componentFile.is_open()) {
				std::cerr << "AsteroidTeamGazingWorld: couldn't open file " << brainJSON["loadFrom"] << " to load brain component " << numBrains << " from" << std::endl;
				exit(EXIT_FAILURE);
			}

			std::string componentJSONStr;
			std::getline(componentFile, componentJSONStr);

			std::string name("currentComponent");
			std::unordered_map<std::string,std::string> surrogateOrgData { { std::string("BRAIN_") + name + "_json", std::string("'") + componentJSONStr + std::string("'") } };
      component->deserialize(curPT, surrogateOrgData, name);

			brains.push_back(component);
		}

		numBrains++;
	}

	if(numEvolvableBrains==0) {
		std::cerr << "AsteroidTeamGazingWorld: at least one brain component should be evolvable" << std::endl;
		std::cerr << "(actually, exactly one in the current implementation)" << std::endl;
		exit(EXIT_FAILURE);
	}
	if(sensors->numInputs()!=0 && numBrainsConnectedToOculomotors==0) {
		std::cerr << "AsteroidTeamGazingWorld: at least one brain component should be marked as connected to oculomotors when sensors are active (current ones have " << sensors->numInputs() << " inputs)" << std::endl;
		exit(EXIT_FAILURE);
	}
	if(exposedOutputs.size()==0) {
		std::cerr << "AsteroidTeamGazingWorld: at least one brain component must be exposed" << std::endl;
		exit(EXIT_FAILURE);
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
	motors = nullptr;

	std::cout << "Done constructing AsteroidTeamGazingWorld" << std::endl;
	printVector(brains, "brains");
	printVector(brainsIDs, "brainIDs");
	printVector(brainsEvolvable, "brainsEvolvable");
	printVector(brainsNumOutputs, "brainsNumOutputs");
	printVector(brainsNumHidden, "brainsNumHidden");
	printVector(brainsOutputCached, "brainsOutputCached"); // format: 0 = do not cache, 1 = input to be cached, 2 = input cached
//	printVector(brainsOutputCache);
	printVector(exposedOutputs, "exposedOutputs");
	printVector(brainsConnectedToOculomotors, "brainsConnectedToOculomotors");
	std::cout << "Total exposed outputs: " << numBrainsOutputs << std::endl;
	std::cout << "Brain graph:" << std::endl; brainsDiagram.printGraph();
};

void AsteroidTeamGazingWorld::resetWorld(int visualize) {
	AbstractSlideshowWorld::resetWorld(visualize);
	for(unsigned i=0; i<numBrains; i++)
		if(brainsEvolvable[i])
			brains[i] = nullptr;
}

void AsteroidTeamGazingWorld::evaluateOnce(std::shared_ptr<Organism> org, unsigned repIdx, int visualize) {

	if(visualize) std::cout << "Evaluating organism " << org->ID << " at " << org << std::endl;
//	std::cout << "Evaluating organism " << org->ID << " at " << org << std::endl;

//	org->translateGenomesToBrains();

	resetWorld(visualize);

	for(unsigned i=0; i<numBrains; i++) {
		if(brainsEvolvable[i]) {
			brains[i] = org->brains[brainName];
			break;
		}
	}

	unsigned long timeStep = 0;
	while(!stateSchedule->stateIsFinal()) {
		std::vector<StateTimeSeries> rawExposedOutputsTS;
		for(int eo : exposedOutputs)
			rawExposedOutputsTS.push_back(executeBrainComponent(eo, visualize));
		// for(unsigned eoi=0; eoi<rawExposedOutputsTS.size(); eoi++) { std::cout << "Exposed component " << eoi << " (idx " << exposedOutputs[eoi] << "):" << std::endl; for(unsigned t=0; t<rawExposedOutputsTS[eoi].size(); t++) printVector(rawExposedOutputsTS[eoi][t], "t=" + std::to_string(t)); }

		// std::cout << "Iterating over timesteps and feeding the concatenated exposed outputs to the mental image:" << std::endl;

		for(unsigned t=0; t<brainUpdatesPerAsteroid; t++) {
			std::vector<double> outputs;
			for(auto& reots : rawExposedOutputsTS)
				outputs.insert(outputs.end(), reots[t].begin(), reots[t].end());
			// printVector(outputs, "t=" + std::to_string(t));

			// from here on outputs[] contains the final values and we can begin to process them
			mentalImage->updateWithInputs(outputs);
			// recordRunningScores(org, timeStep, visualize); // not used anymore, this comment is just to remind me of where it used to be called
			mentalImage->recordRunningScoresWithinState(org, t, brainUpdatesPerAsteroid);
			timeStep++;
		}
		if(visualize) {
			std::string stateLabel = std::string("state_") + stateSchedule->currentStateDescription();
			void* sts; void* bts; void* mits; void* mts = nullptr;
			sts = sensors->logTimeSeries(stateLabel);
			bts = brain->logTimeSeries(stateLabel);
			mits = mentalImage->logTimeSeries(stateLabel);
			timeSeriesLogger->logData(stateLabel, sts, bts, mts, mits, visualize);
		}
		mentalImage->resetAfterWorldStateChange(visualize); // lol before

		stateSchedule->advance(visualize);
	}

	recordSampleScores(org, timeStep, visualize);

	if(!cachingComplete) {
		cachingComplete = true;
		for(unsigned i=0; i<numBrains; i++)
			if(brainsOutputCached[i]==1)
				brainsOutputCached[i] = 2;
	}

	if(assumeDeterministicEvaluations)
		org->dataMap.set("evaluated", true);

	// std::cout << "Done evaluating organism " << org->ID << " at " << org << std::endl;
	// std::cout << "Here's what it looks like evaluated: " << (org->getJSONRecord()).dump() << std::endl;
	// exit(EXIT_FAILURE);
}

std::unordered_map<std::string, std::unordered_set<std::string>> AsteroidTeamGazingWorld::requiredGroups() {
	unsigned nInputs = 0;
	unsigned nOutputs = 0;

	for(unsigned i=0; i<numBrains; i++) {
		if(brainsEvolvable[i]) {
			for(int parent : brainsDiagram.getParents(i))
				nInputs += parent==-1 ? sensors->numOutputs() : brainsNumOutputs[parent];

			nOutputs += brainsNumOutputs[i];
			if(brainsConnectedToOculomotors[i])
				nOutputs += sensors->numInputs();

			// std::cout << "The brain at " << i << " is evolvable with " << nInputs << " inputs and " << nOutputs << " outputs" << std::endl;
			// std::cout << "requiredGroups() is about to return {{" << groupName << ", {" << ("B:" + brainName + "," + std::to_string(nInputs) + "," + std::to_string(nOutputs)) << "}}}" << std::endl;

			return {{groupName, {"B:" + brainName + "," + std::to_string(nInputs) + "," + std::to_string(nOutputs)}}};
		}
	}

	std::cerr << "AsteroidTeamGazingWorld: at least one component should be evolvable" << std::endl;
	exit(EXIT_FAILURE);
}

/*****************************************/
/********** Private definitions **********/
/*****************************************/

StateTimeSeries AsteroidTeamGazingWorld::executeBrainComponent(unsigned idx, int visualize) {

	std::string stateID = stateSchedule->currentStateDescription();

	if(brainsOutputCached[idx]==2) {
		// std::cout << "Retrieved outputs from cache " << idx << " (id " << brainsIDs[idx] << ") for state " << stateSchedule->currentStateDescription() << ":" << std::endl;
		// for(const auto& v : brainsOutputCache[idx][stateID])
		// 	printVector(v, "ts");
		return brainsOutputCache[idx][stateID];
	}

	brains[idx]->resetBrain();
	StateTimeSeries componentOutputTimeSeries;
	auto parents = brainsDiagram.getParents(idx);

	if(parents.size()==1 && parents[0]==-1) {
		// we're feeding from sensors
		// std::cout << "Brain " << idx << " feeds from sensors" << std::endl;
		// std::cout << "Attaching sensors to brain " << brains[idx] << std::endl;
		sensors->reset(visualize);
		sensors->attachToBrain(brains[idx]);
		for(unsigned t=0; t<brainUpdatesPerAsteroid; t++) {
			sensors->update(visualize);
			brains[idx]->update();

			std::vector<double> componentOutput(brainsNumOutputs[idx]);
			for(unsigned i=0; i<brainsNumOutputs[idx]; i++)
				componentOutput[i] = brains[idx]->readOutput(sensors->numInputs() + i); // we're attached to sensors and the sensors know how many outputs they claimed for controls, so we can just trust them
			componentOutputTimeSeries.push_back(componentOutput);
		}
	}
	else {
		// we're feeding from some lower layers
		// std::cout << "Brain " << idx << " feeds from lower layers, as its parents are";
		// for(int p : parents) std::cout << " " << p; std::cout << std::endl;
		std::vector<StateTimeSeries> parentsOutputs;
		for(int p : parents)
			parentsOutputs.push_back(executeBrainComponent(p, visualize));

		// for(unsigned eoi=0; eoi<parentsOutputs.size(); eoi++) { std::cout << "Parent " << eoi << " (idx " << parents[eoi] << "):" << std::endl; for(unsigned t=0; t<parentsOutputs[eoi].size(); t++) printVector(parentsOutputs[eoi][t], "t=" + std::to_string(t)); }

		for(unsigned t=0; t<brainUpdatesPerAsteroid; t++) {
			unsigned inputIdx = 0;
			for(unsigned pi=0; pi<parents.size(); pi++) {
				for(double v : parentsOutputs[pi][t]) {
					brains[idx]->setInput(inputIdx, v);
					inputIdx++;
				}
			}
			brains[idx]->update();

			std::vector<double> componentOutput(brainsNumOutputs[idx]);
			for(unsigned i=0; i<brainsNumOutputs[idx]; i++)
				componentOutput[i] = brains[idx]->readOutput(i); // since only the bottommost components are allowed to interface with sensors, no shift is needed here
			componentOutputTimeSeries.push_back(componentOutput);
		}
	}

	if(brainsOutputCached[idx]==1)
		brainsOutputCache[idx][stateID] = componentOutputTimeSeries;

	return componentOutputTimeSeries;
}
