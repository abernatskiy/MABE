#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <deque>

#include <cstdlib>

#include "../../Utilities/nlohmann/json.hpp" // https://github.com/nlohmann/json/ v3.6.1

#include "DETextureBrain.h"
#include "Gate/DeterministicTextureGate.h"

using namespace std;

/********** Auxiliary funcs **********/

tuple<int,int,int,int> parseGateLimitsStr_texture(string thestring) {
	string sinmin, sinmax, soutmin, soutmax;
	stringstream s(thestring);
	getline(s, sinmin, '-');
	getline(s, sinmax, ',');
	getline(s, soutmin, '-');
	getline(s, soutmax);
	return make_tuple(stoi(sinmin), stoi(sinmax), stoi(soutmin), stoi(soutmax));
}

tuple<size_t,size_t,size_t> parseShapeStr_texture(string thestring) {
	string sizex, sizey, sizet;
	stringstream s(thestring);
	getline(s, sizex, ',');
	getline(s, sizey, ',');
	getline(s, sizet);
	return make_tuple(stoi(sizex), stoi(sizey), stoi(sizet));
}

/********** Static variables definition **********/

shared_ptr<ParameterLink<string>> DETextureBrain::gateIORangesPL = Parameters::register_parameter("BRAIN_DETEXTURE-gateIORanges", (string) "2-2,1-1",
                                                                                                  "allowed ranges of numbers of inputs and outputs for gates (exactly two inputs and one output by default)");
shared_ptr<ParameterLink<int>> DETextureBrain::initialGateCountPL = Parameters::register_parameter("BRAIN_DETEXTURE-initialGateCount", 10,
                                                                                                   "Number of gates to add to the newly generated individuals. Keep in mind that if the brain is operating in logic-sharing convolution regine, this many gates will be added to the shared filter. This tends to result in initial brains that are too dense.");
shared_ptr<ParameterLink<double>> DETextureBrain::gateInsertionProbabilityPL = Parameters::register_parameter("BRAIN_DETEXTURE-gateInsertionProbability", 0.1,
                                                                                                              "probability that a new random gate will be inserted into the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DETextureBrain::gateDeletionProbabilityPL = Parameters::register_parameter("BRAIN_DETEXTURE-gateDeletionProbability", 0.1,
                                                                                                             "probability that a randomly chosen gate will be deleted from the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DETextureBrain::gateDuplicationProbabilityPL = Parameters::register_parameter("BRAIN_DETEXTURE-gateDuplicationProbability", 0.2,
                                                                                                                "probability that a randomly chosen gate will be duplicated within the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DETextureBrain::connectionToTableChangeRatioPL = Parameters::register_parameter("BRAIN_DETEXTURE-connectionToTableChangeRatio", 1.0,
                                                                                                                  "if the brain experiences no insertion, deletions or duplictions, call to mutate() changes parameters of a randomly chosen gate. This parameters governs relative frequencies of such changes to connections and logic tables (default: 1.0 or equal probability)");
shared_ptr<ParameterLink<int>> DETextureBrain::minGateCountPL = Parameters::register_parameter("BRAIN_DETEXTURE-minGateCount", 0,
                                                                                               "number of gates that causes gate deletions to become impossible (mutation operator calls itself if the mutation happens to be a deletion)");
shared_ptr<ParameterLink<bool>> DETextureBrain::readFromInputsOnlyPL = Parameters::register_parameter("BRAIN_DETEXTURE-readFromInputsOnly", false,
                                                                                                      "if set to true, gates will only read from input nodes; by default, they'll read from all nodes");
shared_ptr<ParameterLink<double>> DETextureBrain::structurewideMutationProbabilityPL = Parameters::register_parameter("BRAIN_DETEXTURE-structurewideMutationProbability", 0.0,
                                                                                                                      "probability of mutation of each brain component upon reproduction");
shared_ptr<ParameterLink<string>> DETextureBrain::inTextureShapePL = Parameters::register_parameter("BRAIN_DETEXTURE-inTextureShape", (string) "15,15,1",
                                                                                                    "X, Y and T dimensions of the input (default 15,15,1)");
shared_ptr<ParameterLink<string>> DETextureBrain::filterShapePL = Parameters::register_parameter("BRAIN_DETEXTURE-filterShape", (string) "3,3,1",
                                                                                                 "X, Y and T dimensions of each localized filter (default 3,3,1)");
shared_ptr<ParameterLink<string>> DETextureBrain::strideShapePL = Parameters::register_parameter("BRAIN_DETEXTURE-strideShape", (string) "3,3,1",
                                                                                                 "X, Y and T dimensions of the stride (default 3,3,1)");
shared_ptr<ParameterLink<int>> DETextureBrain::inBitsPerPixelPL = Parameters::register_parameter("BRAIN_DETEXTURE-inBitsPerPixel", 1,
                                                                                                 "number of binary channels in each input pixel (default 1)");
shared_ptr<ParameterLink<int>> DETextureBrain::outBitsPerPixelPL = Parameters::register_parameter("BRAIN_DETEXTURE-outBitsPerPixel", 3,
                                                                                                  "number of binary channels in each output pixel (default 3)");
