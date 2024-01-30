overview

Open employee.csv
Read record/row

offset array for page: 
STRETCH GOAL: determined dynamically each time page is written to
determine max number of records ('n') by finding smallest record size
reserve enough space for n + 1 offset: one for each record plus one to mark end of last offset. 

For each record: 
ensure record is not larger than page size
calculate size of record
Use size to calculate offset
Increment dataSize counter to keep track of total size of records processed
Check spaceRemaining counter to determine if there's enough space on page for new record

If yes:
Store offset in offset array for given page
Append record to position of last offset.
Store end of new record as newest offset. 

If not on current page, but there is space on another page:
Advance to next page
Repeat above process

If not on current page and there is no space on another page:
Write to disk
Reset to page 0 
Perform 'if yes' steps for current record

Writing to file:
open/create EmployeeRelation.dat file
From page 0 to page 2, write each record to file in sequence
??? Use offset to determine write location or simply append?
Check that file contents match what was copied to main memory and has been copied over so far; if not, throw error and abort
After all files have been written, similarly check and verify integrity; if not, throw error and abort
If so, close file and automatically test file contents
