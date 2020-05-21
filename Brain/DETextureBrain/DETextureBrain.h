#pragma once

#include <cmath>
#include <memory>
#include <iostream>
#include <set>
#include <vector>
#include <iomanip>
#include "boost/multi_array.hpp"

#include "../AbstractBrain.h"
#include "Gate/AbstractTextureGate.h"
#include "../../Utilities/Random.h"
#include "../MarkovBrain/logFile.h"
#include "../../Utilities/texture.h"

#define UNSHARED_REGIME 0
#define SHARED_REGIME 1

// This brain expects a vector of two doubles that encode a pointer to a a Texture (boost::multi_array) with axes [x, y, t, channel]

class DETextureBrain : public AbstractBrain {
private:
	// Major constants
	size_t gateMinIns, gateMaxIns, gateMinOuts, gateMaxOuts;
	size_t minGates;
	size_t inputSizeX, inputSizeY, inputSizeT;
	size_t filterSizeX, filterSizeY, filterSizeT;
	size_t strideX, strideY, strideT;
	size_t outputSizeX, outputSizeY, outputSizeT;
	size_t inBitsPerPixel, outBitsPerPixel;
	const int convolutionRegime;

	// Major state vars
	Texture* input;
	Texture* output;
	boost::multi_array<std::vector<std::shared_ptr<AbstractTextureGate>>,3> filters;
	// keep in mind that this class is responsible for maintaining the output pointers of the gates in a valid state
	// it follows that every time this class resets a gate, generates a new one or modifies one's output connections, it must call gate->updateOutputs(output)
	// the inputs do not require such care as they are updated by attachToSensors() before every evalution

	// Private methods
	std::tuple<size_t,size_t,size_t> computeOutputShape();
	unsigned totalNumberOfGates();
	unsigned totalNumberOfFilters() { return outputSizeX*outputSizeY*outputSizeT; };
	void resetOutputTexture();

	void mutateStructurewide();
	void mainMutate();
	void randomize();
	void internallyMutateRandomlySelectedGate();
	void randomlyRewireRandomlySelectedGate();
	void addCopyOfRandomlySelectedGate();
	void addRandomGate();
	void deleteRandomlySelectedGate();

	std::tuple<size_t,size_t,size_t> getRandomFilterIndex();
	TextureIndex getRandomFilterInputIndex();
	TextureIndex getRandomFilterOutputIndex();
	unsigned getLowestAvailableGateID();
//	void beginLogging();
//	void logBrainStructure();
	void validateDimensions(); // throws exceptions
	void validateInput(); // throws exceptions
	void validateBrain(); // prints angry messages to stderr

	// Mutation-related variables
	const double structurewideMutationProbability;
	std::string originationStory;

	// Infrastructure
	const bool visualize;
//	LogFile log;

	static std::shared_ptr<ParameterLink<std::string>> inTextureShapePL;
	static std::shared_ptr<ParameterLink<std::string>> filterShapePL;
	static std::shared_ptr<ParameterLink<std::string>> strideShapePL;
	static std::shared_ptr<ParameterLink<int>> inBitsPerPixelPL;
	static std::shared_ptr<ParameterLink<int>> outBitsPerPixelPL;
	static std::shared_ptr<ParameterLink<int>> convolutionRegimePL;

	static std::shared_ptr<ParameterLink<std::string>> gateIORangesPL;
	static std::shared_ptr<ParameterLink<int>> initialGateCountPL;
	static std::shared_ptr<ParameterLink<double>> gateInsertionProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> gateDeletionProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> gateDuplicationProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> connectionToTableChangeRatioPL;
	static std::shared_ptr<ParameterLink<int>> minGateCountPL;
	static std::shared_ptr<ParameterLink<double>> structurewideMutationProbabilityPL;

public:
	// Public methods
	DETextureBrain() = delete;
	DETextureBrain(int nrInNodes, int nrOutNodes, std::shared_ptr<ParametersTable> PT); // basic constructor that provides an empty object
	DETextureBrain(int ins, int outs, std::shared_ptr<ParametersTable> PT_, bool randomizeIt) : // complete constructor that actually generates a working random brain
		DETextureBrain(ins, outs, PT_) {

		if(randomizeIt)
			randomize();

//		beginLogging();
	};
	~DETextureBrain(); // these objects own their output pointers that must be destroyed explicitly

	void update(std::mt19937* rng=nullptr) override;
	std::shared_ptr<AbstractBrain> makeCopy(std::shared_ptr<ParametersTable> PT_ = nullptr) override;
	std::shared_ptr<AbstractBrain> makeBrain(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // returns a random brain like the one from which it's called
	std::shared_ptr<AbstractBrain> makeBrainFrom(std::shared_ptr<AbstractBrain> brain,
	                                             std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override { return makeBrainFromMany({brain}, _genomes); };
	std::shared_ptr<AbstractBrain> makeBrainFromMany(std::vector<std::shared_ptr<AbstractBrain>> _brains,
	                                                 std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // brains' procreation

	void mutate() override;
	void resetBrain() override;
	void attachToSensors(void*) override; // sets the input address and updates the pointers in gates
	std::unordered_set<std::string> requiredGenomes() override { return {}; };

	// Infrastructure
	std::string description() override;
	DataMap getStats(std::string& prefix) override;
	std::string getType() override { return "DETexture"; };
//	void logNote(std::string note) override { log.log("DETextureBrain's external note: " + note + "\n"); };
	DataMap serialize(std::string& name) override;
	void deserialize(std::shared_ptr<ParametersTable> PT, std::unordered_map<std::string,std::string>& orgData, std::string& name) override;
	void* getDataForMotors() override { return output; };
};

inline std::shared_ptr<AbstractBrain> DETextureBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT, bool randomize = true) {
	return std::make_shared<DETextureBrain>(ins, outs, PT, randomize);
}