shared_ptr<ParameterLink<int>> DETextureBrain::convolutionRegimePL = Parameters::register_parameter("BRAIN_DETEXTURE-convolutionRegime", 0,
                                                                                                    "integer specifying how the brain should treat the spatial structure of the texture: 0 - local filters, 1 - local filters with shared logic; to get a fully connected behavior, set filter dimensions to be equial to the input size");

/********** Public definitions **********/

DETextureBrain::DETextureBrain(int _nrInNodes, int _nrOutNodes, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(_nrInNodes, _nrOutNodes, PT_),
	convolutionRegime(convolutionRegimePL->get(PT_)),
	minGates(minGateCountPL->get(PT_)),
	visualize(Global::modePL->get() == "visualize"),
	originationStory("primordial"),
	structurewideMutationProbability(structurewideMutationProbabilityPL->get(PT_)) {

	if(_nrInNodes!=0) throw invalid_argument("Construction of DETextureBrain with number of input nodes other than 0 attempted");
	if(_nrOutNodes!=0) throw invalid_argument("Construction of DETextureBrain with number of output nodes other than 0 attempted");
	if(convolutionRegime!=UNSHARED_REGIME && convolutionRegime!=SHARED_REGIME) throw invalid_argument(string("DETextureBrain does not support convolution regime ") + to_string(convolutionRegime));

	tie(inputSizeX, inputSizeY, inputSizeT) = parseShapeStr_texture(inTextureShapePL->get(PT_));
	tie(filterSizeX, filterSizeY, filterSizeT) = parseShapeStr_texture(filterShapePL->get(PT_));
	tie(strideX, strideY, strideT) = parseShapeStr_texture(strideShapePL->get(PT_));
	inBitsPerPixel = inBitsPerPixelPL->get(PT_);
	validateDimensions();

	tie(outputSizeX, outputSizeY, outputSizeT) = computeOutputShape();
	outBitsPerPixel = outBitsPerPixelPL->get(PT_);

	filters.resize(boost::extents[outputSizeX][outputSizeY][outputSizeT]);

	output = new Texture(boost::extents[outputSizeX][outputSizeY][outputSizeT][outBitsPerPixel]);
	resetOutputTexture();

	tie(gateMinIns, gateMaxIns, gateMinOuts, gateMaxOuts) = parseGateLimitsStr_texture(gateIORangesPL->get(PT_));

	// columns to be added to ave file
	popFileColumns.clear();
	popFileColumns.push_back("markovBrainGates");
	popFileColumns.push_back("markovBrainDeterministicTextureGates");
	popFileColumns.push_back("markovBrainProbabilisticTextureGates");
}

DETextureBrain::~DETextureBrain() {
	delete output;
}

void DETextureBrain::update() {
	validateInput();

//	cout << "Brain at " << this << " is updating with input at " << input << " and output at " << output << endl;
//	cout << readableRepr(*input) << endl;

	for(size_t fx=0; fx<outputSizeX; fx++)
		for(size_t fy=0; fy<outputSizeY; fy++)
			for(size_t ft=0; ft<outputSizeT; ft++)
				for(auto gate : filters[fx][fy][ft])
					gate->update();

//	cout << readableRepr(*output) << endl;

//	if(visualize)
//		log.logStateAfterUpdate(nodes);
}

shared_ptr<AbstractBrain> DETextureBrain::makeCopy(shared_ptr<ParametersTable> PT_) {
	if(PT_==nullptr) throw invalid_argument("DETextureBrain::makeCopy caught a nullptr");
	if(PT_!=PT) throw invalid_argument("DETextureBrain::makeCopy was called with a parameters table that is different from the one the original used. Are you sure you want to do that?");

	auto newBrain = make_shared<DETextureBrain>(nrInputValues, nrOutputValues, PT);
	for(size_t fx=0; fx<outputSizeX; fx++)
		for(size_t fy=0; fy<outputSizeY; fy++)
			for(size_t ft=0; ft<outputSizeT; ft++)
				for(auto gate : filters[fx][fy][ft]) {
					auto gateCopyPtr = gate->makeCopy(gate->ID);
					gateCopyPtr->updateOutputs(newBrain->output);
					newBrain->filters[fx][fy][ft].push_back(gateCopyPtr);
				}
	return newBrain;
}

shared_ptr<AbstractBrain> DETextureBrain::makeBrain(unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	return make_shared<DETextureBrain>(nrInputValues, nrOutputValues, PT, true);
}

shared_ptr<AbstractBrain> DETextureBrain::makeBrainFromMany(vector<shared_ptr<AbstractBrain>> _brains,
                                                            unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	return makeCopy(PT); // note that mutate() WILL be called upon it right away
}

void DETextureBrain::mutate() {
//	std::cout << "mutate() called on brain " << this << std::endl;
	mainMutate();
	if(structurewideMutationProbability!=0)
		mutateStructurewide();
}

void DETextureBrain::resetBrain() {
	AbstractBrain::resetBrain(); // probably safe to comment out: all it does is resetting the zero-length IO vector<double>s
	resetOutputTexture();
	for(size_t fx=0; fx<outputSizeX; fx++)
		for(size_t fy=0; fy<outputSizeY; fy++)
			for(size_t ft=0; ft<outputSizeT; ft++)
				for(auto gate : filters[fx][fy][ft])
					gate->reset();
//	if(visualize) log.logBrainReset();
}

void DETextureBrain::attachToSensors(void* inputPtr) {
//	cout << "Attaching brain " << this << " to sensors using data pointer " << inputPtr << endl;
	input = reinterpret_cast<Texture*>(inputPtr);
	for(size_t fx=0; fx<outputSizeX; fx++)
		for(size_t fy=0; fy<outputSizeY; fy++)
			for(size_t ft=0; ft<outputSizeT; ft++)
				for(auto gate : filters[fx][fy][ft])
					gate->updateInputs(input);
}

string DETextureBrain::description() {
	stringstream ss;
	ss << "DETextureBrain with " << totalNumberOfGates() << " gates in " << totalNumberOfFilters() << " filters, input at " << input << " and output at " << output << endl
	   << "input dimensions  : x=" << inputSizeX << " y=" << inputSizeY << " t=" << inputSizeT << " depth=" << inBitsPerPixel << "bit" << endl
	   << "filter dimensions : x=" << filterSizeX << " y=" << filterSizeY << " t=" << filterSizeT << endl
	   << "stride            : x=" << strideX << " y=" << strideY << " t=" << strideT << endl
	   << "output dimensions : x=" << outputSizeX << " y=" << outputSizeY << " t=" << outputSizeT << " depth=" << outBitsPerPixel << "bit" << endl
	   << "descriptions of gates:" << endl;
	for(size_t fx=0; fx<outputSizeX; fx++)
		for(size_t fy=0; fy<outputSizeY; fy++)
			for(size_t ft=0; ft<outputSizeT; ft++) {
				ss << "Gates at filter (" << fx << ", " << fy << ", " << ft << "):" << endl;
				for(auto gate : filters[fx][fy][ft])
					ss << gate->description();
			}
	return ss.str();
}

DataMap DETextureBrain::getStats(string& prefix) {
	DataMap dataMap;
	dataMap.set(prefix + "markovBrainGates", static_cast<int>(totalNumberOfGates()));

	int unsharedGates = 0;
	if(convolutionRegime==UNSHARED_REGIME)
		unsharedGates = totalNumberOfGates();
	if(convolutionRegime==SHARED_REGIME)
		unsharedGates = filters[0][0][0].size();
	dataMap.set(prefix + "independentMarkovBrainGates", unsharedGates);

	// TODO: comment out after debugging, this is a useless part of output that creates considerable noise
	map<string,int> gatecounts;
	gatecounts["DeterministicTextureGates"] = 0;
	gatecounts["ProbabilisticTextureGates"] = 0;
	if(convolutionRegime==UNSHARED_REGIME) {
		for(size_t fx=0; fx<outputSizeX; fx++)
			for(size_t fy=0; fy<outputSizeY; fy++)
				for(size_t ft=0; ft<outputSizeT; ft++)
					for(auto gate : filters[fx][fy][ft]) {
						if(gate->gateType()=="DeterministicTexture")
							gatecounts["DeterministicTextureGates"]++;
						if(gate->gateType()=="ProbabilisticTexture")
							gatecounts["ProbabilisticTextureGates"]++;
					}
	}
	if(convolutionRegime==SHARED_REGIME) {
		for(auto gate : filters[0][0][0]) {
			if(gate->gateType()=="DeterministicTexture")
				gatecounts["DeterministicTextureGates"]++;
			if(gate->gateType()=="ProbabilisticTexture")
				gatecounts["ProbabilisticTextureGates"]++;
		}
		gatecounts["DeterministicTextureGates"] *= totalNumberOfFilters();
		gatecounts["ProbabilisticTextureGates"] *= totalNumebrOfFilters();
	}
	dataMap.set(prefix + "markovBrainDeterministicTextureGates", gatecounts["DeterministicTextureGates"]);
	dataMap.set(prefix + "markovBrainProbabilisticTextureGates", gatecounts["ProbabilisticTextureGates"]);

	dataMap.set(prefix + "originationStory", originationStory);
	return dataMap;
}

DataMap DETextureBrain::serialize(string& name) {
	using namespace nlohmann;
	json brainJSON = json::object();

	if(convolutionRegime==UNSHARED_REGIME)
		brainJSON["convolutionRegime"] = "unshared";
	if(convolutionRegime==SHARED_REGIME)
		brainJSON["convolutionRegime"] = "shared";

	json filtersJSON = json::array();
	if(convolutionRegime==UNSHARED_REGIME) {
		for(size_t fx=0; fx<outputSizeX; fx++) {
			json xSliceJSON = json::array();
			for(size_t fy=0; fy<outputSizeY; fy++) {
				json xySliceJSON = json::array();
				for(size_t ft=0; ft<outputSizeT; ft++) {
					json singleFilterJSON = json::array();
					for(auto gate : filters[fx][fy][ft])
						singleFilterJSON.push_back(gate->serialize());
					xySliceJSON.push_back(singleFilterJSON);
				}
				xSliceJSON.push_back(xySliceJSON);
			}
			filtersJSON.push_back(xSliceJSON);
		}
	}
	if(convolutionRegime==SHARED_REGIME) {
		for(auto gate : filters[0][0][0])
			filtersJSON.push_back(gate->serialize());
	}
	brainJSON["filters"] = filtersJSON;

	brainJSON["inputShape"] = json::array({inputSizeX, inputSizeY, inputSizeT, inBitsPerPixel});
	brainJSON["filterShape"] =json::array({filterSizeX, filterSizeY, filterSizeT});
	brainJSON["stride"] = json::array({strideX, strideY, strideT});
	brainJSON["outputShape"] = json::array({outputSizeX, outputSizeY, outputSizeT, outBitsPerPixel});
	brainJSON["originationStory"] = originationStory;

	// storing the string at the DataMap and returning it
	DataMap dm;
	dm.set(name + "_json", string("'") + brainJSON.dump() + "'");
	return dm;
}

void DETextureBrain::deserialize(shared_ptr<ParametersTable> PT, unordered_map<string,string>& orgData, string& name) {
	// the stuff that can be loaded from config files is already loaded during construction,
	// so all that's left is to obsessively check it, initialize gates
	// AND restore the saved origination story

	using namespace nlohmann;

	string brainJSONStr = orgData[string("BRAIN_") + name + "_json"];
	if(brainJSONStr.front()!='\'' || brainJSONStr.back()!='\'')
		throw invalid_argument(string("First or last character of the DETextureBrain JSON string field in the log is not a single quote.\nThe string: ") + brainJSONStr);
	brainJSONStr = brainJSONStr.substr(1, brainJSONStr.size()-2);

	json brainJSON = json::parse(brainJSONStr);

	if(brainJSON["convolutionRegime"]=="unshared" && convolutionRegime!=UNSHARED_REGIME) throw invalid_argument("Deserialization of unshared-convolutional DETextureBrains as something else is risky business for which you'll have to change source anyway so come comment me out");
	if(brainJSON["convolutionRegime"]=="shared" && convolutionRegime!=SHARED_REGIME) throw invalid_argument("Deserialization of shared-convolutional DETextureBrains as something else is risky business for which you'll have to change source anyway so come comment me out");

	const map<string,vector<size_t>> dimsToCheck = { {"inputShape", {inputSizeX, inputSizeY, inputSizeT, inBitsPerPixel}},
	                                                 {"filterShape", {filterSizeX, filterSizeY, filterSizeT}},
	                                                 {"stride", {strideX, strideY, strideT}},
	                                                 {"outputShape", {outputSizeX, outputSizeY, outputSizeT, outBitsPerPixel}} };
	vector<string> dimNames = {"X-dimension", "Y-dimension", "T-dimension", "depth"};

	for(const auto& dimPair : dimsToCheck) {
		string field = dimPair.first;
		size_t i = 0;
		for(const auto& val : dimPair.second) {
			if(brainJSON[field][i]!=val)
				throw invalid_argument(string("DETextureBrain.deserialize has detected mismatch of stored and preloaded ") + dimNames[i] +
				                       " of " + field + ": " + to_string(static_cast<size_t>(brainJSON[field][i])) + " vs " + to_string(val));
			i++;
		}
	}

	originationStory = brainJSON["originationStory"];

	json filtersJSON = brainJSON["filters"];
	auto deserializeGate = [](json gateJSON) {
		shared_ptr<AbstractTextureGate> gate = nullptr;
		if(gateJSON["type"] == "DeterministicTexture")
			gate = make_shared<DeterministicTextureGate>();
		else if(gateJSON["type"] == "ProbabilisticTexture")
			throw invalid_argument("ProbabilisticTextureGate cannot be deserialized since it is not implemented yet"); // gate = make_shared<ProbabilisticTextureGate>();
		else
			throw invalid_argument(string("DETextureBrain::deserialize: Unsupported gate type ") + static_cast<string>(gateJSON["type"]));
		gate->deserialize(gateJSON);
		return gate;
	};
	for(size_t fx=0; fx<outputSizeX; fx++)
		for(size_t fy=0; fy<outputSizeY; fy++)
			for(size_t ft=0; ft<outputSizeT; ft++) {
				filters[fx][fy][ft].clear();
				if(convolutionRegime==UNSHARED_REGIME) {
					for(auto gateJSON : filtersJSON[fx][fy][ft]) {
						auto gate = deserializeGate(gateJSON);
						gate->updateOutputs(output);
						filters[fx][fy][ft].push_back(gate);
					}
				}
				if(convolutionRegime==SHARED_REGIME) {
					for(auto baseGateJSON : filtersJSON) {
						auto gate = deserializeGate(gateJSON);
						gate->setInputsShift({{fx*strideX, fy*strideY, ft*strideT, 0}});
						gate->setOutputsShift({{fx, fy, ft, 0}});
						gate->updateOutputs(output);
						filters[fx][fy][ft].push_back(gate);
					}
				}
			}
	}
}

