#include <sstream>
#include <iomanip>
#include <png++/png.hpp>

#include "../MarkovBrain/Gate/DeterministicGate.h"

#include "DEMarkovBrain.h"

using namespace std;

/********** Auxiliary funcs **********/

tuple<int,int,int,int> parseGateLimitsStr(string thestring) {
	string sinmin, sinmax, soutmin, soutmax;
	stringstream s(thestring);
	getline(s, sinmin, '-');
	getline(s, sinmax, ',');
	getline(s, soutmin, '-');
	getline(s, soutmax);
	return make_tuple(stoi(sinmin), stoi(sinmax), stoi(soutmin), stoi(soutmax));
}

/********** Static variables definition **********/

shared_ptr<ParameterLink<int>> DEMarkovBrain::hiddenNodesPL = Parameters::register_parameter("BRAIN_DEMARKOV-hiddenNodes", 8, "number of hidden nodes");
shared_ptr<ParameterLink<bool>> DEMarkovBrain::recordIOMapPL = Parameters::register_parameter("BRAIN_DEMARKOV_ADVANCED-recordIOMap", false,
                                                                                              "if true, all input, output and hidden nodes will be recorded on every brain update");
shared_ptr<ParameterLink<string>> DEMarkovBrain::IOMapFileNamePL = Parameters::register_parameter("BRAIN_DEMARKOV_ADVANCED-recordIOMap_fileName", (string) "markov_IO_map.csv",
                                                                                                  "Name of file where IO mappings are saved");
shared_ptr<ParameterLink<int>> DEMarkovBrain::initialGateCountPL = Parameters::register_parameter("BRAIN_DEMARKOV-initialGateCount", 30, "number of gates to add to the newly generated individuals");

/********** Public definitions **********/

