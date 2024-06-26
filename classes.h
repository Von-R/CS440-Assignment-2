

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <bitset>
#include <algorithm>
#include <fstream>
#include <tuple>
#include <iomanip>
#include <limits.h>
using namespace std;



class Record {
public:
    int id, manager_id;
    string bio, name;

    Record(vector<string> fields) {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n\n";
    }

    int recordSize() {
        return sizeof(id) + sizeof(manager_id) + name.size() + bio.size();
    }

    // $ marks beginning of record
    // # marks end of field
    // % marks end of record
    string toString() const {
            return "$" + to_string(id) + "#" + name + "#" + bio + "#" + to_string(manager_id) + "%";
        }

    
};

class StorageBufferManager {

    public:
        // initialize the  block size allowed in main memory according to the question 
        const static int BLOCK_SIZE = 4096; 
        const static int maxPages = 3; // 3 pages in main memory at most 
        fstream EmployeeRelation;
        // You may declare variables based on your need 
        int numRecords; // number of records in the file
        int pagesWrittenToFile = 0; // number of pages written to file. Track so that written pages are indexed by page number.
        tuple<vector<int>, unsigned long long, unsigned long long> static initializationResults;
        int static maxPagesOnDisk;
        int static maxEntries;
    
        
         tuple<vector<int>, unsigned long long, unsigned long long> static initializeValues() {
                    int fileCount = 0;
                    int minBioLen = INT_MAX;
                    int maxBioLen = 0;
                    int minNameLen = INT_MAX;
                    int maxNameLen = 0;

                    ifstream file("Employee.csv");
                    
                    // Loop through records
                    string static line;
                    if (file.is_open()) {
                        while (getline(file, line)) {
                            // Split line into fields
                            vector<string> fields;
                            stringstream ss(line);
                            string field;
                            while (getline(ss, field, ',')) {
                                fields.push_back(field);
                            }
                            if (fields.size() != 4) {
                                // Throw an error
                                cerr << "Error: Incorrect number of fields. Fields != 4" << endl;
                            }

                            fileCount++;

                            if (fields[1].length() < minNameLen) {
                                minNameLen = fields[1].length();
                            }
                            if (fields[1].length() > maxNameLen) {
                                maxNameLen = fields[1].length();
                            }
                            if (fields[2].length() < minBioLen) {
                                minBioLen = fields[2].length();
                            }
                            if (fields[2].length() > maxBioLen) {
                                maxBioLen = fields[2].length();
                            }
                        }
                    }

                    // Minimize size of records: ID, name, bio, manager_id and then an 8 byte offset for each
                    // offset could end up being only 4 bytes; test/check
                    int minRecordSize = 3 * 8 + minNameLen + minBioLen;
                    int maxRecords = (BLOCK_SIZE) / minRecordSize;

                    // Use them to calculate number of pages needed assuming all records are max size
                    int maxRecordSize = 3 * 8 + maxNameLen + maxBioLen;

                    // How many records can fit in a page if all records are max size
                    int minRecords = (BLOCK_SIZE) / maxRecordSize;
                    int maxE = fileCount / minRecords + 1;
                    
                    /* Returns tuple containing offset array of size maxRecords, filled with 0's, and the size of the array
                    the total count of all records
                    the max size of record, used later to calc min number of pages needed */
            
                    return make_tuple(vector<int>(maxRecords + 1, -1), static_cast<unsigned long long>(maxRecords) * sizeof(int), static_cast<unsigned long long>(maxE));


                };

        StorageBufferManager(string NewFileName) { 
            /*
                This variable contains:
                    offset array of size maxRecords, filled with 0's
                    the size of the offsetArray
                    the total count of all records
                    the max size of record, used later to calc min number of pages needed
            */
            initializationResults = initializeValues();
            maxEntries = get<2>(initializationResults);
            // cout <<"StorageBufferManager::Constructor, maxEntries: " << maxEntries << endl;
        };
        

        /*
        Returns tuple containing: 
            offset array of size maxRecords, filled with 0's
            the size of the offsetArray
            Max number of pages needed for all records; based on largest record size
        */
        struct FileHeader {
            int totalNumberOfPages; // Total pages in the file
            int pageDirectoryOffset; // Offset where the page directory starts
            int pageDirectorySize; // Number of entries in the page directory; optional if dynamic

            // Default constructor
            FileHeader() : totalNumberOfPages(0), pageDirectoryOffset(sizeof(FileHeader)), pageDirectorySize(1) {}

            void updateTotalNumberOfPages(int newTotal) {
                totalNumberOfPages = newTotal;
            }

            void updateDirectorySize() {
                pageDirectorySize++;
            }

            void serialize(ofstream& file) {
                if (!file.good()) {
                    cerr << "FileHeader.serialize: Error: File is not open for writing.\n";
                    exit(-1);
                }
                /*
                cout << "\nFileHeader.serialize: Writing file header to file at offset: " << file.tellp() << "\n";
                cout << "FileHeader.serialize: totalNumberOfPages: " << totalNumberOfPages << "\n";
                cout << "FileHeader.serialize: pageDirectorySize: " << pageDirectorySize << "\n";
                cout << "FileHeader.serialize: pageDirectoryOffset: " << pageDirectoryOffset << "\n";
                */
                // Write the total number of pages to the file
                file.write(reinterpret_cast<const char*>(&totalNumberOfPages), sizeof(totalNumberOfPages));
                // Write the page directory size to the file
                file.write(reinterpret_cast<const char*>(&pageDirectorySize), sizeof(pageDirectorySize));
                // Write the offset to the page directory to the file
                file.write(reinterpret_cast<const char*>(&pageDirectoryOffset), sizeof(pageDirectoryOffset));
                if (file.fail()) {
                    cerr << "FileHeader.serialize: Error: Failed to write file header to file.\n";
                    exit(-1);
                }
                // cout <<"FileHeader.serialize: Finished writing file header to file. End offset: " << file.tellp() << "\n";
                pageDirectoryOffset = file.tellp();
            }   