/********** Private definitions **********/

tuple<size_t,size_t,size_t> DETextureBrain::computeOutputShape() {
	size_t outx = (inputSizeX-filterSizeX) / strideX + 1;
	size_t outy = (inputSizeY-filterSizeY) / strideY + 1;
	size_t outt = (inputSizeT-filterSizeT) / strideT + 1;
	return make_tuple(outx, outy, outt);
}

unsigned DETextureBrain::totalNumberOfGates() {
	if(convolutionRegime==UNSHARED_REGIME) {
		unsigned numGates = 0;
		for(size_t fx=0; fx<outputSizeX; fx++)
			for(size_t fy=0; fy<outputSizeY; fy++)
				for(size_t ft=0; ft<outputSizeT; ft++)
					numGates += filters[fx][fy][ft].size();
		return numGates;
	}
	if(convolutionRegime==SHARED_REGIME)
		return totalNumberOfFilters()*filters[0][0][0].size();
}

void DETextureBrain::resetOutputTexture() {
	fill_n(output->data(), output->num_elements(), 0);
}

void DETextureBrain::mutateStructurewide() {
	if(convolutionRegime==UNSHARED_REGIME) {
		for(size_t fx=0; fx<outputSizeX; fx++)
			for(size_t fy=0; fy<outputSizeY; fy++)
				for(size_t ft=0; ft<outputSizeT; ft++)
					for(auto gate : filters[fx][fy][ft]) {
						if(Random::getDouble(1.)<structurewideMutationProbability)
							gate->mutateInternalStructure();
						if(Random::getDouble(1.)<structurewideMutationProbability) {
							size_t gateInputSize = gate->inputsFilterIndices.size();
							size_t gateOutputSize = gate->outputsFilterIndices.size();
							size_t connectionIdx = Random::getIndex(gateInputSize+gateOutputSize);
							if(connectionIdx<gateInputSize)
								gate->inputsFilterIndices[connectionIdx] = getRandomFilterInputIndex();
							else {
								connectionIdx -= gateInputSize;
								gate->outputsFilterIndices[connectionIdx] = getRandomFilterOutputIndex();
								gate->updateOutputs(output);
							}
						}
					}
	}
	if(convolutionRegime==SHARED_REGIME) {
		for(size_t gi=0; gi<filters[0][0][0].size(); gi++) {
			if(Random::getDouble(1.)<structurewideMutationProbability) {
				filters[0][0][0][gi]->mutateInternalStructure();
				for(size_t fx=0; fx<outputSizeX; fx++)
					for(size_t fy=0; fy<outputSizeY; fy++)
						for(size_t ft=0; ft<outputSizeT; ft++)
							filters[fx][fy][ft][gi] = filters[0][0][0][gi]->makeCopy(filters[0][0][0][gi]->ID);
			}
			if(Random::getDouble(1.)<structurewideMutationProbability) {
				shared_ptr<AbstractTextureGate> theGate = filters[0][0][0][gi];
				size_t gateInputSize = theGate->inputsFilterIndices.size();
				size_t gateOutputSize = theGate->outputsFilterIndices.size();
				size_t connectionIdx = Random::getIndex(gateInputSize+gateOutputSize);
				if(connectionIdx<gateInputSize) {
					TextureIndex newInputAddress = getRandomFilterInputIndex();
					for(size_t fx=0; fx<outputSizeX; fx++)
						for(size_t fy=0; fy<outputSizeY; fy++)
							for(size_t ft=0; ft<outputSizeT; ft++)
								filter[fx][fy][ft][gi]->inputsFilterIndices[connectionIdx] = newInputAddress;
				}
				else {
					connectionIdx -= gateInputSize;
					TextureIndex newOutputAddress = getRandomFilterOutputIndex();
					for(size_t fx=0; fx<outputSizeX; fx++)
						for(size_t fy=0; fy<outputSizeY; fy++)
							for(size_t ft=0; ft<outputSizeT; ft++) {
								filter[fx][fy][ft][gi]->outputsFilterIndices[connectionIdx] = newOutputAddress;
								filter[fx][fy][ft][gi]->updateOutputs(output);
							}
				}
			}
		}
	}
}

