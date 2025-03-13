#pragma once
#include <unordered_set>
#include <queue>
#include "pureEntityData.h"

struct CompareGameObjectX
{
    bool operator()(Entity* a, Entity* b); // min-heap behavior
};

class AstarAlgorithm
{
    PureEntityData* entityData;
    bool containsInOpenList(Entity* gameobject);
public:
    AstarAlgorithm();
    ~AstarAlgorithm();
    
    static AstarAlgorithm* _instance;
    // storage
    std::priority_queue<Entity*, std::vector<Entity*>, CompareGameObjectX> openList;
    std::unordered_set<Entity*> closedList;
    std::unordered_set<Entity*> openSet;

    //singleton instance
    static AstarAlgorithm* Instance();
    static void destroy();

    // Method

    std::vector<Entity*> findPath(Entity* start, Entity* end);
    int getDistance(Entity* objectA, Entity* objectB);
    std::vector<Entity*> retracePath(Entity* startObject, Entity* endObject);
};

AstarAlgorithm* AstarAlgorithm::_instance = nullptr;


inline bool CompareGameObjectX::operator()(Entity* a, Entity* b)
{
    return a->FCost() > b->FCost(); // Lower fCost = higher priority
}

inline bool AstarAlgorithm::containsInOpenList(Entity* gameobject)
{
    return openSet.find(gameobject) != openSet.end();
}

inline AstarAlgorithm::AstarAlgorithm()
{
    entityData = PureEntityData::instance();
}

inline AstarAlgorithm::~AstarAlgorithm()
{
    entityData->destroy();
}

inline AstarAlgorithm* AstarAlgorithm::Instance()
{
    if (!_instance)
    {
        _instance = new  AstarAlgorithm();
    }
    return _instance;
}

inline void AstarAlgorithm::destroy()
{
    if (_instance)
    {
        delete _instance;
        _instance = nullptr;
    }
}
inline std::vector<Entity*> AstarAlgorithm::findPath(Entity* start, Entity* end)
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

        for (Entity* neighbor : entityData->getNeighbors(current))
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


inline int AstarAlgorithm::getDistance(Entity* objectA, Entity* objectB)
{
    //Heuristic Formula
    auto transA = objectA->GetComponent<Components::TransformComponent>();
    auto transB = objectB->GetComponent<Components::TransformComponent>();

    int dx = abs(static_cast<int>(transA->transform[3].x - transB->transform[3].x));
    int dy = abs(static_cast<int>(transA->transform[3].y - transB->transform[3].y));
    int dz = abs(static_cast<int>(transA->transform[3].z - transB->transform[3].z));

    int minD = std::min({ dx, dy, dz });
    int maxD = std::max({ dx, dy, dz });

    int midD = dx + dy + dz - minD - maxD;

    return 17 * minD + 14 * (midD - minD) + 10 * (maxD - midD);
}

inline std::vector<Entity*> AstarAlgorithm::retracePath(Entity* startObject, Entity* endObject)
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

