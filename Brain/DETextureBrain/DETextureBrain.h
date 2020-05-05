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

	// Major state vars
	Texture* input; // we do not modify the input, but occasionally we may want to switch to another complete one - hence a pointer to const
	Texture* output;
	std::vector<std::shared_ptr<AbstractTextureGate>> gates;
	// keep in mind that this class is responsible for maintaining the output pointers of the gates in a valid state
	// it follows that every time this class resets a gate, generates a new one or modifies one's output connections, it must call gate->updateOutputs(output)
	// the inputs do not require such care as they are updated by attachToSensors() before every evalution

	// Private methods
	std::tuple<size_t,size_t,size_t> computeOutputShape(); // new, OK
	void resetOutputTexture(); // new, OK

	void mutateStructurewide(); // OK
	void mainMutate(); // OK
	void randomize(); // OK
	void rewireGateRandomly(size_t gateIdx);
	std::shared_ptr<AbstractTextureGate> getGateCopy(size_t oldGateIdx, unsigned newGateID); // OK
	std::shared_ptr<AbstractTextureGate> getRandomGate(unsigned gateID); // OK
	TextureIndex getRandomInputTextureAddress(); // new, OK
	TextureIndex getRandomOutputTextureAddress(); // new, OK
	unsigned getLowestAvailableGateID(); // OK
//	void beginLogging();
//	void logBrainStructure();
	void validateDimensions(); // throws exceptions, OK
	void validateInput(); // throws exceptions, OK

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
//	static std::shared_ptr<ParameterLink<bool>> logicalConvolutionPL; // make it an int regime?

	static std::shared_ptr<ParameterLink<std::string>> gateIORangesPL;
	static std::shared_ptr<ParameterLink<int>> initialGateCountPL;
	static std::shared_ptr<ParameterLink<double>> gateInsertionProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> gateDeletionProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> gateDuplicationProbabilityPL;
	static std::shared_ptr<ParameterLink<double>> connectionToTableChangeRatioPL;
	static std::shared_ptr<ParameterLink<int>> minGateCountPL;
	static std::shared_ptr<ParameterLink<bool>> readFromInputsOnlyPL;
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

	// constructors OK

	void update() override; // OK?
	std::shared_ptr<AbstractBrain> makeCopy(std::shared_ptr<ParametersTable> PT_ = nullptr) override;
	std::shared_ptr<AbstractBrain> makeBrain(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // returns a random brain like the one from which it's called
	std::shared_ptr<AbstractBrain> makeBrainFrom(std::shared_ptr<AbstractBrain> brain,
	                                             std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override { return makeBrainFromMany({brain}, _genomes); };
	std::shared_ptr<AbstractBrain> makeBrainFromMany(std::vector<std::shared_ptr<AbstractBrain>> _brains,
	                                                 std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) override; // brains' procreation

	// copiers OK

	void mutate() override; // OK
	void resetBrain() override; // OK
	void attachToSensors(void*) override; // sets the input address and updates the pointers in gates // OK
	std::unordered_set<std::string> requiredGenomes() override { return {}; } // OK

	// Infrastructure
	std::string description() override; // OK
	DataMap getStats(std::string& prefix) override; // OK
	std::string getType() override { return "DETexture"; } // OK
//	void logNote(std::string note) override { log.log("DETextureBrain's external note: " + note + "\n"); };
	DataMap serialize(std::string& name) override; // OK
	void deserialize(std::shared_ptr<ParametersTable> PT, std::unordered_map<std::string,std::string>& orgData, std::string& name) override; // OK
	void* getDataForMotors() override { return output; };
};

inline std::shared_ptr<AbstractBrain> DETextureBrain_brainFactory(int ins, int outs, std::shared_ptr<ParametersTable> PT, bool randomize = true) {
	return std::make_shared<DETextureBrain>(ins, outs, PT, randomize);
}
