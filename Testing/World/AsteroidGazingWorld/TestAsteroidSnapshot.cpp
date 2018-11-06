#include "Utilities/AsteroidSnapshot.h"

// g++ -std=c++14 -lpng TestAsteroidSnapshot.cpp Utilities/AsteroidSnapshot.cpp

int main(int argc, char** argv) {

	auto ss1 = AsteroidSnapshot("/storage/asteroidDatasets/syntheticSphericalHarmoics0/asteroid1/condition0_distance125_phase0001.png");
	auto ss2 = AsteroidSnapshot("/storage/asteroidDatasets/syntheticSphericalHarmoics0/asteroid0/condition0_distance62_phase0004.png");

	// Single pixel tests - primarily for making sure the bounds are enforced properly
//	std::cout << unsigned(ss1.get(50,50)) << std::endl;
//	std::cout << unsigned(302, 20) << std::endl;

	unsigned newres = 7;
	auto ss3 = ss1.resampleArea(0, 0, 300, 300, newres, newres);
	for(unsigned i=0; i<newres; i++) {
		for(unsigned j=0; j<newres; j++)
			std::cout << unsigned(ss3.get(i,j)) << '\t';
		std::cout << std::endl;
	}

	std::cout << std::endl;
	unsigned newestres = 10;
	auto ss4 = ss3.resampleArea(0, 0, newres, newres, newestres, newestres);
	for(unsigned i=0; i<newestres; i++) {
		for(unsigned j=0; j<newestres; j++)
			std::cout << unsigned(ss4.get(i,j)) << '\t';
		std::cout << std::endl;
	}

	return 0;
}
