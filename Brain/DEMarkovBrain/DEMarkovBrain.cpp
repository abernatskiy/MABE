#include <sstream>
#include <iomanip>
#include <png++/png.hpp>

#include "../../Utilities/nlohmann/json.hpp" // https://github.com/nlohmann/json/ v3.6.1

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
shared_ptr<ParameterLink<double>> DEMarkovBrain::gateInsertionProbabilityPL = Parameters::register_parameter("BRAIN_DEMARKOV-gateInsertionProbability", 0.1,
                                                                                                             "probability that a new random gate will be inserted into the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DEMarkovBrain::gateDeletionProbabilityPL = Parameters::register_parameter("BRAIN_DEMARKOV-gateDeletionProbability", 0.1,
                                                                                                            "probability that a randomly chosen gate will be deleted from the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DEMarkovBrain::gateDuplicationProbabilityPL = Parameters::register_parameter("BRAIN_DEMARKOV-gateDuplicationProbability", 0.2,
                                                                                                               "probability that a randomly chosen gate will be duplicated within the brain upon the mutation() call");
shared_ptr<ParameterLink<double>> DEMarkovBrain::connectionToTableChangeRatioPL = Parameters::register_parameter("BRAIN_DEMARKOV-connectionToTableChangeRatio", 1.0,
                                                                                                                 "if the brain experiences no insertion, deletions or duplictions, call to mutate() changes parameters of a randomly chosen gate. This parameters governs relative frequencies of such changes to connections and logic tables");
shared_ptr<ParameterLink<int>> DEMarkovBrain::minGateCountPL = Parameters::register_parameter("BRAIN_DEMARKOV-minGateCount", 0, "number of gates that causes gate deletions to become impossible (mutation operator calls itself if the mutation happens to be a deletion)");
shared_ptr<ParameterLink<bool>> DEMarkovBrain::readFromInputsOnlyPL = Parameters::register_parameter("BRAIN_DEMARKOV-readFromInputsOnly", false,
                                                                                              "if set to true, gates will only read from input nodes; by default, they'll read from all nodes");
shared_ptr<ParameterLink<double>> DEMarkovBrain::structurewideMutationProbabilityPL = Parameters::register_parameter("BRAIN_DEMARKOV-structurewideMutationProbability", 0.0,
                                                                                                                 "probability of mutation of each brain component upon reproduction");
/********** Public definitions **********/

