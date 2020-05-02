#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <algorithm>

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
shared_ptr<ParameterLink<int>> DETextureBrain::initialGateCountPL = Parameters::register_parameter("BRAIN_DETEXTURE-initialGateCount", 30,
                                                                                                   "number of gates to add to the newly generated individuals");
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

/********** Public definitions **********/

DETextureBrain::DETextureBrain(int _nrInNodes, int _nrOutNodes, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(_nrInNodes, _nrOutNodes, PT_),
	minGates(minGateCountPL->get(PT_)),
	visualize(Global::modePL->get() == "visualize"),
	originationStory("primordial"),
	structurewideMutationProbability(structurewideMutationProbabilityPL->get(PT_)) {

	if(_nrInNodes!=0) throw invalid_argument("Construction of DETextureBrain with number of input nodes other than 0 attempted");
	if(_nrOutNodes!=0) throw invalid_argument("Construction of DETextureBrain with number of output nodes other than 0 attempted");

	tie(inputSizeX, inputSizeY, inputSizeT) = parseShapeStr_texture(inTextureShapePL->get(PT_));
	tie(filterSizeX, filterSizeY, filterSizeT) = parseShapeStr_texture(filterShapePL->get(PT_));
	tie(strideX, strideY, strideT) = parseShapeStr_texture(strideShapePL->get(PT_));
	inBitsPerPixel = inBitsPerPixelPL->get(PT_);
	validateDimensions();

	tie(outputSizeX, outputSizeY, outputSizeT) = computeOutputShape();
	outBitsPerPixel = outBitsPerPixelPL->get(PT_);

	tie(gateMinIns, gateMaxIns, gateMinOuts, gateMaxOuts) = parseGateLimitsStr_texture(gateIORangesPL->get(PT_));

//	cout << description() << endl;
//	exit(EXIT_FAILURE);

	output = new Texture(boost::extents[outputSizeX][outputSizeY][outputSizeT][outBitsPerPixel]);
	resetOutputTexture();

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
//	cout << readableTextureRepr(*input) << endl;

	for(auto& g : gates)
		g->update();

//	cout << readableTextureRepr(*output) << endl;

//	if(visualize)
//		log.logStateAfterUpdate(nodes);
}

shared_ptr<AbstractBrain> DETextureBrain::makeCopy(shared_ptr<ParametersTable> PT_) {
	if(PT_==nullptr) throw invalid_argument("DETextureBrain::makeCopy caught a nullptr");
	if(PT_!=PT) throw invalid_argument("DETextureBrain::makeCopy was called with a parameters table that is different from the one the original used. Are you sure you want to do that?");

	auto newBrain = make_shared<DETextureBrain>(nrInputValues, nrOutputValues, PT);
	for(auto gate : gates) {
		newBrain->gates.push_back(gate->makeCopy(gate->ID));
		newBrain->gates.back()->updateOutputs(newBrain->output);
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
	for(auto& g :gates) {
		g->reset();
		g->updateOutputs(output);
	}
//	if(visualize) log.logBrainReset();
}

void DETextureBrain::attachToSensors(void* inputPtr) {
//	cout << "Attaching brain " << this << " to sensors using data pointer " << inputPtr << endl;
	input = reinterpret_cast<Texture*>(inputPtr);
	for(auto& g : gates)
		g->updateInputs(input);
}

string DETextureBrain::description() {
	stringstream ss;
	ss << "DETextureBrain with " << gates.size() << " gates, input at " << input << " and output at " << output << endl
	   << "input dimensions  : x=" << inputSizeX << " y=" << inputSizeY << " t=" << inputSizeT << " depth=" << inBitsPerPixel << "bit" << endl
	   << "filter dimensions : x=" << filterSizeX << " y=" << filterSizeY << " t=" << filterSizeT << endl
	   << "stride            : x=" << strideX << " y=" << strideY << " t=" << strideT << endl
	   << "output dimensions : x=" << outputSizeX << " y=" << outputSizeY << " t=" << outputSizeT << " depth=" << outBitsPerPixel << "bit" << endl
	   << "descriptions of gates:" << endl;
	for(auto& g : gates)
		ss << g->description();
	return ss.str();
}

DataMap DETextureBrain::getStats(string& prefix) {
	DataMap dataMap;
	dataMap.set(prefix + "markovBrainGates", (int) gates.size());
	map<string,int> gatecounts;
	gatecounts["DeterministicTextureGates"] = 0;
	gatecounts["ProbabilisticTextureGates"] = 0;
	for (auto &g : gates) {
		if(g->gateType()=="DeterministicTexture")
			gatecounts["DeterministicTextureGates"]++;
		else if(g->gateType()=="ProbabilisticTexture")
			gatecounts["ProbabilisticTextureGates"]++;
		else
			throw logic_error("DETextureBrain::getStats: unexpected gate type " + g->gateType());
	}
	dataMap.set(prefix + "markovBrainDeterministicTextureGates", gatecounts["DeterministicTextureGates"]);
	dataMap.set(prefix + "markovBrainProbabilisticTextureGates", gatecounts["ProbabilisticTextureGates"]);
	dataMap.set(prefix + "originationStory", originationStory);
	return dataMap;
}

DataMap DETextureBrain::serialize(string& name) {
	nlohmann::json brainJSON = nlohmann::json::object();

	nlohmann::json gatesJSON = nlohmann::json::array();
	for(const auto& gate : gates)
		gatesJSON.push_back(gate->serialize());
	brainJSON["gates"] = gatesJSON;

	brainJSON["inputShape"] = nlohmann::json::array({inputSizeX, inputSizeY, inputSizeT, inBitsPerPixel});
	brainJSON["filterShape"] = nlohmann::json::array({filterSizeX, filterSizeY, filterSizeT});
	brainJSON["stride"] = nlohmann::json::array({strideX, strideY, strideT});
	brainJSON["outputShape"] = nlohmann::json::array({outputSizeX, outputSizeY, outputSizeT, outBitsPerPixel});
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

	string brainJSONStr = orgData[string("BRAIN_") + name + "_json"];
	if(brainJSONStr.front()!='\'' || brainJSONStr.back()!='\'')
		throw invalid_argument(string("First or last character of the DETextureBrain JSON string field in the log is not a single quote.\nThe string: ") + brainJSONStr);
	brainJSONStr = brainJSONStr.substr(1, brainJSONStr.size()-2);

	nlohmann::json brainJSON = nlohmann::json::parse(brainJSONStr);

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

	nlohmann::json gatesJSON = brainJSON["gates"];
	gates.clear();
	for(const auto& gateJSON : gatesJSON) {
		if(gateJSON["type"] == "DeterministicTexture") {
			auto gate = make_shared<DeterministicTextureGate>();
			gate->deserialize(gateJSON);
			gate->updateOutputs(output);
			gates.push_back(gate);
		}
		else if(gateJSON["type"] == "ProbabilisticTexture") {
			throw invalid_argument("ProbabilisticTextureGate cannot be deserialized since it is not implemented yet");
		}
		else
			throw invalid_argument(string("DETextureBrain::deserialize: Unsupported gate type ") + static_cast<string>(gateJSON["type"]));
	}
}

/********** Private definitions **********/

tuple<size_t,size_t,size_t> DETextureBrain::computeOutputShape() {
	size_t outx = (inputSizeX-filterSizeX) / strideX + 1;
	size_t outy = (inputSizeY-filterSizeY) / strideY + 1;
	size_t outt = (inputSizeT-filterSizeT) / strideT + 1;
	return make_tuple(outx, outy, outt);
}

void DETextureBrain::resetOutputTexture() {
	fill_n(output->data(), output->num_elements(), 0);
}

void DETextureBrain::mutateStructurewide() {
//	unsigned internalChanges = 0;
//	unsigned connectionsChanges = 0;

//	cout << "Gates before the structurewide mutation:" << std::endl;
//	for(const auto& gate : gates)
//		cout << gate->description() << endl;

	for(size_t gi=0; gi<gates.size(); gi++) {
		if(Random::getDouble(1.)<structurewideMutationProbability) {
			gates[gi]->mutateInternalStructure();
//			internalChanges++;
		}
		if(Random::getDouble(1.)<structurewideMutationProbability) {
			rewireGateRandomly(gi);
//			connectionsChanges++;
		}
	}

//	std::cout << "Structurewide mutation operator finished, made "
//	          << internalChanges << " table changes and "
//	          << connectionsChanges << " changes to connections in a brain with "
//	          << gates.size() << " gates" << std::endl;

//	cout << "Gates after the structurewide mutation:" << std::endl;
//	for(const auto& gate : gates)
//		cout << gate->description() << endl;

}

void DETextureBrain::mainMutate() {
	double r = Random::getDouble(1.);
//	cout << "DETextureBrain " << this << " is mutating. Rolled " << r;
	if(r < gateInsertionProbabilityPL->get(PT)) {
//		cout << ", chose insertion. Had " << gates.size() << " gates, ";
		gates.push_back(getRandomGate(getLowestAvailableGateID()));
//		cout << " now it's " << gates.size() << endl;
		originationStory = "mutation_insertion";
	}
	else {
		if(gates.size()==0) {
			if(gateInsertionProbabilityPL->get(PT) > 0)
				mainMutate();
			else
				throw logic_error("DETextureBrain: mutation that cannot insert gates was called on a brain with no gates");
			return;
		}

		size_t idx = Random::getIndex(gates.size());

		if(r < gateInsertionProbabilityPL->get(PT)+gateDeletionProbabilityPL->get(PT)) {
			if(gates.size()<=minGates) {
				mainMutate();
				return;
			}
//			cout << ", chose deletion. Had " << gates.size() << " gates, ";
			gates.erase(gates.begin()+idx);
//			cout << ", removed " << idx << "th one, now it's " << gates.size() << endl;
			originationStory = "mutation_deletion";
		}
		else if(r < gateInsertionProbabilityPL->get(PT)+gateDeletionProbabilityPL->get(PT)+gateDuplicationProbabilityPL->get(PT)) {
//			cout << ", chose duplication. Had " << gates.size() << " gates, ";
			auto newGate = getGateCopy(idx, getLowestAvailableGateID());
			gates.push_back(newGate);
//			cout << ", duplicated " << idx << "th one, now it's " << gates.size() << endl;
			originationStory = "mutation_duplication";
		}
		else {
//			cout << ", chose intra-gate mutation. Chose gate " << idx;
			double spentProb = gateInsertionProbabilityPL->get(PT)+gateDeletionProbabilityPL->get(PT)+gateDuplicationProbabilityPL->get(PT);
			double tableChangeThr = spentProb + (1.-spentProb)/(1.+connectionToTableChangeRatioPL->get(PT));
			if(r < tableChangeThr) {
//				cout << " and a table mutation. Gate before the mutation:" << endl << gates[idx]->description() << endl;
				gates[idx]->mutateInternalStructure();
//				cout << "Gate after the mutation:" << endl << gates[idx]->description() << endl;
				originationStory = "mutation_table";
			}
			else {
//				cout << " and a wiring mutation. Gate before the mutation:" << endl << gates[idx]->description();
				rewireGateRandomly(idx);
//				cout << "Gate after the mutation:" << endl << gates[idx]->description() << endl;
				originationStory = "mutation_rewiring";
			}
		}
	}
}

void DETextureBrain::randomize() {
	gates.clear();
	for(unsigned g=0; g<initialGateCountPL->get(PT); g++) {
		auto newGate = getRandomGate(g);
		gates.push_back(newGate);
	}
}

void DETextureBrain::rewireGateRandomly(size_t gateIdx) {
	const size_t gateInputSize = gates[gateIdx]->inputsIndices.size();
	const size_t gateOutputSize = gates[gateIdx]->outputsIndices.size();
	size_t connectionIdx = Random::getIndex(gateInputSize+gateOutputSize);
	if(connectionIdx<gateInputSize)
		gates[gateIdx]->inputsIndices[connectionIdx] = getRandomInputTextureAddress();
	else {
		connectionIdx -= gateInputSize;
		gates[gateIdx]->outputsIndices[connectionIdx] = getRandomOutputTextureAddress();
		gates[gateIdx]->updateOutputs(output);
	}
}

shared_ptr<AbstractTextureGate> DETextureBrain::getGateCopy(size_t oldGateIdx, unsigned newGateID) {
	auto newGate = gates[oldGateIdx]->makeCopy(newGateID);
	newGate->updateOutputs(output); // makeCopy() method of gates does not copy the internal pointers, so they must be updated
	return newGate;
}

shared_ptr<AbstractTextureGate> DETextureBrain::getRandomGate(unsigned gateID) {
	const unsigned nins = Random::getInt(gateMinIns, gateMaxIns);
	const unsigned nouts = Random::getInt(gateMinOuts, gateMaxOuts);
	vector<tuple<size_t,size_t,size_t,size_t>> inputIdxs;
	vector<tuple<size_t,size_t,size_t,size_t>> outputIdxs;
	for(unsigned i=0; i<nins; i++)
		inputIdxs.push_back(getRandomInputTextureAddress());
	for(unsigned j=0; j<nouts; j++)
		outputIdxs.push_back(getRandomOutputTextureAddress());

	auto newGatePtr = make_shared<DeterministicTextureGate>(gateID, inputIdxs, outputIdxs);
	newGatePtr->updateOutputs(output); // output texture is used to update the internal pointers of the gate
	return newGatePtr;
}

tuple<size_t,size_t,size_t,size_t> DETextureBrain::getRandomInputTextureAddress() {
	return make_tuple(Random::getIndex(inputSizeX),
	                  Random::getIndex(inputSizeY),
	                  Random::getIndex(inputSizeT),
	                  Random::getIndex(inBitsPerPixel));
}

tuple<size_t,size_t,size_t,size_t> DETextureBrain::getRandomOutputTextureAddress() {
	return make_tuple(Random::getIndex(outputSizeX),
	                  Random::getIndex(outputSizeY),
	                  Random::getIndex(outputSizeT),
	                  Random::getIndex(outBitsPerPixel));
}

unsigned DETextureBrain::getLowestAvailableGateID() {
	unsigned newGateID;
	for(newGateID=0; newGateID<=gates.size(); newGateID++) {
		bool idIsGood = true;
		for(const auto& g : gates) {
			if(g->ID == newGateID) {
				idIsGood = false;
				break;
			}
		}
		if(idIsGood)
			break;
	}
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
