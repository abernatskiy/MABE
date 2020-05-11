#include <climits>

#include "ParallelIsolatedEmbodiedWorld.h"

// #include "../AsteroidTeamGazingWorld/AsteroidTeamGazingWorld.h"
#include "../AsteroidGazingWorld/AsteroidGazingWorld.h"
#include "../AsteroidTextureGazingWorld/AsteroidTextureGazingWorld.h"

#include "../../Utilities/Random.h"

/***** Auxiliary functions *****/

void recBatchSize(unsigned numEvals, unsigned numBatches, std::vector<unsigned>::iterator pos) {
	if(numEvals%numBatches==0)
		std::fill(pos, pos+numBatches, numEvals/numBatches);
	else {
		*pos = 1 + numEvals/numBatches;
		recBatchSize(numEvals-(*pos), numBatches-1, pos+1);
	}
}

std::vector<unsigned> getBatchSizes(unsigned numEvals, unsigned numBatches) {
	std::vector<unsigned> batchSizes(numBatches);
	recBatchSize(numEvals, numBatches, batchSizes.begin());
	return batchSizes;
}

unsigned estimateActualEvaluationCount(std::vector<std::shared_ptr<Organism>>& population, bool assumeDeterministicEvaluations) {
	unsigned numEvals = 0;
	for(auto& org : population) {
		if(assumeDeterministicEvaluations && org->dataMap.findKeyInData("evaluated")==11 && org->dataMap.getBool("evaluated"))
			continue;
		numEvals++;
	}
	return numEvals;
}

std::vector<unsigned> getBatchStarts(unsigned numBatches, const std::vector<unsigned>& batchSizes, std::vector<std::shared_ptr<Organism>>& population, bool assumeDeterministicEvaluations) {
	std::vector<unsigned> batchStarts(numBatches);
	unsigned curOrgPos = 0;
	for(unsigned b=0; b<numBatches; b++) {
		batchStarts[b] = curOrgPos;
		unsigned evalsInCurBatch = 0;
		while(evalsInCurBatch<batchSizes[b]) {
			std::shared_ptr<Organism> org = population[curOrgPos];
			curOrgPos++;
			if(assumeDeterministicEvaluations && org->dataMap.findKeyInData("evaluated")==11 && org->dataMap.getBool("evaluated"))
				continue;
			else
				evalsInCurBatch++;
		}
	}
	return batchStarts;
}

/***** Parallel isolated embodied world class definitions *****/

std::shared_ptr<ParameterLink<std::string>> ParallelIsolatedEmbodiedWorld::groupNamePL = Parameters::register_parameter("WORLD_PARALLEL_ISOLATED_EMBODIED-groupNameSpace", (std::string) "root::", "namespace of group to be evaluated");
std::shared_ptr<ParameterLink<int>> ParallelIsolatedEmbodiedWorld::numThreadsPL = Parameters::register_parameter("WORLD_PARALLEL_ISOLATED_EMBODIED-numThreads", 8, "number of threads");

ParallelIsolatedEmbodiedWorld::ParallelIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_)
	: AbstractWorld(PT_), numThreads(numThreadsPL->get(PT_)), groupName(groupNamePL->get(PT_)) {

	if(numThreads==0) {
		std::cerr << "Number of threads must be greater than zero" << std::endl;
		exit(EXIT_FAILURE);
	}

	for(unsigned i=0; i<numThreads; i++) {
		// subworlds.push_back(std::make_unique<AsteroidTeamGazingWorld>(PT_)); // setting the daughter class manually for now
		// subworlds.push_back(std::make_unique<AsteroidGazingWorld>(PT_)); // setting the daughter class manually for now
		subworlds.push_back(std::make_unique<AsteroidTextureGazingWorld>(PT_)); // setting the daughter class manually for now
	}
}

void ParallelIsolatedEmbodiedWorld::evaluate(std::map<std::string, std::shared_ptr<Group>>& groups, int analyze, int visualize, int debug) {

	unsigned numEvals = estimateActualEvaluationCount(groups[groupNamePL->get(PT)]->population, subworlds[0]->assumesDeterministicEvaluations());

	if(numEvals<numThreads)
		std::cout << "ParalleIsolatedEmbodiedWorld: WARNING: number of evaluations " << numEvals << " is less than the number of threads " << numThreads << std::endl;

	std::vector<unsigned> batchSizes = getBatchSizes(numEvals, numThreads);
	std::vector<unsigned> batchStarts = getBatchStarts(numThreads, batchSizes, groups[groupNamePL->get(PT)]->population, subworlds[0]->assumesDeterministicEvaluations());

//	std::cout << "Distributed " << numEvals << " evaluations of a population of size " << groups[groupNamePL->get(PT)]->population.size() << " over " << numThreads << " batches. Ranges:" << std::endl;
//	for(unsigned b=0; b<numThreads; b++) {
//		unsigned batchEnd = b==numThreads-1 ? groups[groupNamePL->get(PT)]->population.size() : batchStarts[b+1];
//		std::cout << "batch " << b << " begins at " << batchStarts[b] << " and contains " << batchSizes[b] << " evaluations (ends at " << batchEnd << ")" << std::endl;
//	}

	#pragma omp parallel num_threads(numThreads)
	{
		#pragma omp for
		for(unsigned t=0; t<numThreads; t++) {
			unsigned batchStart = batchStarts[t];
			unsigned batchEnd = t==numThreads-1 ? groups[groupNamePL->get(PT)]->population.size() : batchStarts[t+1];
			for(unsigned i=batchStart; i<batchEnd	; i++) {
//				std::cout << "Batch " << t << " evaluating indiv " << i << std::endl << std::flush;
				subworlds[t]->evaluateSolo(groups[groupName]->population[i], analyze, visualize, debug);
			}
		}
	}
}
