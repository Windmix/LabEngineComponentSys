#pragma once
#include <vector>
#include <utility>  // For std::forward


template <typename T, size_t ChunkSize = 64>
class ChunkAllocator
{
public:
    ChunkAllocator() = default;
    ~ChunkAllocator(); // Proper cleanup

    struct Chunk
    {
        T* data = nullptr;        // Pointer to a block of memory holding objects
        bool* freeList = nullptr; // A bitmap that tracks free slots
        size_t allocatedCount = 0; // Number of active objects
    };

    std::vector<Chunk> chunks;  // All allocated chunks

    // Allocate an object with arguments
    template <typename... Args>
    T* Allocate(Args&&... args);

    // Deallocate an object
    void Deallocate(T* ptr);

private:
    Chunk* FindFreeChunk();
    std::optional<size_t> GetChunkIndex(T* ptr) const;
    bool IsValid(T* ptr) const;
    void DeallocateChunk(size_t chunkIndex);
};

//---------------------------------------------------------------------------

template<typename T, size_t ChunkSize>
ChunkAllocator<T, ChunkSize>::~ChunkAllocator()
{
    for (auto& chunk : chunks)
    {
        delete[] chunk.data;
        delete[] chunk.freeList;
    }
    chunks.clear();
}

//---------------------------------------------------------------------------

template<typename T, size_t ChunkSize>
template <typename... Args>
T* ChunkAllocator<T, ChunkSize>::Allocate(Args&&... args)
{
    Chunk* chunk = FindFreeChunk();

    // If no free chunk exists, create a new one
    if (!chunk)
    {
        chunks.emplace_back();
        chunk = &chunks.back();

        chunk->data = new T[ChunkSize];
        chunk->freeList = new bool[ChunkSize]();
        chunk->allocatedCount = 0;
    }

    // Find an available slot in the chunk
    for (size_t i = 0; i < ChunkSize; i++)
    {
        if (!chunk->freeList[i])  // Slot is free
        {
            chunk->freeList[i] = true;
            chunk->allocatedCount++;

            T* obj = &chunk->data[i];
            new(obj) T(std::forward<Args>(args)...);  // Construct object in place
            return obj;
        }
    }

    return nullptr;  // Should never happen
}

//---------------------------------------------------------------------------

template<typename T, size_t ChunkSize>
void ChunkAllocator<T, ChunkSize>::Deallocate(T* ptr)
{
    if (!ptr) return;

    auto chunkIndexOpt = GetChunkIndex(ptr);
    if (!chunkIndexOpt)
    {
        throw std::invalid_argument("Tried to deallocate an invalid pointer.");
    }

    size_t chunkIndex = *chunkIndexOpt;
    Chunk& chunk = chunks[chunkIndex];

    // Get index of object in chunk
    size_t indexInChunk = ptr - chunk.data;

    if (chunk.freeList[indexInChunk]) // Prevent double free
    {
        ptr->~T();
        chunk.freeList[indexInChunk] = false;
        chunk.allocatedCount--;

        // If chunk is empty, deallocate it
        if (chunk.allocatedCount == 0)
        {
            DeallocateChunk(chunkIndex);
        }
    }
}

//---------------------------------------------------------------------------

template<typename T, size_t ChunkSize>
typename ChunkAllocator<T, ChunkSize>::Chunk* ChunkAllocator<T, ChunkSize>::FindFreeChunk()
{
    for (auto& chunk : chunks)
    {
        for (size_t i = 0; i < ChunkSize; ++i)
        {
            if (!chunk.freeList[i]) // Found free slot
            {
                return &chunk;
            }
        }
    }
    return nullptr;  // No free chunk found
}

//---------------------------------------------------------------------------

template<typename T, size_t ChunkSize>
std::optional<size_t> ChunkAllocator<T, ChunkSize>::GetChunkIndex(T* ptr) const
{
    for (size_t i = 0; i < chunks.size(); ++i)
    {
        if (ptr >= chunks[i].data && ptr < chunks[i].data + ChunkSize)
        {
            return i;
        }
    }
    return std::nullopt;
}

//---------------------------------------------------------------------------

template<typename T, size_t ChunkSize>
bool ChunkAllocator<T, ChunkSize>::IsValid(T* ptr) const
{
    return GetChunkIndex(ptr).has_value();
}

//---------------------------------------------------------------------------

template<typename T, size_t ChunkSize>
void ChunkAllocator<T, ChunkSize>::DeallocateChunk(size_t chunkIndex)
{
    Chunk& chunk = chunks[chunkIndex];

    delete[] chunk.data;
    delete[] chunk.freeList;

    chunks.erase(chunks.begin() + chunkIndex);
}