void DETextureBrain::mainMutate() {
	double r = Random::getDouble(1.);
//	cout << "DETextureBrain " << this << " is mutating. Rolled " << r;
	if(r < gateInsertionProbabilityPL->get(PT)) {
//		cout << ", chose insertion. Had " << gates.size() << " gates, ";
		addRandomGate();
//		cout << " now it's " << gates.size() << endl;
		originationStory = "mutation_insertion";
	}
	else {
		if(totalNumberOfGates()==0) {
			if(gateInsertionProbabilityPL->get(PT) > 0)
				mainMutate();
			else
				throw logic_error("DETextureBrain: mutation that cannot insert gates was called on a brain with no gates");
			return;
		}

		if(r < gateInsertionProbabilityPL->get(PT)+gateDeletionProbabilityPL->get(PT)) {
			if(totalNumberOfGates()<=minGates) {
				mainMutate();
				return;
			}
//			cout << ", chose deletion. Had " << gates.size() << " gates, ";
			deleteRandomlySelectedGate();
//			cout << ", removed " << idx << "th one, now it's " << gates.size() << endl;
			originationStory = "mutation_deletion";
		}
		else if(r < gateInsertionProbabilityPL->get(PT)+gateDeletionProbabilityPL->get(PT)+gateDuplicationProbabilityPL->get(PT)) {
//			cout << ", chose duplication. Had " << gates.size() << " gates, ";
			addCopyOfRandomlySelectedGate();
//			cout << ", duplicated " << idx << "th one, now it's " << gates.size() << endl;
			originationStory = "mutation_duplication";
		}
		else {
//			cout << ", chose intra-gate mutation. Chose gate " << idx;
			double spentProb = gateInsertionProbabilityPL->get(PT)+gateDeletionProbabilityPL->get(PT)+gateDuplicationProbabilityPL->get(PT);
			double tableChangeThr = spentProb + (1.-spentProb)/(1.+connectionToTableChangeRatioPL->get(PT));
			if(r < tableChangeThr) {
//				cout << " and a table mutation. Gate before the mutation:" << endl << gates[idx]->description() << endl;
				internallyMutateRandomlySelectedGate();
//				cout << "Gate after the mutation:" << endl << gates[idx]->description() << endl;
				originationStory = "mutation_table";
			}
			else {
//				cout << " and a wiring mutation. Gate before the mutation:" << endl << gates[idx]->description();
				randomlyRewireRandomlySelectedGate();
//				cout << "Gate after the mutation:" << endl << gates[idx]->description() << endl;
				originationStory = "mutation_rewiring";
			}
		}
	}
}

