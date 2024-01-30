#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <algorithm>
using namespace std;

class Record {
public:
    int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields) {
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
    const static int MAIN_MEMORY_SIZE = 3 * BLOCK_SIZE; // initialize the main memory size according to the question
    //initialize your variables
    const static int maxPages = 3; // 3 pages in main memory at most 
    int spaceRemaining; // space remaining in main memory. If 0, then you've reached capacity. Initliazed later

    // You may declare variables based on your need 
    int numRecords; // number of records in the file

    class Page {
        static const int BLOCK_SIZE = 4096; // Define the size of each page (4KB)
        Page *nextPage; // Pointer to the next page in the list
        int pageNumber; // Identifier for the page
        vector<int> static offsetArray; // Tracks the current writing position within the page
        std::vector<char> data; // Storage for page data

        // Calculate the amount of 
        vector<int> static initializeOffsetArray() {
            int minBioLen = INT_MAX;
            int minNameLen = INT_MAX;

            ifstream file("Employee.csv");
            
            // Loop through records
            string line;
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
                    }

                    if (sizeof(fields[1])/sizeof(char) < minNameLen) {
                        minNameLen = sizeof(fields[1])/sizeof(char);
                    }
                    if (sizeof(fields[2])/sizeof(char) < minBioLen) {
                        minBioLen = sizeof(fields[2])/sizeof(char);
                    }
                }
            }

            // Minimize size of records: ID, name, bio, manager_id and then an 8 byte offset for each
            int minRecordSize = 3 * 8 + minNameLen + minBioLen;
            int maxRecords = (BLOCK_SIZE) / minRecordSize;
            return vector<int>(maxRecords, 0);
        }

        public:
            // Constructor for the Page class
            Page(int pageNum) : pageNumber(pageNum), nextPage(nullptr), offset(0), data(BLOCK_SIZE) {}

            // Method to add a record to this page
            bool addRecord(const std::string& record) {
                // Check if there is enough space left in the page
                if (record.size() + offset > BLOCK_SIZE) {
                    return false; // Not enough space, record not added
                }
                // Copy the record data into the page's data vector
                std::copy(record.begin(), record.end(), data.begin() + offset);
                // Update the offset to the new end of the data
                offset += record.size();
                return true; // Record added successfully
            }

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
            }

            // Setter method to link this page to the next page
            void setNextPage(Page* nextPage) {
                this->nextPage = nextPage;
            }

            // Method to dump the data of this page and all subsequent pages to a file
            void dumpPages(const std::string& filename) {
                Page* currentPage = this; // Start with the current page
                while(currentPage != nullptr) {
                    currentPage->dumpPage(filename); // Dump current page's data
                    currentPage = currentPage->nextPage; // Move to the next page
                }
            }


    // Contains linked list of pages and necessary functions
    class PageList {
        Page *head;
        static const int maxPages = 3; 

        // Function used to determine if row being examinied is among those already copied to EmployeeRelation
        // Use in following maxRecords function
        bool contains(const std::vector<int>& vec, int value) {
            return std::find(vec.begin(), vec.end(), value) != vec.end();
            }

        

        public:
        // Constructor for the PageList class
        PageList() : head(nullptr) {
            offsetArray = initializeOffsetArray();
        }

        // Method to initialize the pages and link them together
        Page* initializePages() {
            Page *tail = nullptr; // Pointer to the last page in the list
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

    };
        
        
        
        
    // calculate offsets of fields
            //offset = calcOffset();
        // append offset for beginning and end of record to offset vector
        // append record contents/fields. Ensure end of record matches with project end of record offset; same with beginning of record
          
            // for each record, insert record into the EmployeeRelation file according to structure:
                // store record contents as strings separated by commas and row terminated by newline
    void insertRecord(Page * page, Record record) {

        /*
        Calculate offsets/file size from argument record. 
        Store offset in array at beginning of page in use
        Then append record
        Offsets must point to beginning of each record, as well as end of last record
        If record is last record, add to page and write all page contents to file. Conclude.
        */

       int recordSize = sizeof(record);

        // Error check that record size is less than block size
        if (sizeof(record) >= BLOCK_SIZE) {
            cerr << "Record size exceeds block size.\n";
            exit(1);
        }
        
        // Calculate offset
        int offset = page->offsetVector[page->offsetVector.size() - 1] + recordSize;

        // Take neccessary steps if capacity is reached (you've utilized all the blocks in main memory)
    }
        

     
        /*
        Storage:
        For each new record:
            Check that there's sufficient space including offset
            If so, store offset and add record to page
            If page is full or next record is too large to fit, begin new page
            If page is third page and is full or next record is too large to fit, write to file. Empty pages.
        */


       
    };
};



    




public:
    StorageBufferManager(string NewFileName) {
        
        //initialize your variables
        int maxPages = 3; // 3 pages in main memory at most 



        // Create your EmployeeRelation.dat file 
        FILE * EmployeeRelation;
        EmployeeRelation = fopen(NewFileName.c_str(), "w");

        
    }

    // Read csv file (Employee.csv) and add records to the (EmployeeRelation)
    void createFromFile(string csvFName) {

        //initialize variables
        
        // Offset of record
        int offset = 0;
        // Space remaining on page. Set to full page intially
        int spaceRemaining = BLOCK_SIZE;
        // For reading line from csv
        string line;
         // For reading line into record object
        Record record;
        // Size of record
        int recordSize = 0;
        // Initialize pages and create pointer to first page
        Page * page = initializePages();

        // Open source file for reading
        FILE * file;
        file = fopen(csvFName.c_str(), "r");

        // Buffer for reading line from csv: equal to max possible record size plus 1
        char buffer[2* 8 + 200 + 500 + 1];

        // Loop through records
        while (fread(buffer, 1, sizeof(buffer), file))  {
        //  Get record
            record = createRecord(line);
            recordSize = record.recordSize();

            // Error check that record size is less than block size
            if recordSize > BLOCK_SIZE {
                stderr << "Record size exceeds block size.\n";
                exit(1);
            }

            // determine if there's enough room on current page:
                // if not and not last page, begin writing to next page
                // if not and last page, write page to file and empty page
            spaceRemaining = calcSpaceRemaining();

            // Current page full
            if (spaceRemaining < recordSize && page.pageNumber < maxPages - 1) {
                // Advance to next page
                page = page->nextPage;
            }
            // Main memory full
            else if (spaceRemaining < recordSize && page.pageNumber == maxPages - 1) {
                // Write page to file
                dumpPages();
                // Empty page
                page = initializePages();
            }
            else {
                // Add record to page
                insertRecord(page, record);
            }

        
        
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
};