            void deserialize(ifstream& file) {
                if (!file.good()) {
                    cerr << "FileHeader.deserialize: Error: File is not open for reading.\n";
                    exit(-1);
                } 

                // cout <<"\nFileHeader.deserialize: Reading file header from file at offset: " << file.tellg() << "\n";
                // Read the total number of pages from the file
                file.read(reinterpret_cast<char*>(&totalNumberOfPages), sizeof(totalNumberOfPages));
                // cout <<"FileHeader.deserialize: totalNumberOfPages: " << totalNumberOfPages << "\n";
                // Read the page directory size from the file
                file.read(reinterpret_cast<char*>(&pageDirectorySize), sizeof(pageDirectorySize));
                // cout <<"FileHeader.deserialize: pageDirectorySize: " << pageDirectorySize << "\n";
                // Read the offset to the page directory from the file
                file.read(reinterpret_cast<char*>(&pageDirectoryOffset), sizeof(pageDirectoryOffset));
                // cout <<"FileHeader.deserialize: pageDirectoryOffset: " << pageDirectoryOffset << "\n";
                if (file.fail()) {
                    cerr << "FileHeader.deserialize: Error: Failed to read file header from file.\n";
                    exit(-1);
                }
                // cout <<"FileHeader.deserialize: Finished reading file header from file. Current offset: " << file.tellg() << "\n\n";
            }
        };

        struct PageDirectoryEntry {
            int pageOffset; // Offset of the page from the beginning of the file
            int recordsInPage; // Number of records within the page

            // Default constructor initializes members to indicate an empty or uninitialized entry
            PageDirectoryEntry() : pageOffset(-1), recordsInPage(0) {}
            PageDirectoryEntry(int offset, int records) : pageOffset(offset), recordsInPage(records) {}
        };

        struct PageDirectory {
            vector<PageDirectoryEntry> entries; // Holds entries for each page
            int entryCount; // Keep track of valid entries so far. Used for add indexing
            int nextPageDirectoryOffset; // File offset to the next directory, or -1 if this is the last
            PageDirectory * nextDirectory; // Pointer to the next directory in the list
            int pageDirectorySize;

            // Default constructor
            PageDirectory() : nextPageDirectoryOffset(-1), entries(maxEntries), entryCount(0), nextDirectory(nullptr) {
                pageDirectorySize = (3 * sizeof(int)) + (entries.capacity() * sizeof(PageDirectoryEntry));
                // cout <<"PageDirectory:: Default constructor. Size of entry vector: " << entries.size() << "\n";
            }

            int getPageDirectorySize() {
                return pageDirectorySize;
            }

            void addNewPageDirectoryNode(ofstream & file, FileHeader * header) {
                // Write the current page directory to the file
                PageDirectory * newPageDirectory = new PageDirectory();
                // cout <<"addNewPageDirectoryNode:: Writing current page directory to file.\n";
                this->nextPageDirectoryOffset = file.tellp();
                // cout <<"addNewPageDirectoryNode:: nextPageDirectoryOffset from tellp: " << nextPageDirectoryOffset << "\n";
                this->nextDirectory = newPageDirectory;
                header->updateDirectorySize();
            }

            // Add a new page directory entry
            int addPageDirectoryEntry(int offset, int records, ofstream& file) {
                // cout <<"\naddPageDirectoryEntry:: begin\n";
                // If directory is full, write to file and return false: create new dir
                if (entryCount >= entries.size()) {
                    serialize(file);
                    cerr << "Error: Page directory is full. Cannot add new entry.\n" <<
                    "Assert: entries.size(): " << entries.size() << " <= entryCount: " << entryCount << "\n" <<
                    "Create and link new page directory instead.\n";
                    return -1;
                } else if (offset == -1 or records == 0) {
                    cerr << "addPageDirectoryEntry:: Error: Invalid offset or record count. Cannot add new entry.\n";
                    return 0;
                }
                // cout <<"addPageDirectoryEntry:: Adding new page directory entry. Offset: " << offset << "\n";
                entries[entryCount].pageOffset = offset;
                // cout <<"addPageDirectoryEntry:: Assert: entries["<< entryCount <<"].pageOffset: " << entries[entryCount].pageOffset << 
                // " == " << offset << "\n";
                entries[entryCount].recordsInPage = records;
                // cout <<"addPageDirectoryEntry:: Assert: entries["<< entryCount <<"].recordsInPage: " << entries[entryCount].recordsInPage <<
                // " == " << records << "\n";
                this->entryCount++;
                // cout <<"addPageDirectoryEntry:: end\nentryCount: " << entryCount << "\n";
                return 1;
            }

            // Function to serialize the PageDirectory to a file
            void serialize(ofstream& file) {
                if (!file.good()) {
                    cerr << "\nPageDirectory.serialize: Error: File is not open for writing.\n";
                    exit(-1);
                }
                // cout <<"Start writing PageDirectory at offset: " << file.tellp() << endl;

                // Write the entry count and next page directory offset first
                file.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));
                // cout <<"PageDirectory.serialize: entryCount: " << entryCount << "\n";
                file.write(reinterpret_cast<const char*>(&nextPageDirectoryOffset), sizeof(nextPageDirectoryOffset));
                // cout <<"PageDirectory.serialize: nextPageDirectoryOffset: " << nextPageDirectoryOffset << "\n";


                vector<PageDirectoryEntry> entriesCopy = entries;
                entriesCopy.resize(entryCount);


                // Then write each entry
                // cout <<"PageDirectory.serialize: Writing page directory entries to file: \n";
                for (const auto& entry : entries) {
                    if (entry.pageOffset == -1) {
                        break;
                    }
                    file.write(reinterpret_cast<const char*>(&entry.pageOffset), sizeof(entry.pageOffset));
                    // cout <<"OS: " << entry.pageOffset << ", ";
                    file.write(reinterpret_cast<const char*>(&entry.recordsInPage), sizeof(entry.recordsInPage));
                    // cout <<"RIP: " << entry.recordsInPage << ", ";
                }
                // cout <<"\n";

