#include <fstream>
#include <cstdlib>
#include <sstream>
#include "../../Utilities/nlohmann/json.hpp" // https://github.com/nlohmann/json/ v3.6.1

#include "../DEMarkovBrain/DEMarkovBrain.h"
#include "../../Utilities/Random.h"

#include "LayeredBrain.h"

using namespace std;

/********** Static variables definition **********/

/*
shared_ptr<ParameterLink<int>> DEMarkovBrain::hiddenNodesPL = Parameters::register_parameter("BRAIN_DEMARKOV-hiddenNodes", 8, "number of hidden nodes");
shared_ptr<ParameterLink<bool>> DEMarkovBrain::recordIOMapPL = Parameters::register_parameter("BRAIN_DEMARKOV_ADVANCED-recordIOMap", false,
                                                                                              "if true, all input, output and hidden nodes will be recorded on every brain update");
shared_ptr<ParameterLink<string>> DEMarkovBrain::IOMapFileNamePL = Parameters::register_parameter("BRAIN_DEMARKOV_ADVANCED-recordIOMap_fileName", (string) "markov_IO_map.csv",
                                                                                                  "Name of file where IO mappings are saved");
shared_ptr<ParameterLink<int>> DEMarkovBrain::initialGateCountPL = Parameters::register_parameter("BRAIN_DEMARKOV-initialGateCount", 30, "number of gates to add to the newly generated individuals");
shared_ptr<ParameterLink<double>> DEMarkovBrain::gateInsertionProbabilityPL = Parameters::register_parameter("BRAIN_DEMARKOV-gateInsertionProbability", 0.1,
                                                                                                             "probability that a new random gate will be inserted into the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DEMarkovBrain::gateDeletionProbabilityPL = Parameters::register_parameter("BRAIN_DEMARKOV-gateDeletionProbability", 0.1,
                                                                                                            "probability that a randomly chosen gate will be deleted from the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DEMarkovBrain::gateDuplicationProbabilityPL = Parameters::register_parameter("BRAIN_DEMARKOV-gateDuplicationProbability", 0.2,
                                                                                                               "probability that a randomly chosen gate will be duplicated within the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DEMarkovBrain::connectionToTableChangeRatioPL = Parameters::register_parameter("BRAIN_DEMARKOV-connectionToTableChangeRatio", 1.0,
                                                                                                                 "if the brain experiences no insertion, deletions or duplictions, call to mutate() changes parameters of a randomly chosen gate. This parameters governs relative frequencies of such changes to connections and logic tables");
shared_ptr<ParameterLink<int>> DEMarkovBrain::minGateCountPL = Parameters::register_parameter("BRAIN_DEMARKOV-minGateCount", 0, "number of gates that causes gate deletions to become impossible (mutation operator calls itself if the mutation happens to be a deletion)");
*/

/********** Public definitions **********/

