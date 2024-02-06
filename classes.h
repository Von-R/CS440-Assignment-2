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

    string toString() const {
            return to_string(id) + "," + name + "," + bio + "," + to_string(manager_id) + "\n";
        }

    
};

// Reconstruct fields vector from stringified records on file
// Used to reconstruct records from file
vector<string> stringToVector(const string& recordString) {
        vector<string> result;
        stringstream ss(recordString);
        string item;

        while (getline(ss, item, ',')) {
            result.push_back(item);
        }

        // Handle the trailing newline if present
        if (!result.empty() && !result.back().empty() && result.back().back() == '\n') {
            result.back().erase(result.back().size() - 1);
        }

        return result;
    }


class StorageBufferManager {

    public:
        // initialize the  block size allowed in main memory according to the question 
        const static int BLOCK_SIZE = 4096; 
        const static int maxPages = 3; // 3 pages in main memory at most 
        ofstream EmployeeRelation;
        // You may declare variables based on your need 
        int numRecords; // number of records in the file
        int pagesWrittenToFile = 0; // number of pages written to file. Track so that written pages are indexed by page number.
        tuple<std::vector<int>, unsigned long long, unsigned long long, unsigned long long> static initializationResults;
        int static maxPagesOnDisk;


        StorageBufferManager(string NewFileName) { 
            cout << "Hello" << endl;

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
            
          
            // Create your EmployeeRelation.dat file 
            FILE * EmployeeRelation;
            EmployeeRelation = fopen(NewFileName.c_str(), "w");
        };
        

        // Function to initialize the values of the variables
        /*
        Returns tuple containing: 
            offset array of size maxRecords, filled with 0's
            the size of the offsetArray
            ??? the minimum number of pages required on disk file: 

        */
         tuple<std::vector<int>, unsigned long long, unsigned long long, unsigned long long> static initializeValues() {

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
                    return make_tuple(std::vector<int>(maxRecords, 0), static_cast<unsigned long long>(maxRecords) * sizeof(int), static_cast<unsigned long long>(fileCount), 
                    static_cast<unsigned long long>(maxRecordSize));


                };

        

        class Page {
            
            static const int page_size = BLOCK_SIZE; // Define the size of each page (4KB)
            int pageNumber; // Identifier for the page
            Page *nextPage; // Pointer to the next page in the list
            vector<char> data; // Vector to store the data in the page
            vector<int> static offsetArray; // Vector to store the offsets of the records in the page
            unsigned long long static dataVectorSize;
            unsigned long long static offsetArraySize;

            
            public:

                struct PageHeader {
                    friend class PageList;
                    int recordsInPage = 0;
                    int spaceRemaining;

                    PageHeader() : recordsInPage(0), spaceRemaining(page_size - sizeof(PageHeader)) {
                        // Initialize spaceRemaining considering the header's own size
                    }

                    // This function is not storing the offset as a member variable but calculating it dynamically
                    // based on the current layout of the page.
                    static int calculateDataOffset() {
                        // Calculate the offset to the start of the data, assuming the data follows immediately after the PageHeader
                        // If there are additional fixed-size overheads before the data begins, add their sizes here as well.
                        return sizeof(PageHeader);
                    }
                };


                PageHeader pageHeader;

                // Constructor for the Page class: Full initialized at instantiation time
                Page(int pageNum) : pageNumber(pageNum), nextPage(nullptr) {
                    // Assuming 'initializeOffsetArray()' returns a std::tuple<std::vector<int>, unsigned long long>
                    
                    offsetArray = get<0>(initializationResults);
                    offsetArraySize = get<1>(initializationResults);

                    pageHeader = PageHeader(); // Assuming BLOCK_SIZE is known and set in PageHeader's constructor

                    // Now that 'offsetArraySize' and 'pageHeader' are initialized, calc size of data vector
                    // Resize data vector be correct size
                    dataVectorSize = BLOCK_SIZE - offsetArraySize - sizeof(PageHeader);
                    data.resize(dataVectorSize);
                };

                // Getters
                int getPageNumber() {return pageNumber;}
                Page * getNextPage() {return nextPage;}
                int getDataSize() {return data.size();}
                int getOffsetArraySize() {return offsetArraySize;}
                int getDataVectorSize() {return dataVectorSize;}
                bool checkDataEmpty() {return data.empty();}