void DETextureBrain::randomize() {
	for(size_t fx=0; fx<outputSizeX; fx++)
		for(size_t fy=0; fy<outputSizeY; fy++)
			for(size_t ft=0; ft<outputSizeT; ft++)
				filters[fx][fy][ft].clear();

	for(unsigned g=0; g<initialGateCountPL->get(PT); g++)
		addRandomGate();
}

void DETextureBrain::internallyMutateRandomlySelectedGate() {
	if(convolutionRegime==UNSHARED_REGIME) {
		size_t rfx, rfy, rft, gidx;
		tie(rfx, rfy, rft) = getRandomFilterIndex();
		gidx = Random::getIndex(filters[rfx][rfy][rft].size());
		filters[rfx][rfy][rft][gidx]->mutateInternalStructure();
	}
	if(convolutionRegime==SHARED_REGIME) {
		size_t gidx = Random::getIndex(filters[0][0][0].size());
		shared_ptr<AbstractTextureGate> theGate = filters[0][0][0][gidx];
		theGate->mutateInternalStructure();
		for(size_t fx=0; fx<outputSizeX; fx++)
			for(size_t fy=0; fy<outputSizeY; fy++)
				for(size_t ft=0; ft<outputSizeT; ft++)
					filters[fx][fy][ft][gidx] = theGate->makeCopy(theGate->ID);
	}
}

