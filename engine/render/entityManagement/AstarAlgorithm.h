#pragma once
#include <unordered_set>
#include <queue>
#include "world.h"

struct CompareGameObjectX
{
    bool operator()(Entity* a, Entity* b); // min-heap behavior
};


// Forward declaration of classes
class Entity;

class AstartAlgorithm
{
    static AstartAlgorithm* _instance;
    bool containsInOpenList(Entity* gameobject);
public:
    // storage
    std::priority_queue<Entity*, std::vector<Entity*>, CompareGameObjectX> openList;
    std::unordered_set<Entity*> closedList;
    std::unordered_set<Entity*> openSet;

    //singleton instance
    static AstartAlgorithm* Instance();
    static void destroy();

    // Methods
    std::vector<Entity*> findPath(Entity* start, Entity* end, World* map);
    int getDistance(Entity* objectA, Entity* objectB);
    std::vector<Entity*> retracePath(Entity* startObject, Entity* endObject);
};



inline bool CompareGameObjectX::operator()(Entity* a, Entity* b)
{
    return a->Fcost > b->Fcost; // Lower fCost = higher priority
}

inline bool AstartAlgorithm::containsInOpenList(Entity* gameobject)
{
    return openSet.find(gameobject) != openSet.end();
}

inline AstartAlgorithm* AstartAlgorithm::Instance()
{
    if (!_instance)
    {
        _instance = new  AstartAlgorithm();
    }
    return _instance;
}

inline void AstartAlgorithm::destroy()
{
    if (_instance)
    {
        delete _instance;
        _instance = nullptr;
    }
}

inline std::vector<Entity*> AstartAlgorithm::findPath(Entity* start, Entity* end, World* map)
{
    openList = {};
    openSet.clear();
    closedList.clear();

    openList.push(start);
    openSet.insert(start);
    Entity* current = nullptr;

    while (!openList.empty())
    {
        current = openList.top();
        openList.pop();
        openSet.erase(current);

        closedList.insert(current);

        if (current->id == end->id)
        {
            return retracePath(start, end);
        }

        for (Entity* neighbor : map->getNeighbors(current))
        {

            if (closedList.contains(neighbor))
                continue;

            int newMovementCostToNeighbor = current->gCost + getDistance(current, neighbor);

            if (newMovementCostToNeighbor < current->gCost || !containsInOpenList(neighbor))
            {
                neighbor->gCost = newMovementCostToNeighbor;
                neighbor->hCost = getDistance(neighbor, end);

                neighbor->parentNode = current;

                if (!containsInOpenList(neighbor))
                {
                    openList.push(neighbor);
                    openSet.insert(neighbor);
                }
            }
        }
    }

    return retracePath(start, current);
}

inline int AstartAlgorithm::getDistance(Entity* objectA, Entity* objectB)
{
    auto transFormComponentA = objectA->GetComponent< Components::TransformComponent>();
    auto transFormComponentB = objectB->GetComponent< Components::TransformComponent>();


    int distX = abs(transFormComponentA->transform[3].x - transFormComponentB->transform[3].x);
    int distY = abs(transFormComponentA->transform[3].y - transFormComponentB->transform[3].y);
    int distZ = abs(transFormComponentA->transform[3].z - transFormComponentB->transform[3].z);

    int minDist = std::min({ distX, distY, distZ });
    int midDist = std::max(std::min(distX, distY), std::min(std::max(distX, distY), distZ));
    int maxDist = std::max({ distX, distY, distZ });

    return 17 * minDist + 14 * (midDist - minDist) + 10 * (maxDist - midDist);
}

inline std::vector<Entity*> AstartAlgorithm::retracePath(Entity* startObject, Entity* endObject)
{
    std::vector<Entity*> path;
    Entity* current = endObject;

    while (current != startObject)
    {
        path.push_back(current);
        current = current->parentNode;
    }

    std::reverse(path.begin(), path.end());
    return path;
}