                // Method to calculate the space remaining on the current page
                int calcSpaceRemaining() {
                    return BLOCK_SIZE - data.size() - offsetArraySize - sizeof(PageHeader);
                }

                // Setters
                void emptyData() {data.clear();}
                void emptyOffsetArray() {offsetArray.clear();}

                void resetPage() {
                    pageHeader.recordsInPage = 0;
                    pageHeader.spaceRemaining = BLOCK_SIZE;
                    data.clear();
                    offsetArray.clear();
                }

                // Test function to print the contents of a page as indexed by their offsets
                void printPageContentsByOffset() {
                    cout << "Page Number: " << pageNumber << endl;
                    cout << "Records in Page: " << pageHeader.recordsInPage << endl;
                    cout << "Space Remaining: " << pageHeader.spaceRemaining << endl;
                    cout << "Offset\t\tBeginning of record\t\tRecord Size\n";
                    for (size_t i = 0; i < offsetArray.size(); ++i) {
                        // Print the offset in hexadecimal
                        cout << "0x" << setw(3) << setfill('0') << hex << offsetArray[i] << "\t";
                        
                        // Determine the start and end of the current record
                        int startOffset = offsetArray[i];
                        int endOffset = (i + 1 < offsetArray.size()) ? offsetArray[i + 1] : data.size();
                        
                        // Print the record's contents from startOffset to endOffset
                        for (int j = startOffset; j < endOffset; ++j) {
                            cout << data[j];
                        }

                        cout << "\t" << dec << endOffset - startOffset << "\n"; // Print the size of the record
                        cout << "\n"; // Move to the next line after printing each record
                    }
                }
                
                // Method to add a record to this page
                bool addRecord(const Record& record) {
                    
                    // Check if there is enough space left in the page
                    if (record.toString().size() > pageHeader.spaceRemaining) {
                        cerr << "PAGE:: Not enough space in the page to add the record.\n";
                        return false;
                    }
                
                    int offsetOfNextRecord = data.size();
                    // Copy the record data into the page's data vector
                    if (offsetArray.empty()) {
                        copy(record.toString().begin(), record.toString().end(), data.begin());
                        pageHeader.recordsInPage += 1;
                        pageHeader.spaceRemaining -= record.toString().size();
                    } else {
                        copy(record.toString().begin(), record.toString().end(), data.begin() + offsetOfNextRecord);
                        pageHeader.recordsInPage += 1;
                        pageHeader.spaceRemaining -= record.toString().size();
                    }
                    this->offsetArray.push_back(offsetOfNextRecord);
                    
                    // Update the offset to the new end of the data
                    return true; // Record added successfully
                }

                // Setter method to link this page to the next page
                void setNextPage(Page* nextPage) {
                    this->nextPage = nextPage;
                }

                Page * goToNextPage() {
                    return this->nextPage;
                }

            }; // End of Page definition
                
                
        // Contains linked list of pages and necessary functions
        class PageList {
            public:
            Page *head;
            //static const int maxPages = 3; 
            // Constructor for the PageList class
            // Head is initialized as page 0 and linked list created from it
            PageList() {
                head = initializePageList();
            }

            // Create 'maxPages' number of pages and link them together
            // Initialization of head member attirbutes done in head constructor
            Page* initializePageList() {
                Page *tail = nullptr;
                for (int i = 0; i < maxPages; ++i) {
                    Page *newPage = new Page(i); // Create a new page
                    if (!head) {
                        head = newPage; // If it's the first page, set it as head
                    } else {  
                        tail->setNextPage(newPage); // Otherwise, link it to the last page
                    }
                    tail = newPage; // Update the tail to the new page
                }
                return head; // Return the head of the list
            };

              // Destructor for the PageList class
            ~PageList() {
                Page* current = head;
                while (current != nullptr) {
                    Page* temp = current->getNextPage(); // Assuming nextPage points to the next Page in the list
                    delete current; // Free the memory of the current Page
                    current = temp; // Move to the next Page
                };
            };

            // Method to dump the data of this page and all subsequent pages to a file
            bool dumpPages(const std::string& filename, int pagesWrittenToFile) {
                cout << "Dumping pages to file...\n";
                cout << "IMPLEMENTATION PENDING\n";
                Page *currentPage = head; // Start with the current page
                while(currentPage != nullptr) {
                    //currentPage->printPageContentsByOffset(); // Dump current page's data
                    /*
                        currentPage->pageHeader.pageNumber = pagesWrittenToFile;
                        pagesWrittenToFile++;
                        Write to file
                    */
                    currentPage->resetPage(); // Reset the page vectors and header
                    currentPage = currentPage->getNextPage(); // Move to the next page
                    }
                };
            

