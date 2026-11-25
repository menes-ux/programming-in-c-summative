# My Document Analyzer

A C program that searches text files concurrently using one POSIX thread per file. Each thread counts lines and keyword occurrences and writes its own analysis report.

## Run

From the repository root in an Ubuntu or WSL terminal:

```bash
cd project-5-document-analyzer
gcc -std=c11 -Wall -Wextra -Wpedantic mydocuments.c -pthread -o a.out
./a.out "IoT" document1.txt document2.txt document3.txt
```

Pass the keyword first, followed by one or more filenames. Use quotes around keywords or paths containing spaces. An empty keyword is rejected.

Reports are written in the current directory. Existing reports and input files are preserved; a numbered suffix is added when a report name is already taken. The terminal prints the actual report filenames.

## Code

Each `Job` structure holds a filename, keyword, thread handle, counters, report filename, and error message. `pthread_create()` receives a pointer to that job and runs `process_file()`. The main thread calls `pthread_join()` before reading each worker's results.

Workers have separate input streams, counters, buffers, and reports, so they need no mutexes or shared counter updates. The main thread displays startup messages, completion results, and errors.

`count_keyword()` compares characters without case sensitivity and counts non-overlapping substring matches within each line. Reported line numbers start at 1. A final line without a newline is counted, and a missing keyword is a successful result with zero matches.

The job array uses `calloc()`. Line buffers use `malloc()` and grow with `realloc()` so long lines are not split into extra lines. Buffers and file handles are released by the workers; job records are released after successful joins.

Missing or unreadable inputs and report creation, writing, memory, or thread errors are reported. A failed file does not stop other workers. If thread creation fails, the main thread still joins the threads that started. Successful runs return 0; runs with errors return 1.

## Sample reports

All three sample documents contain three lines.

| Input | Report | Occurrences of IoT | Matching line numbers |
| --- | --- | --- | --- |
| [document1.txt](document1.txt) | [analysis_1.txt](analysis_1.txt) | 3 | 1, 2 |
| [document2.txt](document2.txt) | [analysis_2.txt](analysis_2.txt) | 2 | 1, 3 |
| [document3.txt](document3.txt) | [analysis_3.txt](analysis_3.txt) | 0 | None |

The first two reports each have two matching lines. The third has none. All three show SUCCESS.

## Testing

Thirty-six checks passed in Ubuntu, including strict compiler warnings, AddressSanitizer, and UndefinedBehaviorSanitizer. Cases covered case-insensitive searching, line and occurrence counts, empty and long files, real concurrent processing, 32 input files, report collisions, file errors, memory failures, and joining remaining threads after creation failures.

The [terminal output](output) records the successful three-file run and this missing-file test:

```bash
./a.out "IoT" missing.txt
```

The [missing-file report](analysis_1_2.txt) shows ERROR with zero processed lines. This error is expected because the input does not exist.
