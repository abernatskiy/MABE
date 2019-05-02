//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/Hintzelab/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/Hintzelab/MABE/wiki/License

#include <sstream>
#include <iomanip>
#include <png++/png.hpp>

#include "DEMarkovBrain.h"

std::shared_ptr<ParameterLink<bool>> DEMarkovBrain::recordIOMapPL=
    Parameters::register_parameter(
        "BRAIN_DEMARKOV_ADVANCED-recordIOMap", false,
        "if true, all inoput output and hidden nodes will be recorderd on every brain update");
std::shared_ptr<ParameterLink<std::string>> DEMarkovBrain::IOMapFileNamePL=
    Parameters::register_parameter("BRAIN_DEMARKOV_ADVANCED-recordIOMap_fileName",
                                   (std::string) "markov_IO_map.csv",
                                   "Name of file where IO mappings are saved");
std::shared_ptr<ParameterLink<bool>> DEMarkovBrain::randomizeUnconnectedOutputsPL =
    Parameters::register_parameter(
        "BRAIN_DEMARKOV_ADVANCED-randomizeUnconnectedOutputs", false,
        "output nodes with no connections will be set randomly (default : "
        "false, behavior set to 0)");
std::shared_ptr<ParameterLink<int>> DEMarkovBrain::randomizeUnconnectedOutputsTypePL =
    Parameters::register_parameter(
        "BRAIN_DEMARKOV_ADVANCED-randomizeUnconnectedOutputsType", 0,
        "determines type of values resulting from randomizeUnconnectedOutput "
        "[0 = int, 1 = double]");
std::shared_ptr<ParameterLink<double>>
    DEMarkovBrain::randomizeUnconnectedOutputsMinPL =
        Parameters::register_parameter(
            "BRAIN_DEMARKOV_ADVANCED-randomizeUnconnectedOutputsMin", 0.0,
            "random values resulting from randomizeUnconnectedOutput will be "
            "in the range of randomizeUnconnectedOutputsMin to "
            "randomizeUnconnectedOutputsMax");
std::shared_ptr<ParameterLink<double>>
    DEMarkovBrain::randomizeUnconnectedOutputsMaxPL =
        Parameters::register_parameter(
            "BRAIN_DEMARKOV_ADVANCED-randomizeUnconnectedOutputsMax", 1.0,
            "random values resulting from randomizeUnconnectedOutput will be "
            "in the range of randomizeUnconnectedOutputsMin to "
            "randomizeUnconnectedOutputsMax");
std::shared_ptr<ParameterLink<int>> DEMarkovBrain::hiddenNodesPL =
    Parameters::register_parameter("BRAIN_DEMARKOV-hiddenNodes", 8,
                                   "number of hidden nodes");
std::shared_ptr<ParameterLink<std::string>> DEMarkovBrain::genomeNamePL =
    Parameters::register_parameter("BRAIN_DEMARKOV-genomeNameSpace",
                                   (std::string) "root::",
                                   "namespace used to set parameters for "
                                   "genome used to encode this brain");

void DEMarkovBrain::readParameters() {
	randomizeUnconnectedOutputs = randomizeUnconnectedOutputsPL->get(PT);
	randomizeUnconnectedOutputsType = randomizeUnconnectedOutputsTypePL->get(PT);
	randomizeUnconnectedOutputsMin = randomizeUnconnectedOutputsMinPL->get(PT);
	randomizeUnconnectedOutputsMax = randomizeUnconnectedOutputsMaxPL->get(PT);
	hiddenNodes = hiddenNodesPL->get(PT);

	genomeName = genomeNamePL->get(PT);

	nrNodes = nrInputValues + nrOutputValues + hiddenNodes;
	nodes.resize(nrNodes, 0);
	nextNodes.resize(nrNodes, 0);
}