            /*
            Test function: Use to confirm page is being filled properly. 
            */
            void printMainMemory() {
                Page* page = head;
                while (page != nullptr) {
                    // Print the page number as a header for each page's content
                    cout << "Printing contents of Page Number: " << page->getPageNumber() << endl;
                    // Now, use the printPageContentsByOffset method to print the contents of the current page
                    page->printPageContentsByOffset();
                    // Move to the next page in the list
                    page = page->getNextPage();
                }
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

        FILE * initializeDataFile(string filename) {
            FILE * dataFile;
            dataFile = fopen(filename.c_str(), "w");
            if (dataFile == NULL) {
                cerr << "Error: Unable to open file for writing.\n";
                exit(1);
            }

        /*
        TODO:
        Create page directory at end of page. Use maxPagesOnDisk to determine how many pages to reserve for directory.
        What else?
        */
        
            return dataFile;
        }

        // Create record from csv line
        Record createRecord(string line) {
            // Split line into fields
            vector<string> fields;
            stringstream ss(line);
            string field;
            while (getline(ss, field, ',')) {
                fields.push_back(field);
            }
            return Record(fields);
        };

          void exitProgram(ofstream &EmpStream) {
            if (EmpStream.is_open()) {
                EmpStream.close();
            }
            
        };

        
        // Read csv file (Employee.csv) and add records to the (EmployeeRelation)
        void createFromFile(string csvFName) {

            // initialize variables
            // Offset of record
            int offset = 0;
            // For reading line from csv
            string line;
            bool addFlag = false;
            bool dumpFlag = false;
            
            int spaceRemaining;
            
            // Size of record
            int recordSize = 0;

            // Initialize pages and create pointer to first page
            PageList * pageList = new PageList();
            
            // Open source file for reading source csv
            ifstream fileStream(csvFName.c_str());

            // Buffer for reading line from csv: equal to max possible record size plus 1
            char buffer[2* 8 + 200 + 500 + 1];

            // Loop through records
            if (fileStream.is_open()) {
                // Get each record as a line
                Page * currentPage = pageList->head;
                while (getline(fileStream, line)) {
                    // Create record from line 
                    Record record = createRecord(line);
                    // Get the size of the record
                    recordSize = record.recordSize();

                    // Error check that record size is less than block size
                    if (recordSize > currentPage->getDataVectorSize()) {
                        cerr << "Record size exceeds block size.\n";
                        exit(1);
                    }
                    
                    // determine if there's enough room on current page:
                        // if not and not last page, begin writing to next page
                        // if not and last page, write page to file and empty page
                    spaceRemaining = currentPage->calcSpaceRemaining();

                    // While current page full and page not last page
                    while (spaceRemaining < recordSize && currentPage->getPageNumber() < maxPages - 1) {
                        // Advance to next page
                        currentPage = currentPage->goToNextPage();
                    }
                    // Main memory full: no room for record on any pages
                    // Write contents to file, then 
                    if (spaceRemaining < recordSize && currentPage->getPageNumber() == maxPages - 1) {
                        pageList->printMainMemory();
                        // Write page to file
                        //dumpFlag = dumpPages(EmployeeRelation, pagesWrittenToFile);
                        //if (dumpFlag == false) {
                        //    cerr << "Failure to copy main memory contents to file. Terminating..." << endl;
                        //    exit(-1);
                        //}
                        currentPage = pageList->head;
                    }
                    // There's room on the current page. Add record
                    // If returns false, some kind of logic error to resolve
                    else {
                        // Add record to page
                        addFlag = currentPage->addRecord(record);
                        if (addFlag == false) {
                            cerr << "Size mismatch. Terminating..." << endl;
                            exit(-1);
                        }
                    }            
                }

                // After getline finishes, if page is not empty, write to file
                if (!currentPage->checkDataEmpty()) {
                    pageList->printMainMemory();
                    /*
                    dumpFlag = dumpPages(EmployeeRelation);
                    if (dumpFlag == false) {
                            cerr << "Failure to copy main memory contents to file. Terminating..." << endl;
                            exit(-1);
                    */
                }
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