void DETextureBrain::randomlyRewireRandomlySelectedGate() {
	if(convolutionRegime==UNSHARED_REGIME) {
		size_t rfx, rfy, rft, gidx;
		tie(rfx, rfy, rft) = getRandomFilterIndex();
		gidx = Random::getIndex(filters[rfx][rfy][rft].size());
		shared_ptr<AbstractTextureGate> theGate = filters[rfx][rfy][rft][gidx];

		size_t gateInputSize = theGate->inputsFilterIndices.size();
		size_t gateOutputSize = theGate->outputsFilterIndices.size();
		size_t connectionIdx = Random::getIndex(gateInputSize+gateOutputSize);
		if(connectionIdx<gateInputSize)
			theGate->inputsFilterIndices[connectionIdx] = getRandomFilterInputIndex();
		else {
			connectionIdx -= gateInputSize;
			theGate->outputsFilterIndices[connectionIdx] = getRandomFilterOutputIndex();
			theGate->updateOutputs(output);
		}
	}
	if(convolutionRegime==SHARED_REGIME) {
		size_t gidx = Random::getIndex(filters[0][0][0].size());
		shared_ptr<AbstractTextureGate> theGate = filters[0][0][0][gidx];
		size_t gateInputSize = theGate->inputsFilterIndices.size();
		size_t gateOutputSize = theGate->outputsFilterIndices.size();
		size_t connectionIdx = Random::getIndex(gateInputSize+gateOutputSize);
		if(connectionIdx<gateInputSize) {
			TextureIndex newInputAddress = getRandomFilterInputIndex();
			for(size_t fx=0; fx<outputSizeX; fx++)
				for(size_t fy=0; fy<outputSizeY; fy++)
					for(size_t ft=0; ft<outputSizeT; ft++)
						filter[fx][fy][ft][gidx]->inputsFilterIndices[connectionIdx] = newInputAddress;
		}
		else {
			connectionIdx -= gateInputSize;
			TextureIndex newOutputAddress = getRandomFilterOutputIndex();
			for(size_t fx=0; fx<outputSizeX; fx++)
				for(size_t fy=0; fy<outputSizeY; fy++)
					for(size_t ft=0; ft<outputSizeT; ft++) {
						filter[fx][fy][ft][gidx]->outputsFilterIndices[connectionIdx] = newOutputAddress;
						filter[fx][fy][ft][gidx]->updateOutputs(output);
					}
		}
	}
}