DEMarkovBrain::DEMarkovBrain(int _nrInNodes, int _nrHidNodes, int _nrOutNodes, shared_ptr<ParametersTable> PT_) :
	AbstractBrain(_nrInNodes, _nrOutNodes, PT_),
	nrHiddenNodes(_nrHidNodes),
	nrNodes(_nrInNodes+_nrOutNodes+_nrHidNodes),
	minGates(minGateCountPL->get(PT_)),
	readFromInputsOnly(readFromInputsOnlyPL->get(PT_)),
	visualize(Global::modePL->get() == "visualize"),
	recordIOMap(recordIOMapPL->get(PT_)),
	originationStory("primordial"),
	structurewideMutationProbability(structurewideMutationProbabilityPL->get(PT_)) {

	gateInsStart = 0;
	gateInsEnd = readFromInputsOnly ? nrInputValues-1 : nrNodes-1;
	gateOutsStart = nrInputValues;
	gateOutsEnd = nrNodes - 1;

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

void DEMarkovBrain::update(std::mt19937* worldRNG) {
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
//	std::cout << "mutate() called on brain " << this << std::endl;

	mainMutate();
	if(structurewideMutationProbability!=0)
		mutateStructurewide();
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
	dataMap.set(prefix + "originationStory", originationStory);
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

	eegImage.write(string("markovBrainStates_") + label + ".png");

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

void DEMarkovBrain::mainMutate() {
	double r = Random::getDouble(1.);
//	cout << "DEMarkovBrain " << this << " is mutating. Rolled " << r;
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
			else {
//				cerr << "DEMarkovBrain: mutation() that cannot insert gates called on a zero gates Markov brain, exiting" << endl;
				exit(EXIT_FAILURE);
			}
			return;
		}

		if(r < gateInsertionProbabilityPL->get(PT) + gateDeletionProbabilityPL->get(PT)) {
			if(gates.size()<=minGates) {
				mainMutate();
				return;
			}
//			cout << ", chose deletion. Had " << gates.size() << " gates, ";
			int idx = Random::getIndex(gates.size());
			gates.erase(gates.begin()+idx);
//			cout << ", removed " << idx << "th one, now it's " << gates.size() << endl;
			originationStory = "mutation_deletion";
		}
		else if(r < gateInsertionProbabilityPL->get(PT) + gateDeletionProbabilityPL->get(PT) + gateDuplicationProbabilityPL->get(PT)) {
//			cout << ", chose duplication. Had " << gates.size() << " gates, ";
			int idx = Random::getIndex(gates.size());
			auto newGate = gates[idx]->makeCopy();
			newGate->ID = getLowestAvailableGateID();
			gates.push_back(newGate);
//			cout << ", duplicated " << idx << "th one, now it's " << gates.size() << endl;
			originationStory = "mutation_duplication";
		}
		else {
			int idx = Random::getIndex(gates.size());
//			cout << ", chose intra-gate mutation. Chose gate " << idx;
			double spentProb = gateInsertionProbabilityPL->get(PT) + gateDeletionProbabilityPL->get(PT) + gateDuplicationProbabilityPL->get(PT);
			double tableChangeThr = spentProb + (1.-spentProb)/(1.+connectionToTableChangeRatioPL->get(PT));
			if(r < tableChangeThr) {
//				cout << " and a table mutation. Gate before the mutation:" << endl << gates[idx]->description() << endl;
				gates[idx]->mutateInternalStructure();
//				cout << "Gate after the mutation:" << endl << gates[idx]->description() << endl;
				originationStory = "mutation_table";
			}
			else {
//				cout << " and a wiring mutation. Gate before the mutation:" << endl << gates[idx]->description();
				gates[idx]->mutateConnections(gateInsStart, gateInsEnd, gateOutsStart, gateOutsEnd);
//				cout << "Gate after the mutation:" << endl << gates[idx]->description() << endl;
				originationStory = "mutation_rewiring";
			}
		}
	}
}

