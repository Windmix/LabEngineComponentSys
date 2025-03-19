#pragma once
#include <vector>
#include "entity.h"

class PureEntityData
{
public:
    static PureEntityData* _instance;
    // Vector to store active entities (to track them) , the total Ammount
    std::vector<Entity*> entities;
    std::unordered_map<int, Entity*> gridNodes;
    std::vector<Entity*> nodes;
   // std::vector<Components::CameraComponent*> allCameras;

    int NodestackSizescubicRoot = 0;

    static PureEntityData* instance();
    static void destroy();

    std::vector<Entity*> getNeighbors(Entity* entity);
};
PureEntityData* PureEntityData::_instance = nullptr;

inline PureEntityData* PureEntityData::instance()
{
    if (!_instance)
    {
        _instance = new PureEntityData();
    }
    return _instance;
}
inline void PureEntityData::destroy()
{
    if (_instance)
    {
        delete _instance;
        _instance = nullptr;
    }

}

inline std::vector<Entity*> PureEntityData::getNeighbors(Entity* entity)
{
    std::vector<Entity*> neighbors;

    int currentID = entity->id;
    int size = NodestackSizescubicRoot;
    int maxID = size * size * size - 1;

    // Convert current ID to 3D position
    int currentX = currentID % size;
    int currentY = (currentID / size) % size;
    int currentZ = currentID / (size * size);

    // Iterate through all neighbor offsets in 3D (-1, 0, +1 in each axis)
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dz = -1; dz <= 1; dz++)
            {
                // Skip self
                if (dx == 0 && dy == 0 && dz == 0)
                    continue;

                int nx = currentX + dx;
                int ny = currentY + dy;
                int nz = currentZ + dz;

                // Bounds check
                if (nx < 0 || ny < 0 || nz < 0 || nx >= size || ny >= size || nz >= size)
                    continue;

                // Convert back to 1D ID
                int neighborID = nx + ny * size + nz * size * size;

                // Skip invalid ID
                if (neighborID < 0 || neighborID > maxID)
                    continue;

                // Find neighbor in grid
                auto neighborIt = gridNodes.find(neighborID);
                if (neighborIt == gridNodes.end())
                    continue;



                // Check if diagonal
                bool isDiagonal = (abs(dx) + abs(dy) + abs(dz)) > 1;

                if (isDiagonal)
                {
                    // Check if all adjacent axis-aligned neighbors exist
                    int idX = currentX + dx + currentY * size + currentZ * size * size;
                    int idY = currentX + (currentY + dy) * size + currentZ * size * size;
                    int idZ = currentX + currentY * size + (currentZ + dz) * size * size;

                    if (gridNodes.find(idX) == gridNodes.end() ||
                        gridNodes.find(idY) == gridNodes.end() ||
                        gridNodes.find(idZ) == gridNodes.end())
                    {
                        continue; // Block diagonal if one of the axis-aligned steps is missing
                    }
                }

                // Optional: skip if node is not walkable
                // if (!neighborIt->second->walkable) continue;

                neighbors.push_back(neighborIt->second);
            }
        }
    }

    return neighbors;

}