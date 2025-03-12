#pragma once
#include <iostream>
#include <memory>
#include "Telegram.h"
#include "AstarAlgorithm.h"
#include "PlayerMap.h"


// Forward declare State

template <class entityType>
class State
{
public:
	virtual void handle(std::shared_ptr<entityType> owner, std::shared_ptr<PlayerMap>& pMap, std::shared_ptr<AstartAlgorithm>& astarAlgo, float deltaTime) = 0;
	virtual void enterState(std::shared_ptr<entityType> owner, std::shared_ptr<PlayerMap>& pMap, std::shared_ptr<AstartAlgorithm>& astarAlgo, float deltaTime) = 0;
	virtual void exitState(std::shared_ptr<entityType> owner, std::shared_ptr<PlayerMap>& pMap, std::shared_ptr<AstartAlgorithm>& astarAlgo, float deltaTime) = 0;

	virtual bool onMessage(std::shared_ptr<entityType> owner, Telegram& telegram) = 0;
};