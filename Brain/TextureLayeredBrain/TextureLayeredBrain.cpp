#include <fstream>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include "../../Utilities/nlohmann/json.hpp" // https://github.com/nlohmann/json/ v3.6.1

#include "../DETextureBrain/DETextureBrain.h"
#include "../../Utilities/Random.h"

#include "TextureLayeredBrain.h"
#include "topology.h"

using namespace std;

/*******************************************/
/********** Auxiliary definitions **********/
/*******************************************/

vector<double> getDefaultMutationRatesOneMoreTime(const vector<string>& componentFileNames) {
	vector<double> mutRates(componentFileNames.size(), 0.);
	double numEvolvableBrains = count(componentFileNames.begin(), componentFileNames.end(), "");
	for(size_t i=0; i<componentFileNames.size(); i++)
		if(componentFileNames[i]=="")
			mutRates[i] = 1./numEvolvableBrains;
	return mutRates;
}

/****************************************/
/****** ParameterLink definitions *******/
/****************************************/

shared_ptr<ParameterLink<double>> TextureLayeredBrain::totalMutationProbabilityPL =
	Parameters::register_parameter("BRAIN_TEXTURE_LAYERED-totalMutationProbability", 1.0,
	"probability any mutation whatsoever occurs upon the call to mutate(), default: 1.0");


/****************************************/
/********** Public definitions **********/
/****************************************/

vector<shared_ptr<ParametersTable>> TextureLayeredBrain::layerPTs;
//const vector<unsigned> junctionSizes TEXTURE_BRAIN_COMPONENT_JUNCTION_SIZES;

