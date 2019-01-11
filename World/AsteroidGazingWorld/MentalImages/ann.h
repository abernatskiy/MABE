#pragma once

#include <vector>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <iomanip>
#include <utility>
#include <iterator>

#include "constants.h"
#include "strutil.h"

#ifndef ANN_TRANSFER
#define ANN_TRANSFER(x) (x<0. ? 0. : x) // ReLU!
#endif // ANN_TRANSFER

typedef double State;
typedef double Weight;

typedef Weight** WeightMatrix;
//#include "boost/multi_array.hpp" // VERY useful for debug
//typedef boost::multi_array<Weight,2> WeightMatrix;

class ArtificialNeuralNetwork {
private:
	std::vector<std::vector<State>> layerStates;

public:
	const std::vector<unsigned> layerSizes;
	std::vector<WeightMatrix> weights;

	ArtificialNeuralNetwork(const std::vector<unsigned>& lSizes) : layerSizes(lSizes) {
		// constructor from layer sizes only - weights initialized to zero
		layerStates.emplace_back(layerSizes.front(), 0.);
		for(auto lsit=layerSizes.begin(); lsit!=(layerSizes.end()-1); lsit++) {
			unsigned inSize = (*lsit)+1;
			unsigned outSize = *(lsit+1);
			layerStates.emplace_back(outSize, 0.);

			WeightMatrix curWeights = new Weight*[inSize];
			for(unsigned i=0; i<inSize; i++)
				curWeights[i] = new Weight[outSize]();
			weights.push_back(curWeights);
//			weights.emplace_back(boost::extents[inSize][outSize]);
		}
	};

	///// Functions required for native arrays begin /////

	ArtificialNeuralNetwork(const ArtificialNeuralNetwork& other) : ArtificialNeuralNetwork(other.layerSizes) {
		for(unsigned l=0; l<(layerSizes.size()-1); l++)
			for(unsigned i=0; i<layerSizes[l]+1; i++)
				for(unsigned j=0; j<layerSizes[l+1]; j++)
					weights[l][i][j] = other.weights[l][i][j];
	};

	ArtificialNeuralNetwork(ArtificialNeuralNetwork&& other) : layerSizes(other.layerSizes), layerStates(other.layerStates) {
		for(unsigned l=0; l<(layerSizes.size()-1); l++)
			for(unsigned i=0; i<layerSizes[l]+1; i++)
					weights[l][i] = other.weights[l][i];
	};

	~ArtificialNeuralNetwork() {
		for(unsigned l=0; l<(layerSizes.size()-1); l++) {
			unsigned inSize = layerSizes[l]+1;
			for(unsigned i=0; i<inSize; i++)
				delete [] weights[l][i];
			delete [] weights[l];
		}
	};

	///// Functions required for native arrays end /////

	std::vector<State> forward(std::vector<State> in) {

		D( if(in.size()!=layerSizes.front()) { std::cerr << "ANN supplied with too many inputs (expected " << layerSizes.front() << ", got " << in.size() << ")" << std::endl; exit(EXIT_FAILURE); } )

		layerStates[0] = in;

		for(unsigned l=1; l<layerSizes.size(); l++) {
			for(unsigned j=0; j<layerSizes[l]; j++) {
				layerStates[l][j] = 0.;
				for(unsigned i=0; i<layerSizes[l-1]; i++)
					layerStates[l][j] += weights[l-1][i][j]*layerStates[l-1][i];
				layerStates[l][j] += weights[l-1][layerSizes[l-1]][j]; // bias
				layerStates[l][j] = ANN_TRANSFER(layerStates[l][j]);
			}
		}

		std::vector<State> out = layerStates.back();
		return out;
	};

