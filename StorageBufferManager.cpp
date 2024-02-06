#include "classes.h"

// Define static members
tuple<vector<int>, unsigned long long, unsigned long long, unsigned long long> StorageBufferManager::initializationResults;
int StorageBufferManager::maxPagesOnDisk;

unsigned long long StorageBufferManager::Page::dataVectorSize;
unsigned long long StorageBufferManager::Page::offsetArraySize;