DEMarkovBrain::DEMarkovBrain(std::vector<std::shared_ptr<AbstractGate>> _gates,
                         int _nrInNodes, int _nrOutNodes,
                         std::shared_ptr<ParametersTable> PT_)
    : AbstractBrain(_nrInNodes, _nrOutNodes, PT_),
      visualize(Global::modePL->get() == "visualize") {

  readParameters();

	// GLB = nullptr;
	GLB = std::make_shared<ClassicGateListBuilder>(PT);
	gates = _gates;
	// columns to be added to ave file
	popFileColumns.clear();
	popFileColumns.push_back("markovBrainGates");
	for (auto name : GLB->getInUseGateNames()) {
		popFileColumns.push_back("markovBrain" + name + "Gates");
	}

	fillInConnectionsLists();
}

DEMarkovBrain::DEMarkovBrain(std::shared_ptr<AbstractGateListBuilder> GLB_,
                         int _nrInNodes, int _nrOutNodes,
                         std::shared_ptr<ParametersTable> PT_)
    : AbstractBrain(_nrInNodes, _nrOutNodes, PT_),
      visualize(Global::modePL->get() == "visualize") {
	GLB = GLB_;
	// make a node map to handle genome value to brain state address look up.

	readParameters();

	const int genomeFieldSize = Gate_Builder::bitsPerBrainAddressPL->get();
	makeNodeMap(inputNodeMap, genomeFieldSize, 0, nrNodes-1);
	makeNodeMap(outputNodeMap, genomeFieldSize, nrInputValues, nrNodes-1);

	// columns to be added to ave file
	popFileColumns.clear();
	popFileColumns.push_back("markovBrainGates");
	popFileColumns.push_back("markovBrainGates_VAR");
	for (auto name : GLB->getInUseGateNames()) {
		popFileColumns.push_back("markovBrain" + name + "Gates");
	}

	fillInConnectionsLists();

}

DEMarkovBrain::DEMarkovBrain(std::shared_ptr<AbstractGateListBuilder> GLB_,
                         std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes,
                         int _nrInNodes, int _nrOutNodes, std::shared_ptr<ParametersTable> PT_)
  : DEMarkovBrain(GLB_, _nrInNodes, _nrOutNodes, PT_) {
	// cout << "in DEMarkovBrain::DEMarkovBrain(std::shared_ptr<Base_GateListBuilder> GLB_,
	// std::shared_ptr<AbstractGenome> genome, int _nrOfBrainStates)\n\tabout to -
	// gates = GLB->buildGateList(genome, nrOfBrainStates);" << endl;
	gates = GLB->buildGateList(_genomes[genomeName], nrNodes, PT_);
	inOutReMap(); // map ins and outs from genome values to brain states
	fillInConnectionsLists();

	if(visualize) {
		log.open(Global::outputPrefixPL->get() + "markov_log");
		logBrainStructure();
		log.log("\nActivity log:\n");
	}
}

void DEMarkovBrain::logBrainStructure() {
	log.log("begin brain desription\n" + description() + "end brain description\n");
}

// Make a brain like the brain that called this function, using genomes and
// initalizing other elements.
std::shared_ptr<AbstractBrain> DEMarkovBrain::makeBrain(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>>& _genomes) {
  std::shared_ptr<DEMarkovBrain> newBrain = std::make_shared<DEMarkovBrain>(GLB, _genomes, nrInputValues, nrOutputValues, PT);
	return newBrain;
}

void DEMarkovBrain::resetBrain() {
	AbstractBrain::resetBrain();
	nodes.assign(nrNodes, 0.0);
	for (auto &g :gates)
		g->resetGate();

	if(visualize) {
		log.logBrainReset();
		nodesStatesTimeSeries.clear();
	}
}

void DEMarkovBrain::resetInputs() {
	AbstractBrain::resetInputs();
	for (int i = 0; i < nrInputValues; i++)
		nodes[i] = 0.0;

	if(visualize)
		log.log("Inputs were reset\n");
}

void DEMarkovBrain::resetOutputs() {
	AbstractBrain::resetOutputs();
	// note nrInputValues+i gets us the index for the node related to each output
	for (int i = 0; i < nrOutputValues; i++)
		nodes[nrInputValues + i] = 0.0;

	if(visualize)
		log.log("Outputs were reset\n");
}

