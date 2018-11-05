// g++ -lboost_filesystem -lboost_system -std=c++14 TestDatasetParser.cpp Utilities/AsteroidsDatasetParser.cpp

#include <iostream>

#include "Utilities/AsteroidsDatasetParser.h"

using namespace std;

int main(int argc, char** argv) {

	AsteroidsDatasetParser adp("/storage/asteroidDatasets/syntheticSphericalHarmoics0");

	auto astNames = adp.getAsteroidsNames();
	cout << "Asteroid names:" << endl;
	for(auto it=astNames.begin(); it!=astNames.end(); it++)
		cout << *it << endl;
	cout << endl << flush;

	std::string exampleAsteroidName = *astNames.begin();

	auto icqPath = adp.getICQPath(exampleAsteroidName);
	cout << "Path to ICQ of " << exampleAsteroidName << " is " << icqPath << endl << endl << flush;

	auto picturePaths = adp.getAllPicturePaths(exampleAsteroidName);
	cout << "Available pictures of " << exampleAsteroidName << ":" << endl;
	for(auto it=picturePaths.begin(); it!=picturePaths.end(); it++)
		cout << *it << endl;
	cout << endl << flush;

	std::string samplePicPath = adp.getPicturePath(exampleAsteroidName, 0, 125, 3);
	cout << "Sample picture path retrieved by name and parameter values: "<< samplePicPath << endl << endl << flush;

	auto parameterValues = adp.getAllParameterValues(exampleAsteroidName);
	cout << "Available parameter values for " << exampleAsteroidName << ":" << endl;
	for(auto it=parameterValues.begin(); it!=parameterValues.end(); it++) {
		cout << it->first << ":";
		for(auto vit=(it->second).begin(); vit!=(it->second).end(); vit++)
			cout << " " << *vit;
		cout << endl;
	}

	return 0;
}
