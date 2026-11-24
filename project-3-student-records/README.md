# My Student Records

A menu-driven C program for managing students and their courses, searching and sorting records, and reporting academic performance.

## Run

From the repository root in an Ubuntu or WSL terminal:

```bash
cd project-3-student-records
gcc -std=c11 -Wall -Wextra -Wpedantic mystudents.c -o a.out
./a.out
```

Saved records load at startup. Option 11 saves changes, option 12 reloads records, and option 0 saves changes and exits. Option 10 displays an academic report and offers to save it as `academic_report.txt`.

## Code

`Student` stores an ID, first and last names, programme, year, and a pointer to its course array. Each `Course` stores a code, name, marks, and maximum marks.

Student and course arrays grow using `realloc()`. Loading uses `calloc()` to reconstruct the records. Allocation results are checked. Deleting a student frees their courses and attempts to shrink the student array; exiting frees all records.

Searching uses a manual linear search. IDs match exactly, ignoring case; names and programmes support partial matches. Sorting uses bubble sort by ID, first then last name, academic average, or programme, in either direction. Students without courses appear last when sorting by average. No `qsort()` is used.

Student average is total marks divided by total maximum marks, multiplied by 100. The report's overall average is the mean of student averages, excluding students with no courses. Course averages use percentage marks across students. A course pass is at least 50%; the report classifies students by an overall average of at least 50%. Students without courses show N/A and are counted separately.

Records are stored in a text file, including each student's courses. Saving writes a temporary file and replaces `students.txt` only after writing succeeds. Loading validates new records before replacing those already in memory.

Validation rejects duplicate student IDs and course codes within a student's record, invalid menu input, non-positive years or maximum marks, and marks outside the allowed range. Missing records, file errors, and allocation failures produce error messages. Empty records are handled without division by zero.

## Sample data and report

- [Student records](students.txt): Einstein Nmamah (J2025, BEL) with 75/100 and Eelaf Adam (J2026, BSE) with 40/100 in C101, Programming in C.
- [Academic report](academic_report.txt): two students, two course registrations, a 57.50% overall average, one passing student, and one failing student.

## Testing

Forty-nine checks passed in Ubuntu, including CRUD operations, course management, searching, all sorting options, academic calculations, save/load precision, invalid input, corrupt files, and allocation failures. The checks included compiler warnings, AddressSanitizer, and UndefinedBehaviorSanitizer.

Manual testing confirmed registration and course entry, reporting, and the operations below. The temporary JTEST record used for updating and deletion is absent from the final sample data.

| Screenshot | Result |
| --- | --- |
| [Search](screenshots/search.png) | Searching for Einstein returned J2025 with a 75.00% average |
| [Sort](screenshots/sort.png) | Descending average placed Einstein at 75.00% before Eelaf at 40.00% |
| [Update](screenshots/update.png) | JTEST was updated to programme BEL and year 2, then viewed |
| [Delete](screenshots/delete.png) | Deletion was confirmed; the remaining list contained only J2025 and J2026 |
| [Save and load](screenshots/save-load.png) | Restart loaded two records and restored Einstein's course and 75.00% average |
