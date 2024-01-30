/*
Skeleton code for storage and buffer management
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;

int main(int argc, char* const argv[]) {

    // Create the EmployeeRelation file from Employee.csv
    StorageBufferManager manager("EmployeeRelation");
    manager.createFromFile("Employee.csv");
    

    
    // Loop to lookup IDs until user is ready to quit
    while (true) {
        cout << "Enter an ID to lookup, or 'q' to quit: ";
        string input;
        cin >> input;
        if (input == "q") {
            break;
        }
        int id = stoi(input);
        Record record = manager.findRecordById(id);
        if (&record == NULL) {
            cout << "No record found with ID " << id << "\n";
        } else {
            record.print();
        }
    }
    
    return 0;
}
