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
    
    // Create a StorageBufferManager object
    StorageBufferManager manager("EmployeeRelation");

    // Create a schema object
    manager.createFromFile("Employee.csv");
    
    // Loop to lookup IDs until user is ready to quit
    while (true) {
        string input;
        string * r;
        cout << "Enter an ID to lookup: ";
        cin >> input;
        if (input == "q") {
            //manager.exitProgram();
            cout << "Goodbye!" << endl;
            break;
        }
        int id = stoi(input);
        
        // Search for the ID
        manager.searchID(id);
    }
    
    return 0;
}
