#include "classes.h"

// Define static members
tuple<std::vector<int>, unsigned long long, unsigned long long, unsigned long long>  StorageBufferManager::initializationResults;
int StorageBufferManager::Page::dataVectorSize;
int StorageBufferManager::Page::offsetArraySize;
const char StorageBufferManager::Page::sentinelValue; 