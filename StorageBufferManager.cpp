#include "classes.h"

// Define static members
tuple<vector<int>, unsigned long long> StorageBufferManager::offsetVectorParameters;
int StorageBufferManager::Page::dataVectorSize;
int StorageBufferManager::Page::offsetVectorSize;
const char StorageBufferManager::Page::sentinelValue; 