void DEMarkovBrain::mutateStructurewide() {
//	unsigned internalChanges = 0;
//	unsigned connectionsChanges = 0;

//	cout << "Gates before the structurewide mutation:" << std::endl;
//	for(const auto& gate : gates)
//		cout << gate->description() << endl;

	for(auto& gate : gates) {
		if(Random::getDouble(1.)<structurewideMutationProbability) {
			gate->mutateInternalStructure();
//			internalChanges++;
		}
		if(Random::getDouble(1.)<structurewideMutationProbability) {
			gate->mutateConnections(gateInsStart, gateInsEnd, gateOutsStart, gateOutsEnd);
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
		conns.first.push_back(Random::getInt(gateInsStart, gateInsEnd));
	for(unsigned j=0; j<nouts; j++)
		conns.second.push_back(Random::getInt(gateOutsStart, gateOutsEnd));

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

int DEMarkovBrain::getLowestAvailableGateID() {
	int newGateID;
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

DataMap DEMarkovBrain::serialize(string& name) {
	nlohmann::json gatesJSON = nlohmann::json::array();
	for(const auto& gate : gates) {
		nlohmann::json gateJSON = nlohmann::json::object();

		gateJSON["id"] = gate->ID;
		gateJSON["type"] = gate->gateType();
		gateJSON["inputs"] = nlohmann::json::array();
		for(const auto& in : gate->getConnectionsLists().first)
			gateJSON["inputs"].push_back(in);
		gateJSON["outputs"] = nlohmann::json::array();
		for(const auto& out : gate->getConnectionsLists().second)
			gateJSON["outputs"].push_back(out);

		if(gateJSON["type"] == "Deterministic") {
			gateJSON["table"] = nlohmann::json::array();
			vector<vector<int>> gtable = dynamic_pointer_cast<DeterministicGate>(gate)->table;
			for(const auto& row : gtable) {
				gateJSON["table"].push_back(nlohmann::json::array());
				for(const auto& columnVal : row)
					gateJSON["table"].back().push_back(columnVal);
			}
		}
		else {
			cerr << "DEMarkovBrain::serialize caught an unsupported gate type " << gateJSON["type"] << endl;
			exit(EXIT_FAILURE);
		}
		gatesJSON.push_back(gateJSON);
	}

	nlohmann::json brainJSON = nlohmann::json::object();
	brainJSON["gates"] = gatesJSON;
	brainJSON["numInputs"] = nrInputValues;
	brainJSON["numOutputs"] = nrOutputValues;
	brainJSON["numHidden"] = nrHiddenNodes;
	brainJSON["originationStory"] = originationStory;

	// storing the string at the DataMap and returning it
	DataMap dm;
	dm.set(name + "_json", string("'") + brainJSON.dump() + "'");
	return dm;
}

void DEMarkovBrain::deserialize(shared_ptr<ParametersTable> PT, unordered_map<string,string>& orgData, string& name) {
	// the stuff that can be loaded from config files is already loaded during construction,
	// so all that's left is to initialize gates
	// AND restore the saved origination story

	string gatesJSONStr = orgData[string("BRAIN_") + name + "_json"];
	if(gatesJSONStr.front()!='\'' || gatesJSONStr.back()!='\'') {
		cout << "First or last character of the DEMarkovBrain JSON string field in the log is not a single quote" << endl;
		cout << "The string: " << gatesJSONStr << endl;
		exit(EXIT_FAILURE);
	}
	gatesJSONStr = gatesJSONStr.substr(1, gatesJSONStr.size()-2);

	nlohmann::json brainJSON = nlohmann::json::parse(gatesJSONStr);

	if(brainJSON["numInputs"]!=nrInputValues) {
		cout << "DEMarkovBrain.deserialize: mismatch between the native size of the brain input (" << nrInputValues << ") and the size of the deserialized brain input (" << brainJSON["numInputs"] << ")" << endl;
		exit(EXIT_FAILURE);
	}

	if(brainJSON["numHidden"]!=nrHiddenNodes) {
		cout << "DEMarkovBrain.deserialize: mismatch between the native size of the brain hidden layer (" << nrHiddenNodes << ") and the size of the deserialized brain hidden layer (" << brainJSON["numHidden"] << ")" << endl;
		exit(EXIT_FAILURE);
	}

	if(brainJSON["numOutputs"]!=nrOutputValues) {
		cout << "DEMarkovBrain.deserialize: mismatch between the native size of the brain output (" << nrOutputValues << ") and the size of the deserialized brain output (" << brainJSON["numOutputs"] << ")" << endl;
		exit(EXIT_FAILURE);
	}

	originationStory = brainJSON["originationStory"];

	nlohmann::json gatesJSON = brainJSON["gates"];
	gates.clear();

	for(const auto& gateJSON : gatesJSON) {
		if(gateJSON["type"] != "Deterministic") {
			cerr << "DEMarkovBrain::deserialize: Unsupported gate type " << gateJSON["type"] << endl;
			exit(EXIT_FAILURE);
		}

		int id = gateJSON["id"];

		pair<vector<int>,vector<int>> addresses;
		for(const auto& in : gateJSON["inputs"])
			addresses.first.push_back(in);
		for(const auto& out : gateJSON["outputs"])
			addresses.second.push_back(out);

		vector<vector<int>> table;
		for(const auto& row : gateJSON["table"]) {
			table.push_back({});
			for(const auto& val : row)
				table.back().push_back(val);
		}

		gates.push_back(make_shared<DeterministicGate>(addresses, table, id));
	}
}
