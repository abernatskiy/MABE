#include "ParallelIsolatedEmbodiedWorld.h"

#include "../AsteroidGazingWorld/AsteroidGazingWorld.h"

/*
std::shared_ptr<ParameterLink<int>> TestWorld::modePL =
    Parameters::register_parameter(
        "WORLD_TEST-mode", 0, "0 = bit outputs before adding, 1 = add outputs");
std::shared_ptr<ParameterLink<int>> TestWorld::numberOfOutputsPL =
    Parameters::register_parameter("WORLD_TEST-numberOfOutputs", 10,
                                   "number of outputs in this world");
std::shared_ptr<ParameterLink<int>> TestWorld::evaluationsPerGenerationPL =
    Parameters::register_parameter("WORLD_TEST-evaluationsPerGeneration", 1,
                                   "Number of times to test each Genome per "
                                   "generation (useful with non-deterministic "
                                   "brains)");
std::shared_ptr<ParameterLink<std::string>> TestWorld::brainNamePL =
    Parameters::register_parameter(
        "WORLD_TEST_NAMES-brainNameSpace", (std::string) "root::",
        "namespace for parameters used to define brain");
*/

std::shared_ptr<ParameterLink<std::string>> ParallelIsolatedEmbodiedWorld::groupNamePL = Parameters::register_parameter("WORLD_PARALLEL_ISOLATED_EMBODIED-groupNameSpace", (std::string) "root::", "namespace of group to be evaluated");
std::shared_ptr<ParameterLink<int>> ParallelIsolatedEmbodiedWorld::numThreadsPL = Parameters::register_parameter("WORLD_PARALLEL_ISOLATED_EMBODIED-numThreads", 8, "number of threads");

ParallelIsolatedEmbodiedWorld::ParallelIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_)
	: AbstractWorld(PT_), numThreads(numThreadsPL->get(PT_)), groupName(groupNamePL->get(PT_)) {

	if(numThreads==0) {
		std::cerr << "Number of threads must be greater than zero" << std::endl;
		exit(EXIT_FAILURE);
	}

	for(unsigned i=0; i<numThreads; i++) {
		subworlds.push_back(std::make_unique<AsteroidGazingWorld>(PT_)); // setting the daughter class manually for now
	}
}

void ParallelIsolatedEmbodiedWorld::evaluate(std::map<std::string, std::shared_ptr<Group>>& groups, int analyze, int visualize, int debug) {

	unsigned popSize = groups[groupNamePL->get(PT)]->population.size();
	unsigned maxBatchSize = popSize%numThreads==0 ? popSize/numThreads : (1+popSize/numThreads);
	std::vector<unsigned> batchSizes(numThreads, maxBatchSize);
	if(popSize%numThreads!=0) batchSizes.back() = popSize % maxBatchSize;

	#pragma omp parallel num_threads(numThreads)
	{
		#pragma omp for
		for(unsigned t=0; t<numThreads; t++) {
			for(unsigned i=t*maxBatchSize; i<(t*maxBatchSize+batchSizes[t]); i++) {
				subworlds[t]->evaluateSolo(groups[groupName]->population[i], analyze, visualize, debug);
			}
		}
	}
}
