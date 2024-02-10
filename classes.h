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
        cout << "\tMANAGER_ID: " << manager_id << "\n";
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

// Reconstruct fields vector from stringified records on file
// Used to reconstruct records from file
inline vector<string> stringToVector(const string& recordString) {
    // cout << "stringToVector begin" << endl;
        vector<string> result;
        stringstream ss(recordString);
        string item;

        while (getline(ss, item, '#')) {
            result.push_back(item);
        }

        // Handle the trailing newline if present
        if (!result.empty() && !result.back().empty() && result.back().back() == '\n') {
            result.back().erase(result.back().size() - 1);
        }

        cout << "stringToVector end" << endl;
        return result;
    }


class StorageBufferManager {

    public:
        // initialize the  block size allowed in main memory according to the question 
        const static int BLOCK_SIZE = 4096; 
        const static int maxPages = 3; // 3 pages in main memory at most 
        fstream EmployeeRelation;
        // You may declare variables based on your need 
        int numRecords; // number of records in the file
        int pagesWrittenToFile = 0; // number of pages written to file. Track so that written pages are indexed by page number.
        tuple<std::vector<int>, unsigned long long, unsigned long long, unsigned long long> static initializationResults;
        int static maxPagesOnDisk;
    


        StorageBufferManager(string NewFileName) { 
            // cout << "StorageBufferManager constructor begin" << endl;

            //initialize your variables
            int maxPages = 3; // 3 pages in main memory at most 
            /*
                This variable contains:
                    offset array of size maxRecords, filled with 0's
                    the size of the offsetArray
                    the total count of all records
                    the max size of record, used later to calc min number of pages needed
            
            */
            initializationResults = initializeValues();
            
            // cout << "StorageBufferManager constructor end" << endl;
        };
        

        // Function to initialize the values of the variables
        /*
        Returns tuple containing: 
            offset array of size maxRecords, filled with 0's
            the size of the offsetArray
            ??? the minimum number of pages required on disk file: 

        */
        struct FileHeader {
            int totalNumberOfPages; // Total pages in the file
            int pageDirectoryOffset; // Offset where the page directory starts
            int pageDirectorySize; // Number of entries in the page directory; optional if dynamic
            // Additional metadata as needed, such as record schema information

            // Default constructor
            FileHeader() : totalNumberOfPages(0), pageDirectoryOffset(sizeof(FileHeader)), pageDirectorySize(0) {}
        };

        struct PageDirectoryEntry {
            int pageOffset; // Offset of the page from the beginning of the file
            int recordsInPage; // Number of records within the page

            // Default constructor initializes members to indicate an empty or uninitialized entry
            PageDirectoryEntry() : pageOffset(-1), recordsInPage(0) {}
            PageDirectoryEntry(int offset, int records) : pageOffset(offset), recordsInPage(records) {}
        };

        struct PageDirectory {
            std::vector<PageDirectoryEntry> entries; // Holds entries for each page
            int entryCount; // Keep track of valid entries so far. Used for add indexing
            int nextPageDirectoryOffset; // File offset to the next directory, or -1 if this is the last

            // Default constructor
            PageDirectory() : nextPageDirectoryOffset(-1), entries(100), entryCount(0) {}

            // Add a new page directory entry
            void addPageDirectoryEntry(int offset, int records) {
                if (entryCount >= entries.size()) {
                    cerr << "Error: Page directory is full. Cannot add new entry.\n" <<
                    "Create and link new page directory instead.\n";
                    return;
                }
                entries[entryCount].pageOffset = offset;
                entries[entryCount].recordsInPage = records;
                entryCount++;
            }

            // Function to serialize the PageDirectory to a file
            void serialize(std::ofstream& file) {
                // Write the entry count and next page directory offset first
                file.write(reinterpret_cast<const char*>(&entryCount), sizeof(entryCount));
                file.write(reinterpret_cast<const char*>(&nextPageDirectoryOffset), sizeof(nextPageDirectoryOffset));

                // Then write each entry
                for (const auto& entry : entries) {
                    file.write(reinterpret_cast<const char*>(&entry.pageOffset), sizeof(entry.pageOffset));
                    file.write(reinterpret_cast<const char*>(&entry.recordsInPage), sizeof(entry.recordsInPage));
                }
            }