void DEMarkovBrain::update() {

	nextNodes.assign(nrNodes, 0.0);
	DataMap IOMap;

	for (int i = 0; i < nrInputValues; i++)
		nodes[i] = inputValues[i];

	if(visualize) {
		log.logStateBeforeUpdate(nodes);
		nodesStatesTimeSeries.push_back(nodes);
	}

	if (recordIOMapPL->get())
		for (int i = 0; i < nrInputValues; i++)
			IOMap.append("input", Bit(nodes[i]));

	for (auto &g :gates) // update each gate
		g->update(nodes, nextNodes);

	if (randomizeUnconnectedOutputs) {
		switch (randomizeUnconnectedOutputsType) {
		case 0:
			for (int i = 0; i < nrOutputValues; i++)
				if (nextNodesConnections[nrInputValues + i] == 0)
					nextNodes[nrInputValues + i] =
					   Random::getInt((int)randomizeUnconnectedOutputsMin,
					                  (int)randomizeUnconnectedOutputsMax);
			break;
		case 1:
			for (int i = 0; i < nrOutputValues; i++)
				if (nextNodesConnections[nrInputValues + i] == 0)
					nextNodes[nrInputValues + i] = Random::getDouble(
					    randomizeUnconnectedOutputsMin, randomizeUnconnectedOutputsMax);
			//break;
		//default:
			//std::cout
			//    << "  ERROR! BRAIN_DEMARKOV_ADVANCED::randomizeUnconnectedOutputsType "
			//       "is invalid. current value: "
			//    << randomizeUnconnectedOutputsType << std::endl;
			//exit(1);
		}
	}
	swap(nodes, nextNodes);
	for (int i = 0; i < nrOutputValues; i++) {
		outputValues[i] = nodes[nrInputValues + i];
	}

	if (recordIOMapPL->get()){
		for (int i = 0; i < nrOutputValues; i++ )
			IOMap.append("output", Bit(nodes[nrInputValues + i]));

		for (int i = nrInputValues + nrOutputValues ; i < nodes.size() ; i++)
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

void DEMarkovBrain::inOutReMap() { // remaps genome site values to valid brain
                                 // state addresses
	for (auto &g : gates)
		g->applyNodeMaps(inputNodeMap, outputNodeMap);
}

std::string DEMarkovBrain::description() {
  std::string S = "DEMarkov Brain\nins:" + std::to_string(nrInputValues) + " outs:" +
             std::to_string(nrOutputValues) + " hidden:" + std::to_string(hiddenNodes);
	S += "\nInput IDs:";
	for(int i=0; i<nrInputValues; i++)
		S += " " + std::to_string(i);
	S += "\nOutput IDs:";
	for(int i=nrInputValues; i<nrInputValues+nrOutputValues; i++)
		S += " " + std::to_string(i);
	S += "\nHidden IDs:";
	for(int i=nrInputValues+nrOutputValues; i<nrNodes; i++)
		S += " " + std::to_string(i);
	S += "\n" + gateList();
	return S;
}

void DEMarkovBrain::fillInConnectionsLists() {
	nodesConnections.resize(nrNodes);
	nextNodesConnections.resize(nrNodes);
	for (auto &g : gates) {
		auto gateConnections = g->getConnectionsLists();
		for (auto c : gateConnections.first)
			nodesConnections[c]++;
		for (auto c : gateConnections.second)
			nextNodesConnections[c]++;
	}
}

DataMap DEMarkovBrain::getStats(std::string &prefix) {
	DataMap dataMap;
	dataMap.set(prefix + "markovBrainGates", (int)gates.size());
	std::map<std::string, int> gatecounts;
	for (auto &n : GLB->getInUseGateNames()) {
		gatecounts[n + "Gates"] = 0;
	}
	for (auto &g : gates) {
		gatecounts[g->gateType() + "Gates"]++;
	}

	for (auto &n : GLB->getInUseGateNames()) {
		dataMap.set(prefix + "markovBrain" + n + "Gates", gatecounts[n + "Gates"]);
	}

	std::vector<int> nodesConnectionsList;
	std::vector<int> nextNodesConnectionsList;

	for (int i = 0; i < nrNodes; i++) {
		nodesConnectionsList.push_back(nodesConnections[i]);
		nextNodesConnectionsList.push_back(nextNodesConnections[i]);
	}
	dataMap.set(prefix + "markovBrain_nodesConnections", nodesConnectionsList);
	dataMap.setOutputBehavior(prefix + "nodesConnections", DataMap::LIST);
	dataMap.set(prefix + "markovBrain_nextNodesConnections",
	            nextNodesConnectionsList);
	dataMap.setOutputBehavior(prefix + "nextNodesConnections", DataMap::LIST);

	return dataMap;
}

std::string DEMarkovBrain::gateList() {
	std::string S = "";
	for (auto &g : gates)
		S += g->description();
	return S;
}

std::vector<std::vector<int>> DEMarkovBrain::getConnectivityMatrix() {
  std::vector<std::vector<int>> M(nrNodes,std::vector<int>(nrNodes,0));
	for (auto &g : gates) {
		auto I = g->getIns();
		auto O = g->getOuts();
		for (int i : I)
			for (int o : O)
				M[i][o]++;
	}
	return M;
}

int DEMarkovBrain::brainSize() { return gates.size(); }

int DEMarkovBrain::numGates() { return brainSize(); }

std::vector<int> DEMarkovBrain::getHiddenNodes() {
	std::vector<int> temp ;
	for (size_t i = nrInputValues + nrOutputValues; i < nodes.size(); i++)
		temp.push_back(Bit(nodes[i]));

	return temp;
}

void DEMarkovBrain::initializeGenomes(std::unordered_map<std::string,std::shared_ptr<AbstractGenome>> &_genomes) {
	int codonMax = (1 << Gate_Builder::bitsPerCodonPL->get()) - 1;
	_genomes[genomeName]->fillRandom();

	auto genomeHandler = _genomes[genomeName]->newHandler(_genomes[genomeName]);

	for (auto gateType : GLB->gateBuilder.inUseGateTypes) {
		for (int i = 0; i < GLB->gateBuilder.intialGateCounts[gateType]; i++) {
			genomeHandler->randomize();
			for (auto value : GLB->gateBuilder.gateStartCodes[gateType])
				genomeHandler->writeInt(value, 0, codonMax);
		}
	}
}

std::shared_ptr<AbstractBrain> DEMarkovBrain::makeCopy(std::shared_ptr<ParametersTable> PT_) {
	if (PT_ == nullptr) {
		PT_ = PT;
	}
	std::vector<std::shared_ptr<AbstractGate>> _gates;
	for (auto gate : gates) {
		_gates.push_back(gate->makeCopy());
	}
	auto newBrain = std::make_shared<DEMarkovBrain>(_gates, nrInputValues, nrOutputValues, PT_);
	return newBrain;
}

std::vector<std::shared_ptr<AbstractBrain>> DEMarkovBrain::getAllSingleGateKnockouts() {
	std::vector<std::shared_ptr<AbstractBrain>> res;
	auto numg = gates.size();
	if (!numg) std::cout <<"No gates?" << std::endl;
	for (int i = 0; i < numg; i++) {
		std::vector<std::shared_ptr<AbstractGate>> gmut;
		int c = 0;
		for (auto g : gates)
			if (c++ != i)
				gmut.push_back(g->makeCopy());
		auto bmut =
				std::make_shared<DEMarkovBrain>(gmut, nrInputValues, nrOutputValues, PT);
		res.push_back(bmut);
	}
	return res;
}

void* DEMarkovBrain::logTimeSeries(const std::string& label) {

	const unsigned m = 4;

	if(nodesStatesTimeSeries[0].size()!=nrNodes) {
		std::cerr << "Wrong number of nodes in the saved time series! Exiting..." << std::endl;
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

	eegImage.write(std::string("markovBrainStates_") + label);

	log.log(std::string("States for ") + label + std::string(" are written into an image\n"));

	return nullptr;
}
