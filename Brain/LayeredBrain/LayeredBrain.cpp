#include <fstream>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include "../../Utilities/nlohmann/json.hpp" // https://github.com/nlohmann/json/ v3.6.1

#include "../DEMarkovBrain/DEMarkovBrain.h"
#include "../../Utilities/Random.h"

#include "LayeredBrain.h"
#include "topology.h"

using namespace std;

/*******************************************/
/********** Auxiliary definitions **********/
/*******************************************/

vector<double> getDefaultMutationRates(const vector<string>& componentFileNames) {
	vector<double> mutRates(componentFileNames.size(), 0.);
	double numEvolvableBrains = count(componentFileNames.begin(), componentFileNames.end(), "");
	for(size_t i=0; i<componentFileNames.size(); i++)
		if(componentFileNames[i]=="")
			mutRates[i] = 1./numEvolvableBrains;
	return mutRates;
}

/**************************************************/
/********** Static variables definitions **********/
/**************************************************/

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

/****************************************/
/********** Public definitions **********/
/****************************************/

vector<shared_ptr<ParametersTable>> LayeredBrain::layerPTs;
const vector<unsigned> junctionSizes BRAIN_COMPONENT_JUNCTION_SIZES;

LayeredBrain::LayeredBrain(int _nrInNodes, int _nrOutNodes, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(_nrInNodes, accumulate(junctionSizes.begin(), junctionSizes.end(), 0), PT_),
	visualize(Global::modePL->get() == "visualize") {

#ifdef BRAIN_COMPONENT_FILE_NAMES
	const vector<string> brainFileNames BRAIN_COMPONENT_FILE_NAMES;
#else // BRAIN_COMPONENT_FILE_NAMES
	const vector<string> brainFileNames(junctionSizes.size(), "");
#endif // BRAIN_COMPONENT_FILE_NAMES

#ifdef BRAIN_COMPONENT_MUTATION_RATES
	mutationRates = vector<double>(BRAIN_COMPONENT_MUTATION_RATES);
#else // BRAIN_COMPONENT_MUTATION_RATES
	mutationRates = getDefaultMutationRates(brainFileNames);
#endif // BRAIN_COMPONENT_MUTATION_RATES

#ifdef BRAIN_COMPONENT_HIDDEN_NODES
	const vector<int> hiddenNodes BRAIN_COMPONENT_HIDDEN_NODES;
#else // BRAIN_COMPONENT_HIDDEN_NODES
	const vector<int> hiddenNodes(brainFileNames.size(), 0);
#endif // BRAIN_COMPONENT_HIDDEN_NODES

//	cout << "BFNs:"; for(auto s : brainFileNames) cout << ' ' << '"' << s << '"';
//	cout << "\nJSs:"; for(auto js : junctionSizes) cout << " " << js;
//	cout << "\nHNs:"; for(auto hn : hiddenNodes) cout << " " << hn;
//	cout << "\nMRs:"; for(auto mr : mutationRates) cout << " " << mr; cout << endl;

	numLayers = brainFileNames.size();
	if( numLayers<1 ) {
		cerr << "LayeredBrain: there must be at least one layer, you requested " << numLayers << ", exiting" << endl;
		exit(EXIT_FAILURE);
	}
	layers.resize(numLayers);
	layerEvolvable.resize(numLayers, true);
	if(layerPTs.empty())
		layerPTs.resize(numLayers, nullptr);
	for(unsigned i=0; i<numLayers; i++) {
		if(layerPTs[i]==nullptr) {
			layerPTs[i] = make_shared<ParametersTable>("", nullptr);

			///// Layer-specific parameters /////
			layerPTs[i]->setParameter("BRAIN_DEMARKOV-hiddenNodes", hiddenNodes[i] );
			///// Layer-specific paramters END /////

			///// Parameter forwarding /////
			layerPTs[i]->setParameter("BRAIN_DEMARKOV_ADVANCED-recordIOMap",
			                    PT_->lookupBool("BRAIN_DEMARKOV_ADVANCED-recordIOMap"));
			layerPTs[i]->setParameter("BRAIN_MARKOV_GATES_DETERMINISTIC-IO_Ranges",
			                    PT_->lookupString("BRAIN_MARKOV_GATES_DETERMINISTIC-IO_Ranges"));
			const vector<string> forwardedDoubleParamNames = { "BRAIN_DEMARKOV-gateInsertionProbability",
			                                                   "BRAIN_DEMARKOV-gateDuplicationProbability",
			                                                   "BRAIN_DEMARKOV-gateDeletionProbability",
			                                                   "BRAIN_DEMARKOV-connectionToTableChangeRatio" };
			for(const auto& param : forwardedDoubleParamNames)
				layerPTs[i]->setParameter(param, PT_->lookupDouble(param));
			const vector<string> forwardedIntParamNames = { "BRAIN_DEMARKOV-initialGateCount",
			                                                "BRAIN_DEMARKOV-minGateCount" };
			for(const auto& param : forwardedIntParamNames)
				layerPTs[i]->setParameter(param, PT_->lookupInt(param));
			layerPTs[i]->setParameter("BRAIN_DEMARKOV-readFromInputsOnly", PT_->lookupBool("BRAIN_DEMARKOV-readFromInputsOnly"));
			///// Parameter forwarding END /////
		}

		layers[i] = DEMarkovBrain_brainFactory(i==0 ? _nrInNodes : junctionSizes[i-1],
		                                       junctionSizes[i],
		                                       layerPTs[i]); // all brains start randomized, then some are read from their files
		if(!brainFileNames[i].empty()) {
			// file provided for the current layer, deserializing...
			ifstream brainFile(brainFileNames[i]);
			string layerJSONStr;
			getline(brainFile, layerJSONStr);
			brainFile.close();

			string name("currentLayer");
			unordered_map<string,string> surrogateOrgData { { string("BRAIN_") + name + "_json", string("'") + layerJSONStr + string("'") } };
			layers[i]->deserialize(PT_, surrogateOrgData, name);
			layerEvolvable[i] = false;
		}
	}

	// columns to be added to ave file
	popFileColumns.clear();
//	popFileColumns.push_back("markovBrainGates");
//	popFileColumns.push_back("markovBrainDeterministicGates");
}

void LayeredBrain::update() {
	for(int i=0; i<nrInputValues; i++)
		layers[0]->setInput(i, inputValues[i]);
	layers[0]->update();

	unsigned shift = 0;
	for(unsigned l=1; l<numLayers; l++) {
		for(int i=0; i<(layers[l]->nrInputValues); i++) {
			outputValues[shift+i] = layers[l-1]->readOutput(i);
			layers[l]->setInput(i, layers[l-1]->readOutput(i));
		}
		layers[l]->update();
		shift += layers[l]->nrInputValues;
	}

	for(int i=0; i<layers.back()->nrOutputValues; i++)
		outputValues[shift+i] = layers[numLayers-1]->readOutput(i);
}

shared_ptr<AbstractBrain> LayeredBrain::makeCopy(shared_ptr<ParametersTable> PT_) {
	auto newBrain = make_shared<LayeredBrain>(nrInputValues, nrOutputValues, PT);
	for(unsigned l=0; l<numLayers; l++)
		newBrain->layers[l] = layers[l]->makeCopy(PT_);
	return newBrain;
}

shared_ptr<AbstractBrain> LayeredBrain::makeBrain(unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	auto newBrain = make_shared<LayeredBrain>(nrInputValues, nrOutputValues, PT);
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

	layers[l]->mutate();
}

void LayeredBrain::resetBrain() {
	AbstractBrain::resetBrain();
	for(auto& l : layers)
		l->resetBrain();
}

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
	int totGates = 0;
	for(unsigned l=0; l<numLayers; l++) {
		string fullPrefix = prefix + (prefix==""?"":"_") + "brain" + to_string(l) + "_";
		dataMap.merge(layers[l]->getStats(fullPrefix));
		totGates += dataMap.getInt(fullPrefix + "markovBrainGates");
	}

	dataMap.set("markovBrainGates", totGates);
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
	dm.set(name + "_json", string("'") + brainsJSONs.dump() + "'");
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

	nlohmann::json brainsJSONs = nlohmann::json::parse(brainJSONsStr);

	for(unsigned l=0; l<numLayers; l++) {
		string name = string("brain") + to_string(l);
		unordered_map<string,string> surrogateOrgData { { string("BRAIN_")+name+"_json", string("'") + brainsJSONs[l].dump() + string("'") } };
		layers[l]->deserialize(PT, surrogateOrgData, name);
	}
}

/*****************************************/
/********** Private definitions **********/
/*****************************************/