	void drawWeights() const {
		std::stringstream s;
		s << std::scientific << std::setprecision(1);
//		s << std::fixed << std::setprecision(2);
		for(unsigned l=1; l<layerSizes.size(); l++) {
			for(unsigned j=0; j<layerSizes[l]; j++) {
				for(unsigned i=0; i<layerSizes[l-1]+1; i++)
					s << (weights[l-1][i][j]>=0 ? " " : "") <<  weights[l-1][i][j] << " ";
				s << std::endl;
			}
			s << std::endl;
		}
		std::cout << s.str();
	};

	std::string compactStr() const {
		std::stringstream s;
		s << std::hexfloat;
		for(unsigned l=0; l<layerSizes.size()-1; l++) {
			for(unsigned j=0; j<layerSizes[l+1]; j++) {
				for(unsigned i=0; i<(layerSizes[l]+1); i++)
					s << weights[l][i][j] << ( i==layerSizes[l] ? "" : "," );
				s << ( j==layerSizes[l+1]-1 ? "" : ";" );
			}
			s << ( l==layerSizes.size()-2 ? "" : "|" );
		}
		return s.str();
	};

	void loadCompactStr(std::string compactString) {
		// load a string produced by method compactStr()
		// WARNING: this method WILL segfault if the layer sizes are wrong!
		std::istringstream layerStream(compactString);
		unsigned l = 0;
		unsigned i = 0;
		unsigned j = 0;
		for(auto lsit=std::istream_iterator<WordDelimitedByPipe>(layerStream); lsit!=std::istream_iterator<WordDelimitedByPipe>(); lsit++) {
			std::istringstream rowStream(*lsit);
			for(auto rsit=std::istream_iterator<WordDelimitedBySemicolon>(rowStream); rsit!=std::istream_iterator<WordDelimitedBySemicolon>(); rsit++) {
				std::istringstream columnStream(*rsit);
				for(auto csit=std::istream_iterator<WordDelimitedByComma>(columnStream); csit!=std::istream_iterator<WordDelimitedByComma>(); csit++) {
//					std::cout << " " << *csit << "(" << std::stod(*csit) << "l" << l << ",i" << i << ",j" << j << ")";
					weights[l][i][j] = std::stod(*csit);
					i++;
				}
				i=0; j++;
//				std::cout << std::endl;
			}
			j=0; l++;
//			std::cout << std::endl;
		}
	};

	void reset(State defaultWeight = 0.) {
		for(unsigned l=0; l<layerSizes.size()-1; l++)
			for(unsigned i=0; i<layerSizes[l]+1; i++)
				for(unsigned j=0; j<layerSizes[l+1]; j++)
					weights[l][i][j] = defaultWeight;
	};

	long unsigned countConnections(unsigned layer) {
		long unsigned out = 0;
		for(unsigned i=0; i<layerSizes[layer]+1; i++)
			for(unsigned j=0; j<layerSizes[layer+1]; j++)
				if(weights[layer][i][j]!=0.) out++;
		return out;
	};

	long unsigned countDisconnections(unsigned layer) {
		return (layerSizes[layer]+1)*layerSizes[layer+1] - countConnections(layer);
	};

	std::vector<std::pair<unsigned,unsigned>> allConnectionsIndices(unsigned layer) {
		std::vector<std::pair<unsigned,unsigned>> allIndices;
		for(unsigned i=0; i<layerSizes[layer]+1; i++)
			for(unsigned j=0; j<layerSizes[layer+1]; j++)
				if(weights[layer][i][j]!=0.) allIndices.push_back(std::make_pair(i, j));
		return allIndices;
	};

	std::vector<std::pair<unsigned,unsigned>> allDisconnectionsIndices(unsigned layer) {
		std::vector<std::pair<unsigned,unsigned>> allIndices;
		for(unsigned i=0; i<layerSizes[layer]+1; i++)
			for(unsigned j=0; j<layerSizes[layer+1]; j++)
				if(weights[layer][i][j]==0.) allIndices.push_back(std::make_pair(i, j));
		return allIndices;
	};
};