DEMarkovBrain::DEMarkovBrain(int _nrInNodes, int _nrHidNodes, int _nrOutNodes, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(_nrInNodes, _nrOutNodes, PT_),
	nrHiddenNodes(_nrHidNodes),
	nrNodes(_nrInNodes+_nrOutNodes+_nrHidNodes),
	visualize(Global::modePL->get() == "visualize"),
	recordIOMap(recordIOMapPL->get(PT_)) {

	nodes.resize(nrNodes, 0);
	nextNodes.resize(nrNodes, 0);

	// columns to be added to ave file
	popFileColumns.clear();
	popFileColumns.push_back("markovBrainGates");
	popFileColumns.push_back("markovBrainDeterministicGates");

	// gates params
	tie(gateMinIns, gateMaxIns, gateMinOuts, gateMaxOuts) =
		parseGateLimitsStr(DeterministicGate::IO_RangesPL->get(PT_));
}

void DEMarkovBrain::update() {
	nextNodes.assign(nrNodes, 0.0);
	DataMap IOMap;

	for(int i=0; i<nrInputValues; i++)
		nodes[i] = inputValues[i];

	if(visualize) {
		log.logStateBeforeUpdate(nodes);
		nodesStatesTimeSeries.push_back(nodes);
	}

	if(recordIOMap)
		for(int i=0; i<nrInputValues; i++)
			IOMap.append("input", Bit(nodes[i]));

	for(auto &g :gates) // update each gate
		g->update(nodes, nextNodes);

	swap(nodes, nextNodes);
	for(int i=0; i<nrOutputValues; i++)
		outputValues[i] = nodes[nrInputValues+i];

	if(recordIOMap) {
		for(int i=0; i<nrOutputValues; i++)
			IOMap.append("output", Bit(nodes[nrInputValues+i]));
		for(int i=nrInputValues+nrOutputValues; i<nodes.size() ;i++)
			IOMap.append("hidden", Bit(nodes[i]));
		IOMap.setOutputBehavior("input", DataMap::LIST);
		IOMap.setOutputBehavior("output", DataMap::LIST);
		IOMap.setOutputBehavior("hidden", DataMap::LIST);
		IOMap.writeToFile(IOMapFileNamePL->get());
		IOMap.clearMap();
	}

	if(visualize)
		log.logStateAfterUpdate(nodes);
}

shared_ptr<AbstractBrain> DEMarkovBrain::makeCopy(shared_ptr<ParametersTable> PT_) {
	if (PT_ == nullptr) {
		cerr << "DEMarkovBrain::makeCopy caught a nullptr" << endl;
		exit(EXIT_FAILURE);
	}

	auto newBrain = make_shared<DEMarkovBrain>(nrInputValues, nrHiddenNodes, nrOutputValues, PT);
	for(auto gate : gates)
		newBrain->gates.push_back(gate->makeCopy());
	return newBrain;
}

shared_ptr<AbstractBrain> DEMarkovBrain::makeBrain(unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	auto newBrain = make_shared<DEMarkovBrain>(nrInputValues, nrHiddenNodes, nrOutputValues, PT);
	newBrain->randomize();
	return newBrain;
}

shared_ptr<AbstractBrain> DEMarkovBrain::makeBrainFromMany(vector<shared_ptr<AbstractBrain>> _brains,
                                                           unordered_map<string,shared_ptr<AbstractGenome>>& _genomes) {
	auto newBrain = makeCopy(PT);
	return newBrain; // note that mutate() WILL be called upon it right away
}

void DEMarkovBrain::mutate() {
	// YOUR WORK HERE IS NOT DONE
}

void DEMarkovBrain::resetBrain() {
	AbstractBrain::resetBrain();
	nodes.assign(nrNodes, 0.0);
	for(auto &g :gates)
		g->resetGate();
	if(visualize) {
		log.logBrainReset();
		nodesStatesTimeSeries.clear();
	}
}

void DEMarkovBrain::resetInputs() {
	AbstractBrain::resetInputs();
	if(visualize)
		log.log("Inputs were reset\n");
}

void DEMarkovBrain::resetOutputs() {
	AbstractBrain::resetOutputs();
	if(visualize)
		log.log("Outputs were reset\n");
}

DataMap DEMarkovBrain::getStats(string& prefix) {
	DataMap dataMap;
	dataMap.set(prefix + "markovBrainGates", (int) gates.size());
	map<string,int> gatecounts;
	gatecounts["DeterministicGates"] = 0;
	for (auto &g : gates) {
		if(g->gateType()=="Deterministic")
			gatecounts["DeterministicGates"]++;
		else {
			cerr << "DEMarkovBrain::getStats: unexpected gate type " << g->gateType() << endl;
			exit(EXIT_FAILURE);
		}
	}
	dataMap.set(prefix + "markovBrainDeterministicGates", gatecounts["DeterministicGates"]);
	return dataMap;
}

void* DEMarkovBrain::logTimeSeries(const string& label) {
	const unsigned m = 4; // size of the pixel in the resulting pixelart brain activity log

	if(nodesStatesTimeSeries[0].size()!=nrNodes) {
		cerr << "Wrong number of nodes in the saved time series! Exiting..." << endl;
		exit(EXIT_FAILURE);
	}

	png::image<png::rgb_pixel> eegImage(m*(nrNodes+3), m*nodesStatesTimeSeries.size());
	for(unsigned Y=0; Y<nodesStatesTimeSeries.size(); Y++)
		for(unsigned y=Y*m; y<(Y+1)*m; y++)
			for(unsigned X=0; X<(nrNodes+3); X++)
				for(unsigned x=X*m; x<(X+1)*m; x++) {
					png::rgb_pixel p;
					if(X==nrInputValues)
						p = png::rgb_pixel(0, 255, 0);
					else if(X==nrInputValues+nrOutputValues+1)
						p = png::rgb_pixel(255, 0, 0);
					else if(X==nrNodes+2)
						p = png::rgb_pixel(0, 0, 255);
					else {
						unsigned localX = X>=nrInputValues ? (X>=nrInputValues+nrOutputValues+1 ? X-2 : X-1 ) : X;
						p = nodesStatesTimeSeries[Y][localX]==0 ? png::rgb_pixel(0,0,0) : png::rgb_pixel(255,255,255);
					}
					eegImage[y][x] = p;
				}

	eegImage.write(string("markovBrainStates_") + label);

	log.log(string("States for ") + label + string(" are written into an image\n"));

	return nullptr;
}

string DEMarkovBrain::description() {
  string S = "DEMarkov Brain\nins:" + to_string(nrInputValues) + " outs:" +
             to_string(nrOutputValues) + " hidden:" + to_string(nrHiddenNodes);
	S += "\nInput IDs:";
	for(int i=0; i<nrInputValues; i++)
		S += " " + to_string(i);
	S += "\nOutput IDs:";
	for(int i=nrInputValues; i<nrInputValues+nrOutputValues; i++)
		S += " " + to_string(i);
	S += "\nHidden IDs:";
	for(int i=nrInputValues+nrOutputValues; i<nrNodes; i++)
		S += " " + to_string(i);
	S += "\n";
	for (auto &g : gates)
		S += g->description();
	return S;
}

/********** Private definitions **********/

void DEMarkovBrain::randomize() {
	gates.clear();
	for(unsigned g=0; g<initialGateCountPL->get(PT); g++) {
		auto newGate = getRandomGate(g);
		gates.push_back(newGate);
	}
}

shared_ptr<AbstractGate> DEMarkovBrain::getRandomGate(int gateID) {
	const int nins = Random::getInt(gateMinIns, gateMaxIns);
	const int nouts = Random::getInt(gateMinOuts, gateMaxOuts);
	pair<vector<int>,vector<int>> conns;
	for(unsigned i=0; i<nins; i++)
		conns.first.push_back(Random::getInt(0, nrNodes-1));
	for(unsigned j=0; j<nouts; j++)
		conns.second.push_back(Random::getInt(nrInputValues, nrNodes-1));

	const int tableRows = 1<<nins;
	vector<vector<int>> table;
	for(unsigned k=0; k<tableRows; k++) {
		vector<int> row;
		for(unsigned j=0; j<nouts; j++)
			row.push_back(Random::getInt(0, 1));
		table.push_back(row);
	}

	return make_shared<DeterministicGate>(conns, table, gateID, PT);
}

void DEMarkovBrain::beginLogging() {
	if(visualize) {
		log.open(Global::outputPrefixPL->get() + "markov_log");
		logBrainStructure();
		log.log("\nActivity log:\n");
	}
}

void DEMarkovBrain::logBrainStructure() {
	log.log("begin brain desription\n" + description() + "end brain description\n");
}