TextureLayeredBrain::TextureLayeredBrain(int _nrInNodes, int _nrOutNodes, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(0, 0, PT_),
	visualize(Global::modePL->get() == "visualize") {

	if(_nrInNodes!=0) throw invalid_argument("TextureLayeredBrain: number of input nodes should be set to zero to work with textures");
	if(_nrOutNodes!=0) throw invalid_argument("TextureLayeredBrain: number of output nodes should be set to zero to work with textures");

	const vector<string> componentInTextureShapes TEXTURE_BRAIN_COMPONENT_IN_TEXTURE_SHAPES;
	const vector<int> componentInBitDepths TEXTURE_BRAIN_COMPONENT_IN_BIT_DEPTHS;
	const vector<string> componentFilterShapes TEXTURE_BRAIN_COMPONENT_FILTER_SHAPES;
	const vector<string> componentStrideShapes TEXTURE_BRAIN_COMPONENT_STRIDE_SHAPES;
	const vector<int> componentOutBitDepths TEXTURE_BRAIN_COMPONENT_OUT_BIT_DEPTHS;

	numLayers = componentInTextureShapes.size();
	if(numLayers<1) throw invalid_argument("TextureLayeredBrain: topology error, number of layers is less than one");
	if(componentInBitDepths.size()!=numLayers) throw invalid_argument("TextureLayeredBrain: topology error, number of in bit depths is different from number of textures");
	if(componentFilterShapes.size()!=numLayers) throw invalid_argument("TextureLayeredBrain: topology error, number of filter shapes is different from number of textures");
	if(componentStrideShapes.size()!=numLayers) throw invalid_argument("TextureLayeredBrain: topology error, number of stride shapes is different from number of textures");
	if(componentOutBitDepths.size()!=numLayers) throw invalid_argument("TextureLayeredBrain: topology error, number of out bit depths is different from number of textures");

#ifdef TEXTURE_BRAIN_COMPONENT_FILE_NAMES
	const vector<string> componentFileNames TEXTURE_BRAIN_COMPONENT_FILE_NAMES;
	if(componentFileNames.size()!=numLayers) throw invalid_argument("TextureLayeredBrain: topology error, number of component file names is different from number of textures");
#else // TEXTURE_BRAIN_COMPONENT_FILE_NAMES
	const vector<string> componentFileNames(numLayers, "");
#endif // TEXTURE_BRAIN_COMPONENT_FILE_NAMES

#ifdef TEXTURE_BRAIN_COMPONENT_MUTATION_RATES
	componentMutationRates = vector<double>(TEXTURE_BRAIN_COMPONENT_MUTATION_RATES);
	if(componentMutationRates.size()!=numLayers) throw invalid_argument("TextureLayeredBrain: topology error, number of mutation rates is different from number of textures");
#else // TEXTURE_BRAIN_COMPONENT_MUTATION_RATES
	componentMutationRates = getDefaultMutationRatesOneMoreTime(componentFileNames);
#endif // TEXTURE_BRAIN_COMPONENT_MUTATION_RATES

	layers.resize(numLayers);
	layerEvolvable.resize(numLayers, true);
	if(layerPTs.empty()) {
		layerPTs.resize(numLayers, nullptr);
		for(unsigned i=0; i<numLayers; i++) {
			layerPTs[i] = make_shared<ParametersTable>("", nullptr);

			///// Layer-specific parameters /////
			layerPTs[i]->setParameter("BRAIN_DETEXTURE-inTextureShape", componentInTextureShapes[i] );
			layerPTs[i]->setParameter("BRAIN_DETEXTURE-inBitsPerPixel", componentInBitDepths[i] );
			layerPTs[i]->setParameter("BRAIN_DETEXTURE-filterShape", componentFilterShapes[i] );
			layerPTs[i]->setParameter("BRAIN_DETEXTURE-strideShape", componentStrideShapes[i] );
			layerPTs[i]->setParameter("BRAIN_DETEXTURE-outBitsPerPixel", componentOutBitDepths[i] );
			///// Layer-specific paramters END /////

			///// Parameter forwarding /////
			layerPTs[i]->setParameter("BRAIN_DETEXTURE-gateIORanges", PT_->lookupString("BRAIN_DETEXTURE-gateIORanges"));
			const vector<string> forwardedDoubleParamNames = { "BRAIN_DETEXTURE-gateInsertionProbability",
			                                                   "BRAIN_DETEXTURE-gateDuplicationProbability",
			                                                   "BRAIN_DETEXTURE-gateDeletionProbability",
			                                                   "BRAIN_DETEXTURE-connectionToTableChangeRatio",
			                                                   "BRAIN_DETEXTURE-structurewideMutationProbability" };
			for(const auto& param : forwardedDoubleParamNames)
				layerPTs[i]->setParameter(param, PT_->lookupDouble(param));
			const vector<string> forwardedIntParamNames = { "BRAIN_DETEXTURE-initialGateCount",
			                                                "BRAIN_DETEXTURE-minGateCount",
			                                                "BRAIN_DETEXTURE-convolutionRegime" };
			for(const auto& param : forwardedIntParamNames)
				layerPTs[i]->setParameter(param, PT_->lookupInt(param));
			for(const auto& param : { "BRAIN_DETEXTURE-enableInputRewirings",
			                          "BRAIN_DETEXTURE-filtersOutputsAreShared" })
				layerPTs[i]->setParameter(param, PT_->lookupBool(param));
			///// Parameter forwarding END /////
		}
	}

	for(unsigned i=0; i<numLayers; i++) {
		// all brains start randomized, then some are read from their files
		layers[i] = DETextureBrain_brainFactory(0, 0, layerPTs[i]);
		if(!componentFileNames[i].empty()) {
			// file provided for the current layer, deserializing...
			ifstream brainFile(componentFileNames[i]);
			string layerJSONStr;
			getline(brainFile, layerJSONStr);
			brainFile.close();

			string name("currentLayer");
			unordered_map<string,string> surrogateOrgData { { string("BRAIN_") + name + "_json", string("'") + layerJSONStr + string("'") } };
			layers[i]->deserialize(PT_, surrogateOrgData, name);
			layerEvolvable[i] = false;
		}
		if(i>0)
			layers[i]->attachToSensors(layers[i-1]->getDataForMotors());
	}

	layerTextures.push_back(nullptr); // the first field is for the original texture received from sensors
	for(unsigned i=0; i<numLayers; i++)
		layerTextures.push_back(layers[i]->getDataForMotors());

	// columns to be added to ave file
	popFileColumns.clear();
//	popFileColumns.push_back("markovBrainGates");
//	popFileColumns.push_back("markovBrainDeterministicGates");
}

void TextureLayeredBrain::update(mt19937* rng) {
	for(auto layer : layers)
		layer->update(rng);
}

shared_ptr<AbstractBrain> TextureLayeredBrain::makeCopy(shared_ptr<ParametersTable> PT_) {
	if(PT_==nullptr) throw invalid_argument("TextureLayeredBrain::makeCopy caught a nullptr");
	if(PT_!=PT) throw invalid_argument("TextureLayeredBrain::makeCopy was called with a parameters table that is different from the one the original used. Are you sure you want to do that?");
	auto newBrain = make_shared<TextureLayeredBrain>(0, 0, PT);
	newBrain->layers.clear();
	for(unsigned l=0; l<numLayers; l++) {
		newBrain->layers.push_back(layers[l]->makeCopy(layerPTs[l]));
		if(l>0)
			newBrain->layers.back()->attachToSensors(newBrain->layers[l-1]->getDataForMotors());
	}
	newBrain->layerTextures.clear();
	newBrain->layerTextures.push_back(nullptr); // the first field is for the original texture received from sensors
	for(unsigned i=0; i<numLayers; i++)
		newBrain->layerTextures.push_back(newBrain->layers[i]->getDataForMotors());
	return newBrain;
}

shared_ptr<AbstractBrain> TextureLayeredBrain::makeBrain(unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	return make_shared<TextureLayeredBrain>(0, 0, PT);
}

shared_ptr<AbstractBrain> TextureLayeredBrain::makeBrainFromMany(vector<shared_ptr<AbstractBrain>> _brains,
                                                                 unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	return makeCopy(PT);
}

void TextureLayeredBrain::mutate() {
	double r0 = Random::getDouble(1.);
	if(r0>totalMutationProbabilityPL->get(PT))
		return;

	vector<double> mutationThresholds;
	mutationThresholds.push_back(componentMutationRates[0]);
	for(size_t l=1; l<numLayers; l++)
		mutationThresholds.push_back(mutationThresholds.back()+componentMutationRates[l]);

	double r = Random::getDouble(1.);
	size_t l = 0;
	for(l=0; l<numLayers; l++)
		if(r<mutationThresholds[l])
			break;
	if(l==numLayers)
		l--;

	layers[l]->mutate();

	// Attachment of the outputs is updated when the layers mutate,
	// but attachment of inputs is not. Here it's done manually
	for(size_t i=1; i<numLayers; i++)
		layers[i]->attachToSensors(layers[i-1]->getDataForMotors());
}

void TextureLayeredBrain::resetBrain() {
	AbstractBrain::resetBrain();
	for(auto& l : layers)
		l->resetBrain();
}

void TextureLayeredBrain::attachToSensors(void* inputPtr) {
	layers[0]->attachToSensors(inputPtr);
	layerTextures.front() = inputPtr;
}

void* TextureLayeredBrain::getDataForMotors() {
	return &layerTextures;
}

nlohmann::json TextureLayeredBrain::getPostEvaluationStats() {
	long numErasures = 0;
	for(auto& l : layers)
		numErasures += stol(l->getPostEvaluationStats()["erasures"].get<string>());
	nlohmann::json stats = nlohmann::json::object();
	stats["erasures"] = to_string(numErasures);
	return stats;
}

string TextureLayeredBrain::description() {
  string S = "Layered Brain with " + to_string(numLayers) + " layers\n";
	for(unsigned l=0; l<numLayers; l++) {
		S += "Layer " + to_string(l) + " of type " + layers[l]->getType() +
		     " is" + (layerEvolvable[l]?"":" not") + " evolvable, rate " + to_string(componentMutationRates[l]) + "\n";
		S += layers[l]->description();
	}
	return S;
}

DataMap TextureLayeredBrain::getStats(string& prefix) {
	DataMap dataMap;
	int totGates = 0;
	int totl1 = 0;
	int totl2 = 0;
	for(unsigned l=0; l<numLayers; l++) {
		string fullPrefix = prefix + (prefix==""?"":"_") + "brain" + to_string(l) + "_";
		dataMap.merge(layers[l]->getStats(fullPrefix));
		totGates += dataMap.getInt(fullPrefix + "markovBrainGates");
		totl1 += dataMap.getInt(fullPrefix + "outputDensityL1");
		totl2 += dataMap.getInt(fullPrefix + "outputDensityL2sq");
	}
	dataMap.set("markovBrainGates", totGates);
	dataMap.set("outputDensityL1", totl1);
	dataMap.set("outputDensityL2sq", totl2);
	return dataMap;
}

void* TextureLayeredBrain::logTimeSeries(const string& label) {
	return nullptr;
}

DataMap TextureLayeredBrain::serialize(string& name) {
	nlohmann::json brainsJSONs = nlohmann::json::array();
	for(unsigned l=0; l<numLayers; l++) {
		string name("curBrain");
		DataMap curSerialization = layers[l]->serialize(name);
		string curJSONString = curSerialization.getString("curBrain_json");
		curJSONString = curJSONString.substr(1, curJSONString.size()-2);
		nlohmann::json curBrainJSON = nlohmann::json::parse(curJSONString);
		brainsJSONs.push_back(curBrainJSON);
	}

	// storing the string at the DataMap and returning it
	DataMap dm;
	dm.set(name + "_json", string("'") + brainsJSONs.dump() + "'");
	return dm;
}

void TextureLayeredBrain::deserialize(shared_ptr<ParametersTable> PT, unordered_map<string,string>& orgData, string& name) {
	string brainJSONsStr = orgData[string("BRAIN_") + name + "_json"];

	if(brainJSONsStr.front()!='\'' || brainJSONsStr.back()!='\'')
		throw invalid_argument(string("First or last character of the TextureLayeredBrain JSON string field in the log is not a single quote.\nThe string: ") + brainJSONsStr);

	brainJSONsStr = brainJSONsStr.substr(1, brainJSONsStr.size()-2);

	nlohmann::json brainsJSONs = nlohmann::json::parse(brainJSONsStr);

	for(unsigned l=0; l<numLayers; l++) {
		string name = string("brain") + to_string(l);
		unordered_map<string,string> surrogateOrgData { { string("BRAIN_")+name+"_json", string("'") + brainsJSONs[l].dump() + string("'") } };
		layers[l]->deserialize(PT, surrogateOrgData, name);
		if(l>0)
			layers[l]->attachToSensors(layers[l-1]->getDataForMotors());
	}
}
