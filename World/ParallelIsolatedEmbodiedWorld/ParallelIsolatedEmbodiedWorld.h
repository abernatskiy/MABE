#pragma once

#include <vector>

#include "../AbstractWorld.h"
#include "../AbstractIsolatedEmbodiedWorld.h"

class ParallelIsolatedEmbodiedWorld : public AbstractWorld {

public:
//  static std::shared_ptr<ParameterLink<int>> modePL;
//  static std::shared_ptr<ParameterLink<int>> numberOfOutputsPL;
//  static std::shared_ptr<ParameterLink<int>> evaluationsPerGenerationPL;
//  static std::shared_ptr<ParameterLink<std::string>> brainNamePL;

	static std::shared_ptr<ParameterLink<std::string>> groupNamePL; std::string groupName;
	static std::shared_ptr<ParameterLink<int>> numThreadsPL; unsigned numThreads;

	std::vector<std::unique_ptr<AbstractIsolatedEmbodiedWorld>> subworlds;

	ParallelIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_);
	virtual ~ParallelIsolatedEmbodiedWorld() = default;

	void evaluate(std::map<std::string, std::shared_ptr<Group>>& groups, int analyze, int visualize, int debug) override;

	std::unordered_map<std::string, std::unordered_set<std::string>> requiredGroups() override { return subworlds[0]->requiredGroups(); };
};

