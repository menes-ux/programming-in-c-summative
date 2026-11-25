#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>

typedef struct {
    const char *file;
    const char *keyword;
    char **inputs;
    size_t inputCount;
    size_t number;
    pthread_t thread;
    int started;
    size_t lines;
    size_t occurrences;
    size_t matchingLines;
    char report[100];
    char error[200];
} Job;

void set_error(Job *job, const char *message) {
    if (job->error[0] == '\0') {
        snprintf(job->error, sizeof(job->error), "%s", message);
    }
}

void file_error(Job *job, const char *message) {
    if (job->error[0] == '\0') {
        snprintf(job->error, sizeof(job->error), "%s: %s", message, strerror(errno));
    }
}

int read_line(FILE *file, char **text, size_t *space, size_t *length) {
    *length = 0;
    if (*text == NULL) {
        *space = 256;
        *text = malloc(*space);
        if (*text == NULL) {
            return -2;
        }
    }

    int letter;
    while ((letter = fgetc(file)) != EOF) {
        if (letter == '\n') {
            (*text)[*length] = '\0';
            return 1;
        }
        if (*length + 1 >= *space) {
            size_t bigger = *space * 2;
            if (bigger <= *space) {
                return -2;
            }
            char *buffer = realloc(*text, bigger);
            if (buffer == NULL) {
                return -2;
            }
            *text = buffer;
            *space = bigger;
        }
        (*text)[(*length)++] = (char)letter;
    }
    if (ferror(file)) {
        return -1;
    }
    if (*length > 0) {
        (*text)[*length] = '\0';
        return 1;
    }
    return 0;
}

size_t count_keyword(const char *line, size_t length, const char *keyword) {
    size_t wordLength = strlen(keyword);
    size_t count = 0;
    if (wordLength == 0 || wordLength > length) {
        return 0;
    }
    for (size_t i = 0; i <= length - wordLength;) {
        size_t j = 0;
        while (j < wordLength &&
               tolower((unsigned char)line[i + j]) == tolower((unsigned char)keyword[j])) {
            ++j;
        }
        if (j == wordLength) {
            ++count;
            i += wordLength;
        } else {
            ++i;
        }
    }
    return count;
}

int report_is_input(const Job *job, const char *name) {
    for (size_t i = 0; i < job->inputCount; ++i) {
        const char *base = strrchr(job->inputs[i], '/');
        base = base == NULL ? job->inputs[i] : base + 1;
        if (strcmp(name, base) == 0) {
            return 1;
        }
    }
    return 0;
}

FILE *create_report(Job *job) {
    for (size_t attempt = 1; ; ++attempt) {
        if (attempt == 1) {
            snprintf(job->report, sizeof(job->report), "analysis_%zu.txt", job->number);
        } else {
            snprintf(job->report, sizeof(job->report), "analysis_%zu_%zu.txt", job->number, attempt);
        }
        if (report_is_input(job, job->report)) {
            continue;
        }
        FILE *report = fopen(job->report, "wx");
        if (report != NULL) {
            return report;
        }
        if (errno != EEXIST) {
            file_error(job, "Cannot create report");
            job->report[0] = '\0';
            return NULL;
        }
    }
}

void write_summary(FILE *report, const Job *job) {
    if (job->matchingLines == 0) {
        fprintf(report, "None\n");
    }
    fprintf(report, "\nTotal lines: %zu\nKeyword occurrences: %zu\nMatching lines: %zu\n",
            job->lines, job->occurrences, job->matchingLines);
    fprintf(report, "Status: %s\n", job->error[0] ? "ERROR" : "SUCCESS");
    if (job->error[0]) {
        fprintf(report, "Error: %s\n", job->error);
    }
}

void *process_file(void *argument) {
    Job *job = argument;
    FILE *input = fopen(job->file, "r");
    if (input == NULL) {
        file_error(job, "Cannot open input file");
    }

    FILE *report = create_report(job);
    if (report == NULL) {
        if (input != NULL) {
            fclose(input);
        }
        return NULL;
    }
    fprintf(report, "Document analysis report\nInput file: %s\nSearch keyword: %s\n\nMatching line numbers:\n",
            job->file, job->keyword);

    char *line = NULL;
    size_t space = 0, length = 0;
    if (input != NULL) {
        int result;
        while ((result = read_line(input, &line, &space, &length)) == 1) {
            ++job->lines;
            size_t found = count_keyword(line, length, job->keyword);
            job->occurrences += found;
            if (found > 0) {
                ++job->matchingLines;
                if (fprintf(report, "%zu\n", job->lines) < 0) {
                    file_error(job, "Cannot write report");
                    break;
                }
            }
        }
        if (result == -2) {
            set_error(job, "Not enough memory to read a line");
        } else if (result == -1) {
            file_error(job, "Cannot read input file");
        }
        if (fclose(input) != 0) {
            file_error(job, "Cannot close input file");
        }
    }
    free(line);

    write_summary(report, job);
    int failed = ferror(report);
    if (fclose(report) != 0) {
        failed = 1;
    }
    if (failed) {
        set_error(job, "Could not finish writing the report");
    }
    return NULL;
}

void show_result(const Job *job) {
    if (job->error[0]) {
        printf("[Thread %zu] ERROR: %s | %s\n", job->number, job->file, job->error);
    } else {
        printf("[Thread %zu] Completed: %s | %zu lines, %zu occurrences, %zu matching lines\n",
               job->number, job->file, job->lines, job->occurrences, job->matchingLines);
        if (job->occurrences == 0) {
            puts("  Keyword not found.");
        }
    }
    if (job->report[0]) {
        printf("  Report: %s\n", job->report);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argv[1][0] == '\0') {
        fprintf(stderr, "Usage: %s keyword file1.txt [file2.txt ...]\n", argv[0]);
        fprintf(stderr, "Provide a non-empty keyword and at least one input file.\n");
        return 1;
    }

    size_t count = (size_t)(argc - 2);
    Job *jobs = calloc(count, sizeof(Job));
    if (jobs == NULL) {
        fprintf(stderr, "Not enough memory to prepare the threads.\n");
        return 1;
    }
    printf("My Document Analyzer\nKeyword: %s\nInput files: %zu\n\n", argv[1], count);

    int failed = 0, joinError = 0;
    for (size_t i = 0; i < count; ++i) {
        jobs[i].file = argv[i + 2];
        jobs[i].keyword = argv[1];
        jobs[i].inputs = argv + 2;
        jobs[i].inputCount = count;
        jobs[i].number = i + 1;
        printf("[Thread %zu] Starting: %s\n", i + 1, jobs[i].file);
        int result = pthread_create(&jobs[i].thread, NULL, process_file, &jobs[i]);
        if (result != 0) {
            snprintf(jobs[i].error, sizeof(jobs[i].error), "Cannot create thread: %s", strerror(result));
            printf("[Thread %zu] ERROR: %s\n", i + 1, jobs[i].error);
            failed = 1;
        } else {
            jobs[i].started = 1;
        }
    }

    puts("\nProcessing results");
    for (size_t i = 0; i < count; ++i) {
        if (!jobs[i].started) {
            continue;
        }
        int result = pthread_join(jobs[i].thread, NULL);
        if (result != 0) {
            printf("[Thread %zu] Cannot join thread: %s\n", i + 1, strerror(result));
            failed = 1;
            joinError = 1;
            continue;
        }
        show_result(&jobs[i]);
        if (jobs[i].error[0]) {
            failed = 1;
        }
    }
    if (!joinError) {
        free(jobs);
    }
    puts(failed ? "Finished with errors. See the results above." : "All files processed successfully.");
    return failed ? 1 : 0;
}
