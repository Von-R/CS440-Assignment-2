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

    // 
    StorageBufferManager manager("EmployeeRelation");


    manager.createFromFile("Employee.csv");
    
    // Loop to lookup IDs until user is ready to quit
    while (true) {
        string input;
        string * r;
        cout << "Enter an ID to lookup: ";
        cin >> input;
        if (input == "q") {
            break;
        }
        int id = stoi(input);
        
        // needs work
        // r = manager.findRecordByID(id);

    }
    //exitProgram();
    

    return 0;
}