                if (file.fail()) {
                    cerr << "PageDirectory.serialize: Error: Failed to write page directory to file.\n";
                    exit(-1);
                }
                // cout <<"Finished writing PageDirectory. End offset: " << file.tellp() << endl << endl;

            } 

            // Function to deserialize the PageDirectory from a file
            void deserialize(ifstream& file, int offset) {
                if (!file.good()) {
                    cerr << "\nPageDirectory.deserialize: Error: File is not open for reading.\n";
                    exit(-1);
                }
                // Read the entry count and next page directory offset first
                // cout <<"PageDirectory.deserialize: Start reading PageDirectory at offset: " << offset << endl;
                file.seekg(offset);
                file.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
                // cout <<"PageDirectory.deserialize: entryCount: " << entryCount << endl;
                file.read(reinterpret_cast<char*>(&nextPageDirectoryOffset), sizeof(nextPageDirectoryOffset));
                // cout <<"PageDirectory.deserialize: nextPageDirectoryOffset: " << nextPageDirectoryOffset << endl;

                // Resize entries vector based on entryCount
                vector<PageDirectoryEntry> entriesCopy = entries;
                entriesCopy.resize(entryCount);

                // Then read each entry
                // cout <<"PageDirectory.deserialize: Reading page directory entries from file: \n";

                int i = 0;
                for (auto& entry : entries) {
                    file.read(reinterpret_cast<char*>(&entry.pageOffset), sizeof(entry.pageOffset));
                    file.read(reinterpret_cast<char*>(&entry.recordsInPage), sizeof(entry.recordsInPage));
                    // cout <<"Page " << i << "; OS: " << entry.pageOffset << ", RIP: " << entry.recordsInPage << " ||| ";
                    i++;
                }

                if (file.fail()) {
                    cerr << "PageDirectory.deserialize: Error: Failed to read page directory from file.\n";
                    exit(-1);
                }
                // cout <<"PageDirectory.deserialize: Finished reading PageDirectory. Current offset: " << file.tellg() << endl << endl;

            }
        };


    class Page {
        public:
        static const int page_size = BLOCK_SIZE; // Define the size of each page (4KB)
        int pageNumber; // Identifier for the page
        Page *nextPage; // Pointer to the next page in the list
        // These are static members of the Page class because the offsetArray and dataVector sizes will be shared by all instances of the Page class
        int static offsetArraySize;
        int static dataVectorSize;
        static const char sentinelValue = '\0'; // Sentinel value to indicate empty space in the data vector
        vector<int> offsetArray; // Vector to store the offsets of the records in the page
        vector<char> data; // Vector to store the data in the page
        int recordCount = 0; // Number of records in the page

        // Helper function to check if the data vector is empty
        bool dataVectorEmpty() {
            for (int i = 0; i < data.size(); i++) {
                if (data[i] != sentinelValue) {
                    return false;
                }
            }
            return true;
        }

        // Returns the size of the data vector by counting non-sentinel values
        // Does not modify data
        int dataSize(const vector<char>& data, char sentinelValue) {
            //// cout <<"dataSize begin" << endl;
            return count_if(data.begin(), data.end(), [sentinelValue](char value) {
            return value != sentinelValue;
            });
            //// cout <<"dataSize end" << endl;
        }

        // Helper function to check if the offset array is empty
        bool offsetArrayEmpty() {
            for (int i = 0; i < offsetArray.size(); i++) {
                if (offsetArray[i] != -1) {
                    return false;
                }
            }
            return true;
        }

        int getDataElems() {
            return dataSize(data, sentinelValue);
        }

        

   
        struct PageHeader {
            int recordsInPage; // Number of records in the page
            int spaceRemaining; // Updated to be initialized later in Page constructor

            PageHeader() : recordsInPage(0) {} // Constructor simplified
        } pageHeader;

        // Constructor for the Page class
        Page(int pageNum) : pageNumber(pageNum), nextPage(nullptr) {
            //cout << "Page constructor begin" << endl;

            // Initialize the offsetArray to its maximum potential size first; sentinel value is -1
            offsetArray = get<0>(initializationResults); // Instance-specific offsetArray is initialized with 0's

            // size of offset array, 2nd element of tuple returned by initializeValues
            offsetArraySize = get<1>(initializationResults); // Update this based on actual logic

             // Update this based on actual logic

            // Size of data vector; calculated based on the size of the offset array and the size of the page header
            dataVectorSize = page_size - offsetArraySize - sizeof(PageHeader);
    
            // Initialize data vector to its maximum potential size first; sentinel value is -1
            data.resize(dataVectorSize, sentinelValue);
            
            // Initialize space remaining in the page
            pageHeader.spaceRemaining = dataVectorSize;
        }

        // Returns the size of the offset array by count non-sentinel values
        // Does not modify offsetArray
        int offsetSize() {
            int count = count_if(offsetArray.begin(), offsetArray.end(), [](int value) {
                return value != -1;
            });
            return count;
        }

        // Returns the size of the data vector by counting non-sentinel values
        int dataSize(const vector<char>& data) {
            int count = count_if(data.begin(), data.end(), [](int value) {
                return value != sentinelValue;
            });
            //cout << "Elements in data array: " << count << endl;
            return count;
        }

        // Method to calculate the space remaining in the page
        int calcSpaceRemaining() {
            // cout  << "calcSpaceRemaining entered:" << endl;
            // Calculate the space remaining based on current usage. PageHeader, page_size and offsetArraySize are all static members/fixed
            int newSpaceRemaining = page_size - dataSize(data, sentinelValue) - offsetArraySize - sizeof(PageHeader);
            // cout  << "calcSpaceRemaining::newSpaceRemaining: " << newSpaceRemaining << endl;
            return newSpaceRemaining;
        }


        // Getters
        int getPageNumber() {return pageNumber;}
        Page * getNextPage() {return nextPage;}
        int getOffsetArraySize() {return offsetArraySize;}
        int getDataVectorSize() {return dataVectorSize;}
        bool checkDataEmpty() {return data.empty();}
        // Getter method to return the next page in the list
        Page * goToNextPage() {
            return this->nextPage;
        }
        int getRecordCount() {return recordCount;}
        

        // Setters
        // Reinitialize data vector elements to sentinel value '\0'
        void emptyData() {fill(data.begin(), data.end(), sentinelValue);}
        // Reinitialize offsetArray elements to sentinel value -1
        void emptyOffsetArray() {fill(offsetArray.begin(), offsetArray.end(), -1);}
        // Method to link this page to the next page
        void setNextPage(Page* nextPage) {
            this->nextPage = nextPage;
        }
        void incrementRecordCount() {recordCount++;}

        

        // Test function to print the contents of a page as indexed by their offsets
        void printPageContentsByOffset() {

            // Check if page is empty. If so simply return
            if (offsetArrayEmpty() && dataVectorEmpty()) {
                // cout  << "Page is empty. No records to print.\n";
                return;
            };

            // Error check: If offsetArray is empty but data vector is not, print error and exit
            if (offsetArrayEmpty() && !dataVectorEmpty()) {
                cerr << "Error: Offset array is empty but data vector not empty!\n";
                exit (-1);
            };
            
            // Error check: If offsetArray is not empty but data vector is empty, print error and exit
            if (!offsetArrayEmpty() && dataVectorEmpty()) {
                cerr << "Error: Data vector is empty but offset array not empty!\n";
                exit (-1);
            };

            // Initial print statements
            // cout  << "printPageContentsByOffset begin" << endl;
            // cout  << "Page Number: " << pageNumber << endl;
            // cout  << "Records in Page: " << pageHeader.recordsInPage << endl;
            // cout  << "Space Remaining: " << pageHeader.spaceRemaining << endl;
            // cout  << "Offset\t\tBeginning of record\t\tRecord Size\n";

            int elementsInOffsetArray = offsetSize();
            // cout  << "elements in offset array: " << elementsInOffsetArray << endl;

            for (size_t i = 0; i <  elementsInOffsetArray; ++i) {
                // Validate the current offset
                if (offsetArray[i] < 0 or offsetArray[i] >= static_cast<int>(dataSize(data))) {
                    cerr << "Error: Invalid offset " << offsetArray[i] << " at offsetArray index " << i << ". Skipping record.\n";
                    continue;
                }

                // Calculate the start and end offsets for the current record
                int startOffset = offsetArray[i];
                int endOffset = (i + 1 < elementsInOffsetArray) ? offsetArray[i + 1] : dataSize(data);

                //if (endOffset == dataSize(data)) { cout << "endOffset currently equal to dataSize(data)" << dataSize(data) << endl;}

                // Validate the end offset
                if (endOffset > static_cast<int>(dataSize(data))) {
                    cerr << "Error: End offset " << endOffset << " exceeds data vector size. Adjusting to data size.\n";
                    endOffset = dataSize(data);
                }

                // Print the offset in hexadecimal and the record's contents
                /*
                cout << "0x" << setw(3) << setfill('0') << hex << startOffset << "\t";
                for (int j = startOffset; j < endOffset; ++j) {
                    // Ensure printing of printable characters only
                    cout << (isprint(data[j]) ? data[j] : '.');
                }
                
                
                cout << "\t\t" << dec << endOffset - startOffset << "\n\n";
                cout << "endoffset: " << endOffset << "\nstartOffset: "<< startOffset << endl;
                */
            }
            // cout  << "printPageContentsByOffset end" << endl;
        }

        // Method to find the offset of the next record in the data vector
        // Searches for the last non-sentinel character and returns the next position
        int findOffsetOfNextRecord(const vector<char>& data, char sentinelValue) {
            // Start from the end of the vector and move backwards
            for (int i = data.size() - 1; i >= 0; --i) {
                if (data[i] != sentinelValue) {
                    // Found the last non-sentinel character, return the next position
                    return i + 1;
                }
            }
            // If all characters are sentinel values or the vector is empty, return 0
            return 0;
        }

        // Returns offset to first sentinel value in the offsetArray
        bool appendToOffsetArray(vector<int>& offsetArray, int newOffset) {
            for (size_t i = 0; i < offsetArray.size(); ++i) {
                if (offsetArray[i] == -1) { // Sentinel value found
                    offsetArray[i] = newOffset; // Replace with new offset
                    return true; // Indicate success
                }
            }
            return false; // Indicate failure (no sentinel value found)
        }

        // Method to add a record to the page
        bool addRecord(const Record& record, bool lastRecordInPage = false) {

            auto recordString = record.toString();
            size_t recordSize = recordString.size();

            // cout <<"\naddRecord:: begin\n";
            // cout <<"addRecord:: recordString size: " << recordSize << endl;

            // Error check if there's enough space left in the page
            if (recordSize > static_cast<size_t>(pageHeader.spaceRemaining)) {
                cerr << "addRecord::Error: Not enough space in the page to add the record.\n";
                return false;
            }

            // Calculate the offset for the new record
            int recordInsertOffset = findOffsetOfNextRecord(data, sentinelValue);
            // cout  << "addRecord:: Inserting record at offset: " << recordInsertOffset << endl;

            // Ensure the insertion does not exceed the vector's predefined max size
            if (recordInsertOffset + recordSize <= data.size()) {
                 // cout  << "addRecord:: \nAdding record to page: " << this->pageNumber << "\n";

                // UPDATE: DATA VECTOR
                // Copies record string to data vector, inserting at the offset provided
                copy(recordString.begin(), recordString.end(), data.begin() + recordInsertOffset);
                
                /*
                for (int i = 0; i < offsetArray.size() - 1; i++) {
                    if (offsetArray[i] == -1) {
                        cout << "Index " << i << " of offsetArray is -1\n";
                        offsetArray[i+1] = recordInsertOffset + recordSize;
                        cout << "addRecord:: offsetArray[" << i + 1 << "] == " << recordInsertOffset + recordSize << "\n";
                        break;
                    }
                }
                */
                
                // Update page header members
                pageHeader.recordsInPage += 1;
                pageHeader.spaceRemaining -= recordSize;

                // cout <<"addRecord:: offsetArray size: " << offsetArray.size() << "\n";
                // cout <<"addRecord:: offsetArray capacity: " << offsetArray.capacity() << "\n";

                // UPDATE: OFFSET ARRAY
                // Append recordInsertOffset to end of offset array. This should point to the beginning of the record.
                if (!appendToOffsetArray(offsetArray, recordInsertOffset)) {
                    cerr << "addRecord:: Error: Unable to add offset to offsetArray.\n";
                    return false;
                } 
                /* else {

                    //cout << "addRecord:: Offset to end of the " << pageHeader.recordsInPage << " record in the page: " << offsetArray.back() << "\n";
                    

                } */

                incrementRecordCount();
                // cout <<"addRecord:: Incremented record count: " << recordCount << "\n";
                // cout <<"addRecord:: end\n";
                return true;







                } else if (recordInsertOffset + recordSize > data.size()) {
                    cerr << "addRecord:: Error: Attempt to exceed predefined max size of data vector.\n";
                    return false;
                } else {
                    cerr << "addRecord:: Error: Unknown error occurred while adding record to page.\n";
                    return false;
                }
            }

            // Method to write the contents of this page to a file, return 
            // offset to end of page in file

            // offsetSize is the number of valid entries in the offsetArray
            int writeRecordsToFile(ofstream& outputFile, int offsetSize) {
                // cout <<"\nwriteRecordsToFile:: entries in offsetArray: " << offsetSize << "\n";

                // Error check stream state
                if (!outputFile.is_open()) {
                    cerr << "Error: Output file is not open for writing.\n";
                    return -1; // Indicate error
                }

                // cout <<"writeRecordsToFile:: Writing page " << pageNumber << " to file...\n";
                for (int i = 0; i < this->getDataElems(); i++) {
                    outputFile.write(reinterpret_cast<const char*>(&data[i]), sizeof(data[i]));
                    // cout << data[i];
                }

                /*
                for (int i = 0; i < offsetSize - 1; ++i) { // -1 to prevent accessing beyond the last valid index
                    int startOffset = offsetArray[i];
                    cout << "\nwriteRecordsToFile::startOffset: " << startOffset << ". offsetArray[" << i << "].\n";
                    int endOffset = offsetArray[i + 1]; // Get the end offset for the current segment
                    cout << "\nwriteRecordsToFile::endOffset: " << endOffset <<  ". offsetArray[" << i + 1 << "]. \n";

                    cout << "\nwriteRecordsToFile::startOffset: " << startOffset << " endOffset: " << endOffset << "\n";
                    // Write each character in the range [startOffset, endOffset) to the file
                    for (int j = startOffset; j < endOffset; ++j) {
                        if (data[j] != sentinelValue) {
                            cout << data[j];   
                            outputFile.write(reinterpret_cast<const char*>(&data[j]), sizeof(data[j]));
                        }
                    }
                }
                */

                // Optionally, return the new offset after writing, if needed
                // cout <<"\n\nwriteRecordsToFile:: endOffset of page on DISK: " << outputFile.tellp() << "\n";
                return outputFile.tellp();
            }

        }; // End of Page definition
            
                
        // Contains linked list of pages and necessary functions
        class PageList {
            public:
            Page *head = nullptr;
            //static const int maxPages = 3; 
            // Constructor for the PageList class
            // Head is initialized as page 0 and linked list created from it
            PageList() {
                //// cout  << "PageList constructor begin" << endl;
                head = initializePageList();
                //// cout  << "PageList constructor end" << endl;
            }

            // Create 'maxPages' number of pages and link them together
            // Initialization of head member attirbutes done in head constructor
            Page* initializePageList() {
                //// cout  << "initializePageList begin" << endl;
                Page *tail = nullptr;
                for (int i = 0; i < maxPages; ++i) {
                    //// cout  << "Creating page " << i << endl;
                    Page *newPage = new Page(i); // Create a new page
                    //// cout  << "Page " << i << " created" << endl;

                    if (!head) {
                        //// cout  << "Setting head to page " << i << endl;
                        head = newPage; // If it's the first page, set it as head
                    } else {  
                        //// cout  << "Linking page " << i << " to page " << i - 1 << endl;
                        tail->setNextPage(newPage); // Otherwise, link it to the last page
                    }
                    //// cout  << "Setting tail to page " << i << endl;
                    tail = newPage; // Update the tail to the new page
                }
                //// cout  << "initializePageList end" << endl;
                return head; // Return the head of the list
            };

              // Destructor for the PageList class
            ~PageList() {
                //// cout  << "PageList destructor begin" << endl;
                Page* current = head;
                while (current != nullptr) {
                    Page* temp = current->getNextPage(); // Assuming nextPage points to the next Page in the list
                    delete current; // Free the memory of the current Page
                    current = temp; // Move to the next Page
                };
                //// cout  << "PageList destructor end" << endl;
            };

            // Method to reset the data and offsetArray of each page in the list to initial values
            void resetPages() {
                //// cout  << "resetPage begin" << endl;
                Page * current = head;
                while (current != nullptr) {
                    current->emptyData();
                    current->emptyOffsetArray();
                    current->pageHeader.recordsInPage = 0;
                    current->recordCount = 0;
                    current->pageHeader.spaceRemaining = current->calcSpaceRemaining();
                    current = current->getNextPage();
                }
                //// cout  << "resetPage end" << endl;
            }

            // Method to dump the pages to a file
            bool dumpPages(ofstream& file, int& pagesWrittenToFile, FileHeader* header, PageDirectory* pageDirectory) {
                //// cout  << "Dumping pages to file...\n";

                streampos tmpOffset;

                if (!file.is_open()) {
                    cerr << "File is not open for writing.\n";
                    return false;
                }

                Page* currentPage = head;
                int pageOffset;

                int dummyVal;

                while (currentPage != nullptr) {
                    pageOffset = file.tellp();
                    // Empty page. End loop.
                    if (currentPage->dataVectorEmpty() /* or currentPage->recordCount == 0 */) {
                        // cout <<"dumpPages:: Page " << currentPage->getPageNumber() << " is empty. breaking...\n";
                        break;
                    }

                    // Write page contents to file, get offset pointing to beginning of free space in file
                    dummyVal = currentPage->writeRecordsToFile(file, currentPage->offsetSize());

                    // Diagnostic print statements: page offset and number of offsets in page offset array
                    // cout <<"\ndumpPages::pageOffset: " << pageOffset <<
                    //". offsetSize: " << currentPage->offsetSize() <<  "\n";
                    
                    // Error check: If pageOffset is negative stream failed to open: print error and return false
                    if (pageOffset < 0){
                        cerr << "Failed to write page records to file.\n";
                        return false;
                    } //else {
                       // cout << "dumpPages:: pageOffset " << pageOffset << " >= 0\n";
                    //}

                    // Update page count for new page directory entry
                    int currentPageRecordCount = currentPage->getRecordCount();
                    
                    // Update the page directory with new entry
                    
                    // cout <<"dumpPages::Adding page directory entry. Offset: " << pageOffset << "\n";
                    if (pageDirectory->addPageDirectoryEntry(pageOffset, currentPageRecordCount, file) == -1){
                        cerr << "dumpPages::Error: entries capacity exceeded. Dynamic calculation failed.\n";
                        exit(-1);
                    }

                    // Diagnostic print statements: print info of most recent page added to page directory
                    /*
                    if (pageDirectory->entries[pageDirectory->entryCount].pageOffset != -1) {
                    cout << "dumpPages:: addPageDirectory successful. Printing most recent entry added to page directory:\n";
                    cout << "dumpPages::pageDirectory->entries[" << pageDirectory->entryCount << "].pageOffset: " << pageDirectory->entries[pageDirectory->entryCount].pageOffset << "\n";
                    cout << "Assert: " << pageDirectory->entries[pageDirectory->entryCount].pageOffset << " == " << pageOffset << "\n";
                    cout << "dumpPages::pageDirectory->entries[" << pageDirectory->entryCount << "].recordsInPage: " << pageDirectory->entries[pageDirectory->entryCount].recordsInPage << "\n";
                    cout << "Assert: " << pageDirectory->entries[pageDirectory->entryCount].recordsInPage << " == " << currentPageRecordCount << "\n\n";
                    break;
                    }
                    */
                    

                    pagesWrittenToFile++;
                    currentPage = currentPage->getNextPage();
                }

                // Serialize and write the updated page directory and file header here
                // Reset the pages for reuse
                tmpOffset = file.tellp();
                file.seekp(0);
                header->updateTotalNumberOfPages(pagesWrittenToFile);
                
                header->serialize(file);
                pageDirectory->serialize(file);
                file.seekp(tmpOffset);
                resetPages();
                //cout << "Page dumping complete.\n";
                return true;
            }

            /*
            Test function: Use to confirm page is being filled properly. 
            */
            void printMainMemory() {
                // cout <<"Printing main memory contents...\n";
                Page* page = head;
                while (page != nullptr) {
                    // Print the page number as a header for each page's content
                    // cout <<"\n\nPrinting contents of Page Number: " << page->getPageNumber() << endl;
                    // Now, use the printPageContentsByOffset method to print the contents of the current page
                    page->printPageContentsByOffset();
                    // Move to the next page in the list
                    page = page->getNextPage();
                }
                // cout <<"End of main memory contents.\n";
            }

        }; // End of PageList definition

        void initializeDataFile(ofstream & file, FileHeader * header, PageDirectory * pageDirectory) {
            // Clear file
            
            // Error check: If file cannot be opened, print error and exit
            if (!file.is_open()) {
                cerr << "Unable to open file for initialization " << endl;
                exit(-1);
            }

            // Write an empty file header
            header->pageDirectoryOffset = sizeof(header);
            file.write(reinterpret_cast<const char*>(&header), sizeof(header));
            file.write(reinterpret_cast<const char*>(&pageDirectory), pageDirectory->getPageDirectorySize());
            //cout << "initializeDataFile:: pageDirectorySize: " << sizeof(pageDirectory) << ".\n";
            
        }



        // Create record from csv line
        Record createRecord(string line) {
            // cout  << "createRecord begin" << endl;
            // Split line into fields
            vector<string> fields;
            stringstream ss(line);
            string field;
            while (getline(ss, field, ',')) {
                fields.push_back(field);
            }
            // cout  << "createRecord end" << endl;
            return Record(fields);
        };

        void loadMemoryPage(ifstream & file, Page * page, int beginOffset, int endOffset, bool lastPage = false) {
            // Error check: If file cannot be opened, print error and exit
            // cout <<"loadMemoryPage begin" << endl;
            if (!file.is_open()) {
                cerr << "Error opening file for reading.\n";
                exit(-1);
            }

            // Calculate the size of the chunk to read
            int size = endOffset - beginOffset;
            if (size <= 0) {
                cerr << "Invalid offsets provided.\n";
                return;
            }


            // Test Prints
            // cout <<"\nloadMemoryPage:: beginOffset: " << beginOffset << ".\n";
            // cout <<"loadMemoryPage:: endOffset: " << endOffset << ".\n";

            file.seekg(beginOffset, ios::beg);
            // cout <<"loadMemoryPage:: Current file offset after seekg to beginOffset: " << file.tellg() << ".\n";

            // Read the page from the file in page in memory
            // cout <<"loadMemoryPage:: Reading page " << page->getPageNumber() << " from file. " << size << " bytes; offset: " << beginOffset << ".\n";
            // cout <<"loadMemoryPage:: Current file offset: " << file.tellg() << ".\n";
            if (!lastPage) {
                for (int i = 0; i < size; i++) {
                    file.read(reinterpret_cast<char*>(&page->data[i]), sizeof(page->data[i]));
                    // cout <<page->data[i];
                    //if (page->getPageNumber() == 0) {
                    //    cout << "(" << i + beginOffset << ": " << page->data[i] << ")";
                    //} else {
                    //}
                }
            }
            else if (lastPage){
                     for (int i = 0; i < size; i++) {
                    file.read(reinterpret_cast<char*>(&page->data[i]), sizeof(page->data[i]));
                    // cout <<page->data[i];
                    //if (page->getPageNumber() == 0) {
                    //    cout << "(" << i + beginOffset << ": " << page->data[i] << ")";
                    //} else {
                    //}
                }

            }
            // cout <<"\n\nloadMemoryPage end" << endl;
        }

        void searchID(int searchID){
           
            // cout <<"searchID:: Records in file: " << get<2>(initializationResults) << ".\n";
            cout << "Searching for Record ID: " << searchID << endl;
            // instantiate objects
            FileHeader * header = new FileHeader();
            PageDirectory * pageDirectory = new PageDirectory();
            PageList * pageList = new PageList();
            Page * currentPage = pageList->head;
            int begIndex;
            int endIndex;
            vector<Record> matchingRecords;

            // Open file for reading
            ifstream dataFile("EmployeeRelation.dat", ios::binary | ios::in);
            dataFile.seekg(0, ios::beg);
            
            // Error check: If file cannot be opened, print error and exit
            if (!dataFile.is_open()) {
                cerr << "Error opening file for reading.\n";
                exit(-1);
            }

            // Deserialize file header and page directory
            header->deserialize(dataFile);
            // cout <<"searchID:: Number of records on file: " << header->totalNumberOfPages << ".\n";
            pageDirectory->deserialize(dataFile, header->pageDirectoryOffset);

            // cout <<"searchID:: Deserialized file header and page directory test prints: \n\n";
            // cout <<"searchID:: header->totalNumberOfPages: " << header->totalNumberOfPages << endl;
            // cout <<"searchID:: header->pageDirectoryOffset: " << header->pageDirectoryOffset << endl;
            // cout <<"searchID:: pageDirectory->entryCount: " << pageDirectory->entryCount << endl;
            // cout <<"searchID:: pageDirectory->nextPageDirectoryOffset: " << pageDirectory->nextPageDirectoryOffset << endl;
            // cout <<"searchID:: pageDirectory->entries[0].pageOffset: " << pageDirectory->entries[0].pageOffset << endl;
            // cout <<"searchID:: pageDirectory->entries[0].recordsInPage: " << pageDirectory->entries[0].recordsInPage << endl;
            // cout <<"searchID:: End deserialized file header and page directory test prints.\n\n";
            

            // Loop through page directories
            while (pageDirectory != nullptr) {
                // cout <<"searchID:: Looping through page directories...\n";
                // cout <<"searchID:: entryCount: " << pageDirectory->entryCount << " pages in directory\n";

                // Loop through page directory entries
                for (int entryIndex = 0; entryIndex < pageDirectory->entryCount - 1; entryIndex++) {

                    // cout <<"searchID:: Looping through page directory entries...\n\n";
                    
                    // Error: Break if offset for page referenced in page directory is sentinel value / uninitialized
                    if (pageDirectory->entries[entryIndex].pageOffset == -1) {
                        cerr << "searchID:: Error: Page offset is invalid. Breaking...\n";
                        // Diagnostic print
                        /*
                        for (int i = 0; i < pageDirectory->entries.size(); i++) {
                            cout << "searchID:: pageDirectory->entries[" << i << "].pageOffset: " << pageDirectory->entries[i].pageOffset << endl;
                            cout << "searchID:: pageDirectory->entries[" << i << "].recordsInPage: " << pageDirectory->entries[i].recordsInPage << endl; 
                        }
                        */
                        break;
                    }

                    // Offset on DISK of beginning of page referenced in page directory
                    begIndex = pageDirectory->entries[entryIndex].pageOffset;
                    // Offset on DISK of end of page referenced in page directory
                    endIndex = pageDirectory->entries[entryIndex + 1].pageOffset;
                    // cout <<"searchID:: begIndex: " << begIndex << endl;
                    // cout <<"searchID:: endIndex: " << endIndex << endl;

                    // If last main memory page, load page from disk then search full main memory
                    if (currentPage->getNextPage() == nullptr) {
                        // cout <<"searchID:: Last page reached. Loading and searching...\n";
                        loadMemoryPage(dataFile, currentPage, begIndex, endIndex);
                        searchMainMemory(pageList->head, searchID, matchingRecords);
                        pageList->resetPages();
                        currentPage = pageList->head;
                    }
                    // Otherwise just load the page
                    // Advance to next page
                    else if (currentPage != nullptr) {
                        // cout <<"searchID:: Loading and advancing to next page...\n";
                        // Load a single page, advance to next page
                        loadMemoryPage(dataFile, currentPage, begIndex, endIndex);
                        currentPage = currentPage->nextPage;
                    } else {
                        cerr << "Error: Some weird error\n";
                        exit(-1);
                    }
                }

                // If last page directory, load last page from disk then search full main memory
                // This is the cause of the last record not being found
                // Not sure how to fix


                // Handle cases where current page has stuff in it
                if (!currentPage->dataVectorEmpty() && currentPage->getNextPage() != nullptr) {
                    currentPage = currentPage->getNextPage();
                } else if (!currentPage->dataVectorEmpty()) {
                    searchMainMemory(pageList->head, searchID, matchingRecords);
                    pageList->resetPages();
                    currentPage = pageList->head;
                }
                
                // cout <<"searchID:: searching last batch of records...\n\n";
                begIndex = dataFile.tellg();
                char ch;
                endIndex = begIndex;
                // dataFile.seekg(begIndex, ios::beg);
               
                while (dataFile.get(ch)) {
                    // cout <<ch;
                    endIndex = dataFile.tellg();
                }
                dataFile.clear();
                dataFile.seekg(begIndex, ios::beg);

                // cout <<"\n\nsearchID:: begIndex: " << begIndex << endl;
                // cout <<"searchID:: endIndex: " << endIndex << endl;
                loadMemoryPage(dataFile, currentPage, begIndex, endIndex);
                searchMainMemory(pageList->head, searchID, matchingRecords);
                // Move to the next page directory
                // cout <<"searchID:: Moving to next page directory...\n";
                pageDirectory = pageDirectory->nextDirectory;
            }
            // cout <<"searchID end" << endl;

            // Print matching records
            if (matchingRecords.empty()) {
                cout << "No matching records found for Search ID: " << searchID << ".\n";
            } else {
                cout << "Matching records found for Search ID: " << searchID << ".\n\n";
                for (auto record : matchingRecords) {
                    record.print();
                }
            }
        };

        // Read records from file and search by ID
        // There may be duplicates, so search the entire file
        void searchMainMemory(Page* page, int searchID, vector<Record>& matchingRecords) {
            // cout <<"searchMainMemory begin:: searching Page " << page->pageNumber << endl;

            int i = 0;

            // Print the contents of the page
            /*
            while (i < page->data.size()) {
                cout << page->data[i];
                i++;
            }
            */
            
            // cout <<"searchMainMemory:: Initializing records variables...\n";
            string recordsString = string(page->data.begin(), page->data.end());
            istringstream recordsStream(recordsString);
            string record;
            int recordCounter = 1;

            // cout <<"searchMainMemory:: Looping through records...\n";
            while (getline(recordsStream, record, '%')) {
                // cout <<"Parsing record number " << recordCounter++ << "\n";
                if (record.empty()){ 
                    // cout <<"searchMainMemory:: Empty record found. Investigate. Skipping...\n";
                    continue; // Skip empty records
                }

                // cout <<"searchMainMemory:: Initializing individual record variables...\n";
                istringstream recordStream(record);
                vector<string> fields;
                string field;

                // cout <<"searchMainMemory:: Looping through fields...\n";
                while (getline(recordStream, field, '#')) {
                    fields.push_back(field);
                }

                fields[0] = fields[0].substr(1);
                // cout <<"Record ID: " << fields[0] << endl;

                //for (auto field : fields) {
                //    cout << "Field: " << field << ". Field type: " << typeid(field).name() << endl;
                //}

                if (fields.size() == 4 && stoi(fields[0]) == searchID) {
                    // cout <<"Matching record found: " << fields[0] << endl;
                    Record foundRecord(fields);
                    matchingRecords.push_back(foundRecord);
                    //foundRecord.print(); // Assuming Record::print() is a method to print the record details
                }
            }

            // Recursive call with the next page
            if (page->getNextPage() == nullptr) {
                return; // Base case: Reached the end of the page list
            } else {
                searchMainMemory(page->getNextPage(), searchID, matchingRecords);
            }
            // cout <<"searchMainMemory end" << endl;
        }


        void exitProgram(ofstream &EmpStream) {
            if (EmpStream.is_open()) {
                EmpStream.close();
            }
        };
 
        // Read csv file (Employee.csv) and add records to the (EmployeeRelation)
        void createFromFile(string csvFName) {
            // cout  << "CreateFromFile: Begin createFromFile...\n";

            FileHeader * header = new FileHeader();
            PageDirectory * pageDirectoryHead = new PageDirectory();
            PageDirectory * currentPageDirectory = pageDirectoryHead;

            // cout <<"createFromFile: FileHeader size: " << sizeof(*header) << endl;
            // cout <<"createFromFile: PageDirectory size: " << pageDirectoryHead->getPageDirectorySize() << endl;
            // cout <<"createFromFile: PageDirectory entries: " << pageDirectoryHead->entries.size() << endl;

            // initialize variables
            // Offset of record
            int offset = 0;
            // For reading line from csv
            string line;
            // Add success
            bool addFlag = false;
            // Dump success
            bool dumpFlag = false;
            
            int spaceRemaining;
            
            // Size of record
            int recordSize = 0;

            // Initialize pages and create pointer to first page
            PageList * pageList = new PageList();
            
            // Open source file for reading source csv
            ifstream fileStream(csvFName.c_str());

            // Bootstrap data file: create file and initialize
            ofstream EmployeeRelation("EmployeeRelation.dat", ios::binary | ios::out | ios::trunc);
            initializeDataFile(EmployeeRelation, header, currentPageDirectory);
            
            // Buffer for reading line from csv: equal to max possible record size plus 1
            char buffer[2 * 8 + 200 + 500 + 1];

            // Loop through records
            if (fileStream.is_open()) {
                // cout  << "CreateFromFile: File opened...\n\n\n";
                // Get each record as a line
                Page * currentPage = pageList->head;
                bool lastRecordInPage = false;
                while (getline(fileStream, line)) {
                    //// cout  << "CreateFromFile: Reading line from file...\n";
                    // Create record from line 
                    Record record = createRecord(line);
                    //// cout  << "CreateFromFile: Record created from line...\nTest print: \n";
                    //record.print();
                    
                    // Get the size of the stringified record
                    // Stringified record is the data that will actually be added to memory
                    recordSize = record.toString().size();
                    //// cout  << "CreateFromFile: Record size: " << recordSize << endl;

                    // Check that record size is less than block size
                    if (recordSize > currentPage->getDataVectorSize()) {
                        cerr << "Record size exceeds block size.\n";
                        exit(1);
                    }
                    //// cout  << "CreateFromFile:: Check passed:  Record size is less than size of currPage dataVector...\n";
                    
                    // determine if there's enough room on current page:
                        // if not and not last page, begin writing to next page
                        // if not and last page, write page to file and empty page
                    spaceRemaining = currentPage->calcSpaceRemaining();
                    //// cout  << "CreateFromFile: Space remaining on current page: " << spaceRemaining << endl;

                    // While current page is full, and we're not on the last page, advance to next page
                    while (spaceRemaining < recordSize && currentPage->getPageNumber() < maxPages - 1) {
                        
                        // Advance to next page
                        currentPage = currentPage->goToNextPage();
                        // Recalculate space remaining on new page
                        spaceRemaining = currentPage->calcSpaceRemaining();
                        // Add record to new page
                        /*
                        if (!currentPage->addRecord(record)) {
                            cerr << "Error: Size mismatch on NEXT PAGE ADD. Terminating..." << endl;
                            exit(-1);
                        } else {
                            cout << "Record added to next page, page: " << currentPage->getPageNumber() << ": \n" << record.toString() << "\n";
                        }
                        */
                    }
                    // Main memory full: no room for record on any pages
                    // Write contents to file, then 
                    if (spaceRemaining < recordSize && currentPage->getPageNumber() == maxPages - 1) {
                        //// cout  << "CreateFromFile: Main memory full. Test printing contents...\n";
                        //pageList->printMainMemory();
                        
                        if (!pageList->dumpPages(EmployeeRelation, pagesWrittenToFile, header, currentPageDirectory)) {
                            cerr << "Error: Failure to copy main memory contents to file. Stream not open." << endl;
                            exit(-1);
                        } 
                        /* else {
                            cout << "createFromFile: Main memory full. Dumping pages to file...\n";
                        } */

                        currentPage = pageList->head;
                    }
                    // There's room on the current page. Add record
                    // If returns false, some kind of logic error to resolve
                    else {
                        // Add record to page
                        //// cout  << "CreateFromFile: Adding record to page...\n";
                        
                        if (!currentPage->addRecord(record)) {
                            cerr << "Size mismatch. Terminating..." << endl;
                            exit(-1);
                        } /* else {
                            cout << "Record added to page " << currentPage->getPageNumber() << ": \n" << record.toString() << "\n";
                        } */
                        //// cout  << "CreateFromFile: Record added to page?\n";
                    }            
                }

                // After getline finishes, if page is not empty, write to file
                // Situation: final records, don't fill entire page
                if (!currentPage->checkDataEmpty()) {

                    // Diagnostic print
                    //// cout  << "CreateFromFile: Test printing last page...\n";
                    // pageList->printMainMemory();
                   
                    // Write contents to file
                    if (!pageList->dumpPages(EmployeeRelation, pagesWrittenToFile, header, currentPageDirectory)) {
                            cerr << "Error: Failure to copy main memory contents to file. Stream not open." << endl;
                            exit(-1);
                        } /* else {
                            cout << "createFromFile: Last of records read into main memory. Dumping records to file...\n";
                        }*/
                    // Reset to head node
                    currentPage = pageList->head;
                }
                delete pageList;

                currentPageDirectory = pageDirectoryHead;

                // cout <<"createFromFile: File written to successfully.\n";
                // cout <<":: header->totalNumberOfPages: " << header->totalNumberOfPages << endl;
                // cout <<":: pageDirectory->entryCount: " << currentPageDirectory->entryCount << endl;
                // cout <<":: pageDirectory->nextPageDirectoryOffset: " << currentPageDirectory->nextPageDirectoryOffset << endl;
                // cout <<":: pageDirectory->entries[0].pageOffset: " << currentPageDirectory->entries[0].pageOffset << endl;
                // cout <<":: pageDirectory->entries[0].recordsInPage: " << currentPageDirectory->entries[0].recordsInPage << endl;
                // cout <<"/createFromFile:: End deserialized file header and page directory test prints.\n\n";
                // cout <<"createFromFile: End createFromFile...\n\n\n\n";

            };

        

        /*
        void printStructuredFileContents(const string& filePath) {
            ifstream file(filePath, ios::binary);
            if (!file) {
                cerr << "Cannot open file: " << filePath << endl;
                return;
            }

            // Assuming the file starts with the size of the page directory (e.g., an integer)
            int pageDirectorySize;
            file.read(reinterpret_cast<char*>(&pageDirectorySize), sizeof(pageDirectorySize));

            // Read the page directory to get page offsets
            vector<int> pageOffsets(pageDirectorySize);
            file.read(reinterpret_cast<char*>(pageOffsets.data()), pageDirectorySize * sizeof(int));

            // Iterate over each page using the offsets
            for (int pageOffset : pageOffsets) {
                file.seekg(pageOffset, ios::beg);

                // Read page metadata here (e.g., number of records in the page)
                // For simplicity, let's assume we just start reading records

                // Example: read the size of the record vector for the page
                int recordVectorSize;
                file.read(reinterpret_cast<char*>(&recordVectorSize), sizeof(recordVectorSize));

                vector<int> recordOffsets(recordVectorSize);
                file.read(reinterpret_cast<char*>(recordOffsets.data()), recordVectorSize * sizeof(int));

                // Now print each record by seeking to its offset within the page
                for (int recordOffset : recordOffsets) {
                    // Seek to the record's position within the current page
                    file.seekg(pageOffset + recordOffset, ios::beg);

                    // Assuming records are terminated by a newline character for simplicity
                    string record;
                    getline(file, record);
                    cout << record << endl;
                }
            }

            file.close();
        } */
    }; 
};    // End of StorageBufferManager definition