void DETextureBrain::addCopyOfRandomlySelectedGate() {
	if(convolutionRegime==UNSHARED_REGIME) {
		// can be non-neutral
		size_t rsfx, rsfy, rsft, rdfx, rdfy, rdft; // random <source|destination> filter <dimension>
		tie(rsfx, rsfy, rsft) = getRandomFilterIndex();
		tie(rdfx, rdfy, rdft) = getRandomFilterIndex();
		size_t sidx = Random::getIndex(filters[rsfx][rsfy][rsft].size()); // index within the source filter
		shared_ptr<AbstractTextureGate> newGate = filters[rsfx][rsfy][rsft][sidx]->makeCopy(getLowestAvailableGateID());
		newGate->setInputsShift({{rdfx*strideX, rdfy*strideY, rdft*strideT, 0}});
		newGate->setOutputsShift({{rdfx, rdfy, rdft, 0}});
		newGate->updateOutputs(output);
		filters[rdfx][rdfy][rdft].push_back(newGate);
	}
	if(convolutionRegime==SHARED_REGIME) {
		// always neutral
		size_t sidx = Random::getIndex(filters[0][0][0].size());
		unsigned newGateID = getLowestAvailableGateID();
		for(size_t fx=0; fx<outputSizeX; fx++)
			for(size_t fy=0; fy<outputSizeY; fy++)
				for(size_t ft=0; ft<outputSizeT; ft++) {
					auto newGate = filters[0][0][0][sidx]->makeCopy(newGateID);
					newGate->setInputsShift({{fx*strideX, fy*strideY, ft*strideT, 0}});
					newGate->setOutputShift({{fx, fy, ft, 0}});
					newGate->updateOutputs(output);
					filters[fx][fy][ft].push_back(newGate);
				}
	}
}

void DETextureBrain::addRandomGate() {
	const unsigned nins = Random::getInt(gateMinIns, gateMaxIns);
	const unsigned nouts = Random::getInt(gateMinOuts, gateMaxOuts);
	vector<TextureIndex> inputIdxs;
	vector<TextureIndex> outputIdxs;
	for(unsigned i=0; i<nins; i++)
		inputIdxs.push_back(getRandomFilterInputIndex());
	for(unsigned j=0; j<nouts; j++)
		outputIdxs.push_back(getRandomFilterOutputIndex());

	shared_ptr<AbstractTextureGate> newGate = make_shared<DeterministicTextureGate>(getLowestAvailableGateID(), inputIdxs, outputIdxs);
	if(convolutionRegime==UNSHARED_REGIME) {
		size_t rfx, rfy, rft; // random filter <dimension>
		tie(rfx, rfy, rft) = getRandomFilterIndex();
		newGate->setInputsShift({{rfx*strideX, rfy*strideY, rft*strideT, 0}});
		newGate->setOutputsShift({{rfx, rfy, rft, 0}});
		newGate->updateOutputs(output);
		filters[rfx][rfy][rft].push_back(newGate);
	}
	if(convolutionRegime==SHARED_REGIME) {
		for(size_t fx=0; fx<outputSizeX; fx++)
			for(size_t fy=0; fy<outputSizeY; fy++)
				for(size_t ft=0; ft<outputSizeT; ft++) {
					shared_ptr<AbstractTextureGate> newLocalGate = newGate->makeCopy(newGate->ID);
					newLocalGate->setInputsShift({{fx*strideX, fy*strideY, ft*strideT, 0}});
					newLocalGate->setOutputsShift({{rfx, rfy, rft, 0}});
					newLocalGate->updateOutputs(output);
					filters[fx][fy][ft].push_back(newLocalGate);
				}
	}
}

void DETextureBrain::deleteRandomlySelectedGate() {
	if(convolutionRegime==UNSHARED_REGIME) {
		size_t rfx, rfy, rft;
		tie(rfx, rfy, rft) = getRandomFilterIndex();
		size_t rgidx = Random::getIndex(filters[rfx][rfy][rft].size());
		filters[rfx][rfy][rft].erase(filters[rfx][rfy][rft].begin() + rgidx);
	}
	if(convolutionalRegime==SHARED_REGIME) {
		size_t rgidx = Random::getIndex(filters[0][0][0].size());
		for(size_t fx=0; fx<outputSizeX; fx++)
			for(size_t fy=0; fy<outputSizeY; fy++)
				for(size_t ft=0; ft<outputSizeT; ft++)
					filters[fx][fy][ft].erase(filters[fx][fy][ft].begin() + rgidx);
	}
}