            // Function to deserialize the PageDirectory from a file
            void deserialize(std::ifstream& file) {
                // Read the entry count and next page directory offset first
                file.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
                file.read(reinterpret_cast<char*>(&nextPageDirectoryOffset), sizeof(nextPageDirectoryOffset));

                // Resize entries vector based on entryCount
                entries.resize(entryCount);

                // Then read each entry
                for (auto& entry : entries) {
                    file.read(reinterpret_cast<char*>(&entry.pageOffset), sizeof(entry.pageOffset));
                    file.read(reinterpret_cast<char*>(&entry.recordsInPage), sizeof(entry.recordsInPage));
                }
            }

            // Function to update the next directory offset
            void setNextPageDirectoryOffset(int offset) {
                nextPageDirectoryOffset = offset;
            }
        };

         tuple<std::vector<int>, unsigned long long, unsigned long long, unsigned long long> static initializeValues() {
                    cout << "initializeValues begin" << endl;

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
                    //int minPages = (BLOCK_SIZE - static_cast<unsigned long long>(maxRecords) * sizeof(int) - sizeof(Page::PageHeader)) / maxRecordSize;
                    // Returns tuple containing offset array of size maxRecords, filled with 0's, and the size of the array
                    //         the total count of all records
                    //         the max size of record, used later to calc min number of pages needed
                    // cout << "initializeValues end" << endl;
                    return make_tuple(std::vector<int>(maxRecords, -1), static_cast<unsigned long long>(maxRecords) * sizeof(int), static_cast<unsigned long long>(fileCount), 
                    static_cast<unsigned long long>(maxRecordSize));


                };

    class Page {
        static const int page_size = BLOCK_SIZE; // Define the size of each page (4KB)
        int pageNumber; // Identifier for the page
        Page *nextPage; // Pointer to the next page in the list
        // These are static members of the Page class because the offsetArray and dataVector sizes will be shared by all instances of the Page class
        int static offsetArraySize;
        int static dataVectorSize;
        static const char sentinelValue = '\0'; // Sentinel value to indicate empty space in the data vector
        vector<int> offsetArray; // Vector to store the offsets of the records in the page
        vector<char> data; // Vector to store the data in the page

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
        int dataSize(const std::vector<char>& data, char sentinelValue) {
            //cout << "dataSize begin" << endl;
            return std::count_if(data.begin(), data.end(), [sentinelValue](char value) {
            return value != sentinelValue;
            });
            //cout << "dataSize end" << endl;
        }

        bool offsetArrayEmpty() {
            for (int i = 0; i < offsetArray.size(); i++) {
                if (offsetArray[i] != -1) {
                    return false;
                }
            }
            return true;
        }

        

    public:
        struct PageHeader {
            int recordsInPage; // Number of records in the page
            int spaceRemaining; // Updated to be initialized later in Page constructor

            PageHeader() : recordsInPage(0) {} // Constructor simplified
        } pageHeader;

        // Constructor for the Page class
        Page(int pageNum) : pageNumber(pageNum), nextPage(nullptr) {
            cout << "Page constructor begin" << endl;

            // Initialize the offsetArray to its maximum potential size first; sentinel value is -1
            offsetArray = get<0>(initializationResults); // Instance-specific offsetArray is initialized with 0's
            cout << "Value of offsetArray[0]: " << offsetArray[0] << endl;

            
            // size of offset array, 2nd element of tuple returned by initializeValues
            offsetArraySize = get<1>(initializationResults); // Update this based on actual logic
            cout << "Value of offsetArraySize: " << offsetArraySize << endl;

            // Size of data vector; calculated based on the size of the offset array and the size of the page header
            dataVectorSize = page_size - offsetArraySize - sizeof(PageHeader);
            cout << "Value of dataVectorSize: " << dataVectorSize << endl;
            // Initialize data vector to its maximum potential size first; sentinel value is -1
            data.resize(dataVectorSize, sentinelValue);
            cout << "Value of data[0]: " << data[0] << endl;
            cout << "Size of resized data vector: " << data.size() << endl;
            // Initialize space remaining in the page
            pageHeader.spaceRemaining = dataVectorSize;
            cout << "Value of spaceRemaining: " << pageHeader.spaceRemaining << endl;
            cout << "Ensure all members are initialized correctly\n";
            cout << "Page constructor end" << endl;
        }