LayeredBrain::LayeredBrain(int _nrInNodes, int _nrOutNodes, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(_nrInNodes, _nrOutNodes, PT_),
	visualize(Global::modePL->get() == "visualize") {

	const vector<string> brainFileNames { "layer0.json", "" };
	const vector<unsigned> junctionSizes { 20 };
	const vector<double> constMutationRates { 0., 1. };

	numLayers = brainFileNames.size();
	if( numLayers<1 ) {
		cerr << "LayeredBrain: there must be at least one layer, you requested " << numLayers << ", exiting" << endl;
		exit(EXIT_FAILURE);
	}
	layers.resize(numLayers);
	layerEvolvable.resize(numLayers, true);
	for(unsigned i=0; i<numLayers; i++) {
		layers[i] = DEMarkovBrain_brainFactory(i==0 ? _nrInNodes : junctionSizes[i-1],
		                                       i==numLayers-1 ? _nrOutputNodes : junctionSizes[i],
		                                       PT_); // all brains start randomized, then some are read from their files
		if(!bfn.empty()) {
			ifstream brainFile(bfn);
			string layerJSONStr;
			getline(brainFile, layerJSONStr);
			brainFile.close();

			string name("currentLayer");
			unordered_map<string,string> surrogateOrgData { { string("BRAIN_") + name + "_json",  layerJSONStr };
			layers[i]->deserialize(PT_, surrogateOrgData, name);
			layerEvolvable = false;
		}
	}
	mutationRates = constMutationRates;

	// columns to be added to ave file
	popFileColumns.clear();
//	popFileColumns.push_back("markovBrainGates");
//	popFileColumns.push_back("markovBrainDeterministicGates");
}

//void LayeredBrain::randomize() {
//}

void LayeredBrain::update() {

	for(int i=0; i<nrInputValues; i++)
		layers[0]->setInput(i, inputValues[i]);
	layers[0]->update();

	for(unsigned l=1; l<numLayers; l++) {
		for(int i=0; i<(layers[l]->nrInputValues); i++)
			layers[l]->setInput(i, layers[l-1]->readOutput(i));
		layers[l]->update();
	}

	for(int i=0; i<nrOutputValues; i++)
		outputValues[i] = layers[numLayers-1]->readOutput(i);
}

shared_ptr<AbstractBrain> LayeredBrain::makeCopy(shared_ptr<ParametersTable> PT_) {
	auto newBrain = make_shared<LayeredBrain>(nrInputValues, nrOutputValues, PT);
	for(unsigned l=0; l<numLayers; l++)
		newBrain->layers[l] = layers[l]->makeCopy();
	return newBrain;
}

shared_ptr<AbstractBrain> LayeredBrain::makeBrain(unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	auto newBrain = make_shared<LayeredBrain>(nrInputValues, nrOutputValues, PT);
	newBrain->randomize();
	return newBrain;
}

shared_ptr<AbstractBrain> LayeredBrain::makeBrainFromMany(vector<shared_ptr<AbstractBrain>> _brains,
                                                           unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	auto newBrain = makeCopy(PT);
	return newBrain; // note that mutate() WILL be called upon it right away
}

void LayeredBrain::mutate() {
	vector<double> mutationThresholds;
	mutationThresholds.push_back(mutationRates[0]);
	for(unsigned l=1; l<numLayers; l++)
		mutationThresholds.push_back(mutationThresholds.back()+mutationRates[l]);

	double r = Random::getDouble(1.);
	unsigned l = 0;
	for(l=0; l<numLayers; l++)
		if(r<mutationThresholds[l])
			break;

//	cout << "LayeredBrain is mutating. Rolled " << r << ", choosing " << l << "th brain to mutate" << endl;

	layers[l]->mutate();
}

void LayeredBrain::resetBrain() {
	AbstractBrain::resetBrain();
	for(auto& l : layers)
		l->resetBrain();
}

//void LayeredBrain::resetInputs() {
//	AbstractBrain::resetInputs();
//}

//void LayeredBrain::resetOutputs() {
//	AbstractBrain::resetOutputs();
//}

string LayeredBrain::description() {
  string S = "Layered Brain\nins:" + to_string(nrInputValues) + " outs:" +
             to_string(nrOutputValues) + " layers:" + to_string(numLayers) + "\n";
	for(unsigned l=0; l<numLayers; l++) {
		S += "Layer " + to_string(l) + " of type " + layers[l]->getType() +
		     " is" + (layerEvolvable[l]?"":" not") + " evolvable, rate " + to_string(mutationRates[l]) + "\n";
	}
	return S;
}

DataMap LayeredBrain::getStats(string& prefix) {
	DataMap dataMap;
	for(unsigned l=0; l<numLayers; l++) {
		string prefix = string("brain") + to_string(l);
		dataMap.merge(layers[l]->getStats(prefix));
	}
	return dataMap;
}

void* LayeredBrain::logTimeSeries(const string& label) {
	return nullptr;
}

DataMap LayeredBrain::serialize(string& name) {
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
	dm.set(name + "_json", string("'") + brainJSONs.dump() + "'");
	return dm;
}

void LayeredBrain::deserialize(shared_ptr<ParametersTable> PT, unordered_map<string,string>& orgData, string& name) {
	string brainJSONsStr = orgData[string("BRAIN_") + name + "_json"];
	if(brainJSONsStr.front()!='\'' || brainJSONsStr.back()!='\'') {
		cout << "First or last character of the LayeredBrain JSON string field in the log is not a single quote" << endl;
		cout << "The string: " << brainJSONsStr << endl;
		exit(EXIT_FAILURE);
	}
	brainJSONsStr = brainJSONsStr.substr(1, brainJSONsStr.size()-2);

	nlohmann::json brainJSONs = nlohmann::json::parse(brainJSONsStr);

	for(unsigned l=0; l<numLayers; l++) {
		string name = string("brain") + to_string(l);
		unordered_map<string,string> surrogateOrgData { { string("BRAIN_")+name+"_json", string("'") + brainJSONs[l].dump() + string("'") } };
		layers[l]->deserialize(PT, surrogateOrgData, name);
	}
}

/********** Private definitions **********/
