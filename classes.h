#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <bitset>
#include <algorithm>
#include <fstream>
#include <tuple>
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


class StorageBufferManager {

    private:
        
        const static int BLOCK_SIZE = 4096; // initialize the  block size allowed in main memory according to the question 
        const static int maxPages = 3; // 3 pages in main memory at most 
        ofstream EmployeeRelation;
        // You may declare variables based on your need 
        int numRecords; // number of records in the file

        tuple<std::vector<int>, unsigned long long> static initializeOffsetArray() {

                    int minBioLen = INT_MAX;
                    int minNameLen = INT_MAX;

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

                            if (fields[1].length() < minNameLen) {
                                minNameLen = fields[1].length();
                            }
                            if (fields[2].length() < minBioLen) {
                                minBioLen = fields[2].length();
                            }
                        }
                    }

                    // Minimize size of records: ID, name, bio, manager_id and then an 8 byte offset for each
                    // offset could end up being only 4 bytes; test/check
                    int minRecordSize = 3 * 8 + minNameLen + minBioLen;
                    int maxRecords = (BLOCK_SIZE) / minRecordSize;
                    // Returns tuple containing offset array of size maxRecords, filled with 0's, and the size of the array
                    return make_tuple(std::vector<int>(maxRecords, 0), static_cast<unsigned long long>(maxRecords) * sizeof(int));

                };

        class Page {
            
            static const int BLOCK_SIZE = 4096; // Define the size of each page (4KB)

            struct PageHeader {
                friend class PageList;
                int recordsInPage = 0;
                int spaceRemaining;

                // Constructor to initialize a new page with no records and full space
                PageHeader() : recordsInPage(0), spaceRemaining(BLOCK_SIZE) {}; // Assuming 4096 bytes per page
            };

            int pageNumber; // Identifier for the page
            PageHeader pageHeader;
            Page *nextPage; // Pointer to the next page in the list
            vector<char> data; // Vector to store the data in the page
            vector<int> offsetArray; // Vector to store the offsets of the records in the page
            unsigned long long static dataVectorSize;
            unsigned long long static offsetArraySize;

            
            public:
                // Constructor for the Page class: Full initialized at instantiation time
                Page(int pageNum) : pageNumber(pageNum), nextPage(nullptr) {
                    // Assuming 'initializeOffsetArray()' returns a std::tuple<std::vector<int>, unsigned long long>
                    auto resultTuple = initializeOffsetArray();
                    offsetArray = get<0>(resultTuple);
                    offsetArraySize = get<1>(resultTuple);

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

                // Method to calculate the space remaining on the current page
                int calcSpaceRemaining() {
                    return BLOCK_SIZE - data.size() - offsetArraySize - sizeof(PageHeader);
                }
                
                // Method to add a record to this page
                bool addRecord(const Record& record) {
                    
                    // Check if there is enough space left in the page
                    if (record.toString().size() > pageHeader.spaceRemaining) {
                        return false; // Not enough space, record not added
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

                // For testing:
                void printPageContents() {
                    // Print page header information
                    cout << "Records in Page: " << pageHeader.recordsInPage << endl;
                    cout << "Space Remaining: " << pageHeader.spaceRemaining << endl;

                    for (int elem: offsetArray){
                        cout << elem << " ";
                    }
                    cout << endl;

                    for (char elem : data) {
                        cout << elem;
                    }
                    cout << endl;
                }

                /*
                // Method to dump (write) the current page's data to a file
                void dumpPage(const std::string& filename) {
                    std::ofstream outFile(filename, std::ios::app); // Open the file in append mode
                    if (outFile.is_open()) {
                        // Write the data from the page to the file
                        outFile.write(data.data(), offset);
                        outFile.close(); // Close the file
                    } else {
                        std::cerr << "Unable to open file for writing.\n"; // Error handling
                    }
                } */
                }; // End of Page definition
                
                // Method to dump the data of this page and all subsequent pages to a file
                bool dumpPages(const std::string& filename) {
                    Page* currentPage = this; // Start with the current page
                    while(currentPage != nullptr) {
                        currentPage->dumpPage(filename); // Dump current page's data
                        currentPage = currentPage->nextPage; // Move to the next page
                    }
                }
                

        // Contains linked list of pages and necessary functions
        class PageList {
            public:
            Page *head;
            static const int maxPages = 3; 
            // Constructor for the PageList class
            // Head is initialized as page 0 and linked list created from it
            PageList() : head(new Page(0)) {
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
            }

            /*
            Test function: Use to confirm page is being filled properly. 
            */
            void printMainMemory() {
                Page * page = head;
                while (page->getPageNumber() < maxPages) {
                    // Use offset array to locate beginning of record in page memory
                    // Print out fields, formatted
                    // Print out remaining space on page
                    page = page->getNextPage();
                }
            }
        }; // End of PageList definition

        
    
    public:
        StorageBufferManager(string NewFileName) { 
            cout << "Hello" << endl;

            //initialize your variables
            int maxPages = 3; // 3 pages in main memory at most 

            // Create your EmployeeRelation.dat file 
            FILE * EmployeeRelation;
            EmployeeRelation = fopen(NewFileName.c_str(), "w");
        };

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
        }

        
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
                    if (recordSize > currentPage->dataVectorSize) {
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
                        // Write page to file
                        dumpFlag = dumpPages(EmployeeRelation);
                        if (dumpFlag == false) {
                            cerr << "Failure to copy main memory contents to file. Terminating..." << endl;
                            exit(-1);
                        }
                        currentPage = pageList->head;
                    }
                    /*
                    
                    Account for situation:
                    Last record, space remaining. 

                    */


                    // There's room on the current page. Add record
                    else {
                        // Add record to page
                        addFlag = currentPage->addRecord(record);

                        if (addFlag == false){
                            cerr << "Size mismatch. Terminating..." << endl;
                            exit(-1);
                        }
                    }            
                }
                if (!currentPage->data.empty()){
                    dumpFlag = dumpPages(EmployeeRelation);
                    if (dumpFlag == false) {
                            cerr << "Failure to copy main memory contents to file. Terminating..." << endl;
                            exit(-1);
                }
            }

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
                            return Record(fields);
                        }
                    }
                }
                
            }
        } */

        void exitProgram() {
            if (EmployeeRelation.is_open()) {
                EmployeeRelation.close();
            }
            
        }
    }; 

    