        // Returns the size of the offset array by count non-sentinel values
        // Does not modify offsetArray
        int offsetSize() {
            int count = std::count_if(offsetArray.begin(), offsetArray.end(), [](int value) {
                return value != -1;
            });
            return count;
        }

        int dataSize(const std::vector<char>& data) {
            int count = std::count_if(data.begin(), data.end(), [](int value) {
                return value != sentinelValue;
            });
            //cout << "Elements in data array: " << count << endl;
            return count;
        }

        int calcSpaceRemaining() {
            cout << "calcSpaceRemaining entered:" << endl;
            // Calculate the space remaining based on current usage. PageHeader, page_size and offsetArraySize are all static members/fixed
            int newSpaceRemaining = page_size - dataSize(data, sentinelValue) - offsetArraySize - sizeof(PageHeader);
            cout << "calcSpaceRemaining::newSpaceRemaining: " << newSpaceRemaining << endl;
            return newSpaceRemaining;
        }


            // Getters
            int getPageNumber() {return pageNumber;}
            Page * getNextPage() {return nextPage;}
            int getDataSize() {return data.size();}
            int getOffsetArraySize() {return offsetArraySize;}
            int getDataVectorSize() {return dataVectorSize;}
            bool checkDataEmpty() {return data.empty();}
            // Getter method to return the next page in the list
            Page * goToNextPage() {
                return this->nextPage;
            }
            

            // Setters
            // Reinitialize data vector elements to sentinel value '\0'
            void emptyData() {fill(data.begin(), data.end(), sentinelValue);}
            // Reinitialize offsetArray elements to sentinel value -1
            void emptyOffsetArray() {fill(offsetArray.begin(), offsetArray.end(), -1);}
            // Method to link this page to the next page
            void setNextPage(Page* nextPage) {
                this->nextPage = nextPage;
            }

            