tuple<size_t,size_t,size_t> DETextureBrain::getRandomFilterIndex() {
	return tuple<size_t,size_t,size_t>(Random::getIndex(outputSizeX),
	                                   Random::getIndex(outputSizeY),
	                                   Random::getIndex(outputSizeT));
}

TextureIndex DETextureBrain::getRandomFilterInputIndex() {
	return {{ Random::getIndex(filterSizeX),
	          Random::getIndex(filterSizeY),
	          Random::getIndex(filterSizeT),
	          Random::getIndex(inBitsPerPixel) }};
}

TextureIndex DETextureBrain::getRandomFilterOutputIndex() {
	return {{ 0,
	          0,
	          0,
	          Random::getIndex(outBitsPerPixel) }};
}

unsigned DETextureBrain::getLowestAvailableGateID() {
	deque<bool> idAvailable;

	if(convolutionRegime==UNSHARED_REGIME) {
		idAvailable.resize(totalNumberOfGates(), false);
		for(size_t fx=0; fx<outputSizeX; fx++)
			for(size_t fy=0; fy<outputSizeY; fy++)
				for(size_t ft=0; ft<outputSizeT; ft++)
					for(auto gate : filters[fx][fy][ft])
						idAvailable[gate->ID] = true;
	}
	if(convolutionRegime==SHARED_REGIME) {
		size_t numSharedGates = filters[0][0][0].size();
		idAvailable.resize(numSharedGates, false);
		for(auto gate : filters[0][0][0])
			idAvailable[gate->ID] = true;
	}

	unsigned newGateID;
	for(newGateID=0; newGateID<idAvailable.size(); newGateID++)
		if(!idAvailable[newGateID])
			return newGateID;
	return newGateID;
}

/*
void DETextureBrain::beginLogging() {
	if(visualize) {
		log.open(Global::outputPrefixPL->get() + "markov_log");
		logBrainStructure();
		log.log("\nActivity log:\n");
	}
}

void DETextureBrain::logBrainStructure() {
	log.log("begin brain desription\n" + description() + "end brain description\n");
}
*/

void DETextureBrain::validateDimensions() {
	stringstream errorss;

	if(filterSizeX > inputSizeX) errorss << endl << "filterSizeX > inputSizeX: " << filterSizeX << " > " << inputSizeX;
	if(filterSizeY > inputSizeY) errorss << endl << "filterSizeY > inputSizeY: " << filterSizeY << " > " << inputSizeY;
	if(filterSizeT > inputSizeT) errorss << endl << "filterSizeT > inputSizeT: " << filterSizeT << " > " << inputSizeT;

	if(strideX > filterSizeX) errorss << endl << "strideX > filterSizeX: " << strideX << " > " << filterSizeX;
	if(strideY > filterSizeY) errorss << endl << "strideY > filterSizeY: " << strideY << " > " << filterSizeY;
	if(strideT > filterSizeT) errorss << endl << "strideT > filterSizeT: " << strideT << " > " << filterSizeT;

	if((inputSizeX-filterSizeX)%strideX != 0) errorss << endl << "X axis tiling failed: inputSizeX=" << inputSizeX << " filterSizeX=" << filterSizeX << " strideX=" << strideX;
	if((inputSizeY-filterSizeY)%strideY != 0) errorss << endl << "Y axis tiling failed: inputSizeY=" << inputSizeX << " filterSizeY=" << filterSizeX << " strideY=" << strideX;
	if((inputSizeT-filterSizeT)%strideT != 0) errorss << endl << "T axis tiling failed: inputSizeT=" << inputSizeX << " filterSizeT=" << filterSizeX << " strideT=" << strideX;

	if(!errorss.str().empty()) throw invalid_argument(string("Error(s) in DETextureBrain dimensions:") + errorss.str());
}

void DETextureBrain::validateInput() {
	size_t receivedSizeX = input->shape()[0];
	size_t receivedSizeY = input->shape()[1];
	size_t receivedSizeT = input->shape()[2];
	size_t receivedBitDepth = input->shape()[3];

	stringstream errorss;

	if(receivedSizeX != inputSizeX) errorss << endl << "receivedSizeX != inputSizeX: " << receivedSizeX << " != " << inputSizeX;
	if(receivedSizeY != inputSizeY) errorss << endl << "receivedSizeY != inputSizeY: " << receivedSizeY << " != " << inputSizeY;
	if(receivedSizeT != inputSizeT) errorss << endl << "receivedSizeT != inputSizeT: " << receivedSizeT << " != " << inputSizeT;
	if(receivedBitDepth != inBitsPerPixel) errorss << endl << "receivedBitDepth != inBitsPerPixel: " << receivedBitDepth << " != " << inBitsPerPixel;

	if(!errorss.str().empty()) throw invalid_argument(string("DETextureBrain dimensions not aligned with received input texture:") + errorss.str());

}