            // Test function to print the contents of a page as indexed by their offsets
            void printPageContentsByOffset() {

                // Check if page is empty. If so simply return
                if (offsetArrayEmpty() && dataVectorEmpty()) {
                    cout << "Page is empty. No records to print.\n";
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
                cout << "printPageContentsByOffset begin" << endl;
                cout << "Page Number: " << pageNumber << endl;
                cout << "Records in Page: " << pageHeader.recordsInPage << endl;
                cout << "Space Remaining: " << pageHeader.spaceRemaining << endl;
                cout << "Offset\t\tBeginning of record\t\tRecord Size\n";

                int elementsInOffsetArray = offsetSize();
                cout << "elements in offset array: " << elementsInOffsetArray << endl;

                for (size_t i = 0; i <  elementsInOffsetArray; ++i) {
                    // Validate the current offset
                    if (offsetArray[i] < 0 || offsetArray[i] >= static_cast<int>(dataSize(data))) {
                        cerr << "Error: Invalid offset " << offsetArray[i] << " at offsetArray index " << i << ". Skipping record.\n";
                        continue;
                    }

                    // Calculate the start and end offsets for the current record
                    int startOffset = offsetArray[i];
                    int endOffset = (i + 1 < elementsInOffsetArray) ? offsetArray[i + 1] : dataSize(data);

                    if (endOffset == dataSize(data)) { cout << "endOffset currently equal to dataSize(data)" << dataSize(data) << endl;}

                    // Validate the end offset
                    if (endOffset > static_cast<int>(dataSize(data))) {
                        cerr << "Error: End offset " << endOffset << " exceeds data vector size. Adjusting to data size.\n";
                        endOffset = dataSize(data);
                    }

                    // Print the offset in hexadecimal and the record's contents
                    cout << "0x" << setw(3) << setfill('0') << hex << startOffset << "\t";
                    for (int j = startOffset; j < endOffset; ++j) {
                        // Ensure printing of printable characters only
                        cout << (isprint(data[j]) ? data[j] : '.');
                    }
                    
                    cout << "\t\t" << dec << endOffset - startOffset << "\n\n";
                    cout << "endoffset: " << endOffset << "\nstartOffset: "<< startOffset << endl;
                }
                cout << "printPageContentsByOffset end" << endl;
            }

            // Method to find the offset of the next record in the data vector
            // Searches for the last non-sentinel character and returns the next position
            int findOffsetOfNextRecord(const std::vector<char>& data, char sentinelValue) {
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

            // Method to add an offset to the end of offsetArray
            // Bespoke method due to array being filled with sentinel values
            bool addOffsetToFirstSentinel(std::vector<int>& offsetArray, int newOffset) {
                for (size_t i = 0; i < offsetArray.size(); ++i) {
                    if (offsetArray[i] == -1) { // Sentinel value found
                        offsetArray[i] = newOffset; // Replace with new offset
                        return true; // Indicate success
                    }
                }
                return false; // Indicate failure (no sentinel value found)
            }

            // Method to add a record to the page
            bool addRecord(const Record& record) {
                cout << "addRecord begin" << endl;
                bool offsetAdd = false;
                auto recordString = record.toString();

                // This should print out a properly formatted record string with not trailing periods
                cout << "addRecord:: TEST PRINT: Record string: " << recordString << "\n";

                size_t recordSize = recordString.size();
                
                // Check if there's enough space left in the page
                if (recordSize > static_cast<size_t>(pageHeader.spaceRemaining)) {
                    std::cerr << "addRecord::Error: Not enough space in the page to add the record.\n";
                    return false;
                }
                
                // Calculate the offset for the new record
                int offsetOfNextRecord = findOffsetOfNextRecord(data, sentinelValue);

                // Ensure the insertion does not exceed the vector's predefined max size
                if (offsetOfNextRecord + recordSize <= data.size()) {
                    cout << "addRecord:: Adding record to page: " << this->pageNumber << "\n";
                    std::copy(recordString.begin(), recordString.end(), data.begin() + offsetOfNextRecord);
                    pageHeader.recordsInPage += 1;
                    cout << "addRecord:: spaceRemaining - recordSize: " << pageHeader.spaceRemaining << " - " << recordSize << " = " << pageHeader.spaceRemaining - recordSize << endl;
                    pageHeader.spaceRemaining -= recordSize;
                    //offsetArray.push_back(offsetOfNextRecord);

                    if (addOffsetToFirstSentinel(offsetArray, offsetOfNextRecord)) {
                        cout << "addRecord:: Offset added to offsetArray\n";
                    } else {
                        std::cerr << "addRecord:: Error: Unable to add offset to offsetArray.\n";
                        return false;
                    }

                    cout << "addRecord::Test: Confirm record added to page:\nPrinting stored record from main memory: \n";
                    
                    cout << "0x" << setw(3) << setfill('0') << hex << offsetArray.back() << "\t" << dec;
                    for (int i = offsetOfNextRecord; i < offsetOfNextRecord + recordSize; ++i) {
                        cout << data[i];
                    }

                    return true;

                } else if (offsetOfNextRecord + recordSize > data.size()) {
                    std::cerr << "addRecord:: Error: Attempt to exceed predefined max size of data vector.\n";
                    cout << "addRecord failed" << endl;
                    return false;
                } else {
                    std::cerr << "addRecord:: Error: Unknown error occurred while adding record to page.\n";
                    cout << "addRecord failed" << endl;
                    return false;
                }
                cout << "addRecord successful" << endl;
            }

            // Method to write the contents of this page to a file
            int writeRecordsToFile(std::ofstream& outputFile, int offsetSize, int pageID) {
                if (!outputFile.is_open()) {
                    std::cerr << "Error: Output file is not open for writing.\n";
                    return -1; // Indicate error
                }

               

                for (int i = 0; i < offsetSize - 1; ++i) { // -1 to prevent accessing beyond the last valid index
                    int startOffset = offsetArray[i];
                    int endOffset = offsetArray[i + 1]; // Get the end offset for the current segment

                    // Write each character in the range [startOffset, endOffset) to the file
                    for (int j = startOffset; j < endOffset; ++j) {
                        if (data[j] != sentinelValue) {
                            outputFile.write(reinterpret_cast<const char*>(&data[j]), sizeof(data[j]));
                        }
                    }
                }

                // Optionally, return the new offset after writing, if needed
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
                cout << "PageList constructor begin" << endl;
                head = initializePageList();
                cout << "PageList constructor end" << endl;
            }

            // Create 'maxPages' number of pages and link them together
            // Initialization of head member attirbutes done in head constructor
            Page* initializePageList() {
                cout << "initializePageList begin" << endl;
                Page *tail = nullptr;
                for (int i = 0; i < maxPages; ++i) {
                    cout << "Creating page " << i << endl;
                    Page *newPage = new Page(i); // Create a new page
                    cout << "Page " << i << " created" << endl;

                    if (!head) {
                        cout << "Setting head to page " << i << endl;
                        head = newPage; // If it's the first page, set it as head
                    } else {  
                        cout << "Linking page " << i << " to page " << i - 1 << endl;
                        tail->setNextPage(newPage); // Otherwise, link it to the last page
                    }
                    cout << "Setting tail to page " << i << endl;
                    tail = newPage; // Update the tail to the new page
                }
                cout << "initializePageList end" << endl;
                return head; // Return the head of the list
            };

              // Destructor for the PageList class
            ~PageList() {
                cout << "PageList destructor begin" << endl;
                Page* current = head;
                while (current != nullptr) {
                    Page* temp = current->getNextPage(); // Assuming nextPage points to the next Page in the list
                    delete current; // Free the memory of the current Page
                    current = temp; // Move to the next Page
                };
                cout << "PageList destructor end" << endl;
            };

            // Method to reset the data and offsetArray of each page in the list to initial values
            void resetPages() {
                cout << "resetPage begin" << endl;
                Page * current = head;
                while (current != nullptr) {
                    current->emptyData();
                    current->emptyOffsetArray();
                    current->pageHeader.recordsInPage = 0;
                    current->pageHeader.spaceRemaining = current->calcSpaceRemaining();
                    current = current->getNextPage();
                }
                cout << "resetPage end" << endl;
            }

            bool dumpPages(ofstream& file, int& pagesWrittenToFile) {
                cout << "Dumping pages to file...\n";

                if (!file.is_open()) {
                    cerr << "File is not open for writing.\n";
                    return false;
                }

                Page* currentPage = head;
                int pageOffset = 0;
                while (currentPage != nullptr) {
                    // Assume currentPage->writeRecordsToFile() handles serialization of page data
                    if (!currentPage->writeRecordsToFile(file, currentPage->offsetSize(), currentPage->getPageNumber() {
                        cerr << "Failed to write page records to file.\n";
                        return false;
                    }

                    // Update the page directory with new entry
                    int currentPageOffset = /* Calculate current page offset in the file */;
                    int currentPageRecordCount = currentPage->getRecordCount();
                    // Add the new directory entry
                    // Assuming you have a method to handle adding and serializing the page directory entry
                    addPageDirectoryEntry(currentPageOffset, currentPageRecordCount);

                    pagesWrittenToFile++;
                    currentPage = currentPage->getNextPage();
                }

                // Serialize and write the updated page directory and file header here
                // Reset the pages for reuse
                resetPages();
                cout << "Page dumping complete.\n";
                return true;
            }

            

            /*
            Test function: Use to confirm page is being filled properly. 
            */
            void printMainMemory() {
                cout << "Printing main memory contents...\n";
                Page* page = head;
                while (page != nullptr) {
                    // Print the page number as a header for each page's content
                    cout << "\n\nPrinting contents of Page Number: " << page->getPageNumber() << endl;
                    // Now, use the printPageContentsByOffset method to print the contents of the current page
                    page->printPageContentsByOffset();
                    // Move to the next page in the list
                    page = page->getNextPage();
                }
                cout << "End of main memory contents.\n";
            }

        }; // End of PageList definition

        // TODO: Implement this function
        /*
        reserve space for file header
        reserve space of page directory. 
        store relevant info in file header
        */
        void setMaxPagesOnDisk() {
            // Assuming:
            // BLOCK_SIZE is the size of a page in bytes.
            // sizeof(Page::PageHeader) gives the static size of the page header.
            // get<1>(initializationResults) returns the total size of the offset array in bytes.
            // get<2>(initializationResults) provides the size of the largest record.

            StorageBufferManager::maxPagesOnDisk = (BLOCK_SIZE - sizeof(Page::PageHeader) - get<1>(initializationResults)) / get<2>(initializationResults);

        };

        void initializeDataFile(const std::string& filename) {
            // Clear file
            std::ofstream file(filename, std::ios::binary | std::ios::out | std::ios::trunc);

            // Error check: If file cannot be opened, print error and exit
            if (!file.is_open()) {
                std::cerr << "Unable to open file for initialization: " << filename << std::endl;
                return;
            }

            // Write an empty file header
            FileHeader header;
            PageDirectory pageDirectory;
            header.pageDirectoryOffset = sizeof(header);
            file.write(reinterpret_cast<const char*>(&header), sizeof(header));
            file.write(reinterpret_cast<const char*>(&pageDirectory), sizeof(pageDirectory));

            file.close();
        }



        // Create record from csv line
        Record createRecord(string line) {
            cout << "createRecord begin" << endl;
            // Split line into fields
            vector<string> fields;
            stringstream ss(line);
            string field;
            while (getline(ss, field, ',')) {
                fields.push_back(field);
            }
            cout << "createRecord end" << endl;
            return Record(fields);
        };

        


        void exitProgram(ofstream &EmpStream) {
            if (EmpStream.is_open()) {
                EmpStream.close();
            }
        };

        
        // Read csv file (Employee.csv) and add records to the (EmployeeRelation)
        void createFromFile(string csvFName) {
            cout << "CreateFromFile: Begin createFromFile...\n";

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
            initializeDataFile("EmployeeRelation.dat", EmployeeRelation, maxPagesOnDisk);
            
            // Buffer for reading line from csv: equal to max possible record size plus 1
            char buffer[2 * 8 + 200 + 500 + 1];

            // Loop through records
            if (fileStream.is_open()) {
                cout << "CreateFromFile: File opened...\n\n\n";
                // Get each record as a line
                Page * currentPage = pageList->head;
                while (getline(fileStream, line)) {
                    cout << "CreateFromFile: Reading line from file...\n";
                    // Create record from line 
                    Record record = createRecord(line);
                    cout << "CreateFromFile: Record created from line...\nTest print: \n";
                    record.print();
                    
                    // Get the size of the stringified record
                    // Stringified record is the data that will actually be added to memory
                    recordSize = record.toString().size();
                    cout << "CreateFromFile: Record size: " << recordSize << endl;

                    // Check that record size is less than block size
                    if (recordSize > currentPage->getDataVectorSize()) {
                        cerr << "Record size exceeds block size.\n";
                        exit(1);
                    }
                    cout << "CreateFromFile:: Check passed:  Record size is less than size of currPage dataVector...\n";
                    
                    // determine if there's enough room on current page:
                        // if not and not last page, begin writing to next page
                        // if not and last page, write page to file and empty page
                    spaceRemaining = currentPage->calcSpaceRemaining();
                    cout << "CreateFromFile: Space remaining on current page: " << spaceRemaining << endl;

                    // While current page full and page not last page
                    while (spaceRemaining < recordSize && currentPage->getPageNumber() < maxPages - 1) {
                        cout << "CreateFromFile:: Current page full; not last page.\n"<<
                        "VERIFY: Current page space Remaining: " << spaceRemaining << " < Record Size: " << recordSize << 
                        "\nMoving from page " << currentPage->getPageNumber() << " to page " << currentPage->getPageNumber() + 1 << "\n";
                        cout <<  endl;
                        // Advance to next page
                        currentPage = currentPage->goToNextPage();
                        addFlag = currentPage->addRecord(record);
                        spaceRemaining = currentPage->calcSpaceRemaining();
                        if (addFlag == false) {
                            cerr << "Size mismatch on NEXT PAGE ADD. Terminating..." << endl;
                            exit(-1);
                        }
                    }
                    // Main memory full: no room for record on any pages
                    // Write contents to file, then 
                    if (spaceRemaining < recordSize && currentPage->getPageNumber() == maxPages - 1) {
                        cout << "CreateFromFile: Main memory full. Test printing contents...\n";
                        pageList->printMainMemory();
                        // Write page to file
                        //dumpFlag = dumpPages(EmployeeRelation, pagesWrittenToFile);
                        //if (dumpFlag == false) {
                        //    cerr << "Failure to copy main memory contents to file. Terminating..." << endl;
                        //    exit(-1);
                        //}
                        
                        pageList->dumpPages(EmployeeRelation, pagesWrittenToFile);
                        currentPage = pageList->head;
                    }
                    // There's room on the current page. Add record
                    // If returns false, some kind of logic error to resolve
                    else {
                        // Add record to page
                        cout << "CreateFromFile: Adding record to page...\n";
                        addFlag = currentPage->addRecord(record);
                        cout << "CreateFromFile: Record added to page?\n";
                        if (addFlag == false) {
                            cerr << "Size mismatch. Terminating..." << endl;
                            exit(-1);
                        }
                    }            
                }

                // After getline finishes, if page is not empty, write to file
                // Situation: final records, don't fill entire page
                if (!currentPage->checkDataEmpty()) {
                    cout << "CreateFromFile: Test printing last page...\n";
                    pageList->printMainMemory();
                    /*
                    dumpFlag = dumpPages(EmployeeRelation);
                    if (dumpFlag == false) {
                            cerr << "Failure to copy main memory contents to file. Terminating..." << endl;
                            exit(-1);
                    */
                    pageList->dumpPages(EmployeeRelation, pagesWrittenToFile);
                    currentPage = pageList->head;
                }
                delete pageList;
            };

        Record stringToRecord(const std::string& recordStr) {
            // Split the recordStr by field delimiters and construct a Record object
            // Example implementation detail depends on the exact format
            cout << endl;
        };

        /*
        void printStructuredFileContents(const std::string& filePath) {
            std::ifstream file(filePath, std::ios::binary);
            if (!file) {
                std::cerr << "Cannot open file: " << filePath << std::endl;
                return;
            }

            // Assuming the file starts with the size of the page directory (e.g., an integer)
            int pageDirectorySize;
            file.read(reinterpret_cast<char*>(&pageDirectorySize), sizeof(pageDirectorySize));

            // Read the page directory to get page offsets
            std::vector<int> pageOffsets(pageDirectorySize);
            file.read(reinterpret_cast<char*>(pageOffsets.data()), pageDirectorySize * sizeof(int));

            // Iterate over each page using the offsets
            for (int pageOffset : pageOffsets) {
                file.seekg(pageOffset, std::ios::beg);

                // Read page metadata here (e.g., number of records in the page)
                // For simplicity, let's assume we just start reading records

                // Example: read the size of the record vector for the page
                int recordVectorSize;
                file.read(reinterpret_cast<char*>(&recordVectorSize), sizeof(recordVectorSize));

                std::vector<int> recordOffsets(recordVectorSize);
                file.read(reinterpret_cast<char*>(recordOffsets.data()), recordVectorSize * sizeof(int));

                // Now print each record by seeking to its offset within the page
                for (int recordOffset : recordOffsets) {
                    // Seek to the record's position within the current page
                    file.seekg(pageOffset + recordOffset, std::ios::beg);

                    // Assuming records are terminated by a newline character for simplicity
                    std::string record;
                    std::getline(file, record);
                    std::cout << record << std::endl;
                }
            }

            file.close();
        } */



        std::vector<Record> readRecordsFromFileAndSearchByID(const std::string& filePath, int searchID) {
            std::ifstream inputFile(filePath, std::ios::binary | std::ios::in);
            std::vector<Record> matchingRecords;
            if (!inputFile) {
                std::cerr << "Error opening file for reading.\n";
                return matchingRecords;
            }

            std::string line;
            while (std::getline(inputFile, line)) {
                if (line.empty()) continue; // Skip empty lines or end of file markers
                
                Record record = stringToRecord(line); // Deserialize string to record
                if (record.id == searchID) {
                    matchingRecords.push_back(record);
                }
            }

            inputFile.close();
            return matchingRecords;
        };


            /*
            int getRecordCount(string filename) {
                ifstream fileStream(filename);
                string line;
                int count = 0;
                if (fileStream.is_open()) {
                    while (getline(fileStream, line)) {
                        count++;
                    }
                }
                return count;
            }

            // Given an ID, find the relevant record and print it
            Record findRecordById(int id) {
                // rid = page id, offset 
                ifstream relationStream("EmployeeRelation.dat");
                string line;
                if (relationStream.is_open()) {
                    while (getline(relationStream, line)) {
                        // Split line into fields
                        vector<string> fields;
                        stringstream ss(line);
                        string field;
                        while (getline(ss, field, ',')) {
                            fields.push_back(field);
                        }
                        // Check if ID matches
                        if (stoi(fields[0]) == id) {
                            // Return record
                            return &Record(fields).toString();
                        }
                    }
                }
                
            }
        } */
    }; 
};    // End of StorageBufferManager definition