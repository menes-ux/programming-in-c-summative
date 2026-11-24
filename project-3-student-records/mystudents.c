#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

typedef struct {
    char code[32];
    char name[100];
    double marks;
    double maximum;
} Course;

typedef struct {
    char id[32];
    char first[50];
    char last[50];
    char programme[100];
    long year;
    size_t courseCount;
    Course *courses;
} Student;

Student *students = NULL;
size_t studentCount = 0;
int changed = 0;
const char *dataFile = "students.txt";

int read_line(FILE *file, char *text, size_t size) {
    if (fgets(text, (int)size, file) == NULL) {
        return ferror(file) ? -1 : 0;
    }

    size_t length = strlen(text);
    if (length > 0 && text[length - 1] == '\n') {
        text[--length] = '\0';
    } else {
        int next = fgetc(file);
        if (next != '\n' && next != EOF) {
            while ((next = fgetc(file)) != '\n' && next != EOF) {
            }
            return -1;
        }
        if (ferror(file)) {
            return -1;
        }
    }

    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[--length] = '\0';
    }
    size_t start = 0;
    while (text[start] && isspace((unsigned char)text[start])) {
        ++start;
    }
    memmove(text, text + start, length - start + 1);
    return 1;
}

int valid_text(const char *text) {
    if (*text == '\0') {
        return 0;
    }
    for (size_t i = 0; text[i]; ++i) {
        if (iscntrl((unsigned char)text[i])) {
            return 0;
        }
    }
    return 1;
}

int get_text(const char *prompt, char *text, size_t size) {
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        int result = read_line(stdin, text, size);
        if (result == 0 || (result < 0 && ferror(stdin))) {
            return 0;
        }
        if (result < 0) {
            printf("Keep the input to %zu characters or fewer.\n", size - 1);
        } else if (!valid_text(text)) {
            puts("Enter a value without control characters.");
        } else {
            return 1;
        }
    }
}

int parse_int(const char *text, int minimum, int maximum, int *number) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno || end == text || *end || value < minimum || value > maximum) {
        return 0;
    }
    *number = (int)value;
    return 1;
}

int get_int(const char *prompt, int minimum, int maximum, int *number) {
    char text[100];
    while (get_text(prompt, text, sizeof(text))) {
        if (parse_int(text, minimum, maximum, number)) {
            return 1;
        }
        printf("Enter a whole number from %d to %d.\n", minimum, maximum);
    }
    return 0;
}

int parse_year(const char *text, long *year) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno || end == text || *end || value < 1) {
        return 0;
    }
    *year = value;
    return 1;
}

int get_year(long *year) {
    char text[100];
    while (get_text("Year of study: ", text, sizeof(text))) {
        if (parse_year(text, year)) {
            return 1;
        }
        puts("Enter a positive whole number for the year.");
    }
    return 0;
}

int parse_number(const char *text, double *number) {
    for (size_t i = 0; text[i]; ++i) {
        char letter = text[i];
        if (!isdigit((unsigned char)letter) && letter != '.' && letter != '+' &&
            letter != '-' && letter != 'e' && letter != 'E') {
            return 0;
        }
    }
    char *end;
    errno = 0;
    double value = strtod(text, &end);
    if (errno || end == text || *end) {
        return 0;
    }
    *number = value;
    return 1;
}

int get_number(const char *prompt, double *number) {
    char text[100];
    while (get_text(prompt, text, sizeof(text))) {
        if (parse_number(text, number)) {
            return 1;
        }
        puts("Enter a number, such as 75 or 82.5.");
    }
    return 0;
}

int confirm(const char *prompt) {
    char answer[10];
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        int result = read_line(stdin, answer, sizeof(answer));
        if (result == 0 || (result < 0 && ferror(stdin))) {
            return 0;
        }
        if (result == 1) {
            if (strcmp(answer, "y") == 0 || strcmp(answer, "Y") == 0) {
                return 1;
            }
            if (*answer == '\0' || strcmp(answer, "n") == 0 || strcmp(answer, "N") == 0) {
                return 0;
            }
        }
        puts("Enter y or n, or press Enter for no.");
    }
}

int valid_id(const char *id) {
    if (*id == '\0') {
        return 0;
    }
    for (size_t i = 0; id[i]; ++i) {
        unsigned char letter = (unsigned char)id[i];
        if (!isalnum(letter) && letter != '-' && letter != '_') {
            return 0;
        }
    }
    return 1;
}

int get_id(const char *prompt, char *id, size_t size) {
    while (get_text(prompt, id, size)) {
        if (valid_id(id)) {
            return 1;
        }
        puts("Use letters, numbers, hyphens, or underscores.");
    }
    return 0;
}

int compare_text(const char *left, const char *right) {
    while (*left && *right) {
        int difference = tolower((unsigned char)*left) - tolower((unsigned char)*right);
        if (difference != 0) {
            return difference;
        }
        ++left;
        ++right;
    }
    return tolower((unsigned char)*left) - tolower((unsigned char)*right);
}

int contains(const char *text, const char *word) {
    for (size_t i = 0; text[i]; ++i) {
        size_t j = 0;
        while (word[j] && text[i + j] &&
               tolower((unsigned char)text[i + j]) == tolower((unsigned char)word[j])) {
            ++j;
        }
        if (word[j] == '\0') {
            return 1;
        }
    }
    return 0;
}

size_t find_student(Student *list, size_t count, const char *id) {
    for (size_t i = 0; i < count; ++i) {
        if (compare_text(list[i].id, id) == 0) {
            return i;
        }
    }
    return count;
}

size_t find_course(const Student *person, const char *code) {
    for (size_t i = 0; i < person->courseCount; ++i) {
        if (compare_text(person->courses[i].code, code) == 0) {
            return i;
        }
    }
    return person->courseCount;
}

Student *choose_student(void) {
    char id[32];
    if (!get_id("Student ID: ", id, sizeof(id))) {
        return NULL;
    }
    size_t position = find_student(students, studentCount, id);
    if (position == studentCount) {
        puts("No student has that ID.");
        return NULL;
    }
    return &students[position];
}

void free_students(Student *list, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        free(list[i].courses);
    }
    free(list);
}

double average(const Student *person) {
    long double marks = 0;
    long double maximum = 0;
    for (size_t i = 0; i < person->courseCount; ++i) {
        marks += person->courses[i].marks;
        maximum += person->courses[i].maximum;
    }
    return maximum > 0 ? (double)(marks / maximum * 100.0L) : 0;
}

void show_heading(void) {
    printf("\n%-16s %-30s %-24s %4s %7s %9s\n",
           "ID", "Name", "Programme", "Year", "Courses", "Average");
}

void show_row(const Student *person) {
    char name[102];
    snprintf(name, sizeof(name), "%s %s", person->first, person->last);
    printf("%-16s %-30s %-24s %4ld %7zu ",
           person->id, name, person->programme, person->year, person->courseCount);
    if (person->courseCount == 0) {
        puts("      N/A");
    } else {
        printf("%8.2f%%\n", average(person));
    }
}

void list_students(void) {
    if (studentCount == 0) {
        puts("No students registered yet.");
        return;
    }
    show_heading();
    for (size_t i = 0; i < studentCount; ++i) {
        show_row(&students[i]);
    }
}

void show_student(const Student *person) {
    printf("\nStudent ID: %s\nName: %s %s\nProgramme: %s\nYear: %ld\n",
           person->id, person->first, person->last, person->programme, person->year);
    printf("Courses: %zu\n", person->courseCount);
    if (person->courseCount == 0) {
        puts("No courses registered. Academic average: N/A");
        return;
    }
    for (size_t i = 0; i < person->courseCount; ++i) {
        const Course *course = &person->courses[i];
        double percent = course->marks / course->maximum * 100.0;
        printf("%s | %s | %.2f / %.2f | %.2f%% | %s\n",
               course->code, course->name, course->marks, course->maximum,
               percent, percent >= 50 ? "PASS" : "FAIL");
    }
    printf("Academic average: %.2f%% (%s)\n", average(person),
           average(person) >= 50 ? "PASS" : "FAIL");
}

int get_details(Student *person) {
    return get_text("First name: ", person->first, sizeof(person->first)) &&
           get_text("Last name: ", person->last, sizeof(person->last)) &&
           get_text("Programme: ", person->programme, sizeof(person->programme)) &&
           get_year(&person->year);
}

void register_student(void) {
    Student person = {0};
    if (!get_id("New student ID: ", person.id, sizeof(person.id))) {
        return;
    }
    if (find_student(students, studentCount, person.id) != studentCount) {
        puts("That student ID is already registered.");
        return;
    }
    if (!get_details(&person)) {
        return;
    }
    Student *bigger = realloc(students, (studentCount + 1) * sizeof(Student));
    if (bigger == NULL) {
        puts("Not enough memory to register this student.");
        return;
    }
    students = bigger;
    students[studentCount++] = person;
    changed = 1;
    puts("Student registered.");
}

void update_student(void) {
    Student *person = choose_student();
    if (person == NULL) {
        return;
    }
    Student updated = *person;
    if (get_details(&updated)) {
        *person = updated;
        changed = 1;
        puts("Student information updated.");
    }
}

void delete_student(void) {
    Student *person = choose_student();
    if (person == NULL || !confirm("Delete this student and their courses? [y/N]: ")) {
        return;
    }
    size_t position = (size_t)(person - students);
    free(person->courses);
    for (size_t i = position + 1; i < studentCount; ++i) {
        students[i - 1] = students[i];
    }
    --studentCount;
    if (studentCount == 0) {
        free(students);
        students = NULL;
    } else {
        Student *smaller = realloc(students, studentCount * sizeof(Student));
        if (smaller != NULL) {
            students = smaller;
        }
    }
    changed = 1;
    puts("Student deleted.");
}

int get_marks(double maximum, double *marks) {
    while (get_number("Marks obtained: ", marks)) {
        if (*marks >= 0 && *marks <= maximum) {
            return 1;
        }
        printf("Marks must be from 0 to %.2f.\n", maximum);
    }
    return 0;
}

void add_course(void) {
    Student *person = choose_student();
    Course course = {0};
    if (person == NULL || !get_id("Course code: ", course.code, sizeof(course.code))) {
        return;
    }
    if (find_course(person, course.code) != person->courseCount) {
        puts("This student is already registered for that course.");
        return;
    }
    if (!get_text("Course name: ", course.name, sizeof(course.name))) {
        return;
    }
    do {
        if (!get_number("Maximum marks: ", &course.maximum)) {
            return;
        }
        if (course.maximum <= 0) {
            puts("Maximum marks must be greater than zero.");
        }
    } while (course.maximum <= 0);
    if (!get_marks(course.maximum, &course.marks)) {
        return;
    }
    Course *bigger = realloc(person->courses, (person->courseCount + 1) * sizeof(Course));
    if (bigger == NULL) {
        puts("Not enough memory to add this course.");
        return;
    }
    person->courses = bigger;
    person->courses[person->courseCount++] = course;
    changed = 1;
    puts("Course added.");
}

void update_marks(void) {
    Student *person = choose_student();
    char code[32];
    if (person == NULL || !get_id("Course code: ", code, sizeof(code))) {
        return;
    }
    size_t position = find_course(person, code);
    if (position == person->courseCount) {
        puts("This student is not registered for that course.");
        return;
    }
    Course *course = &person->courses[position];
    printf("%s: maximum %.2f\n", course->name, course->maximum);
    double marks;
    if (get_marks(course->maximum, &marks)) {
        course->marks = marks;
        changed = 1;
        puts("Course marks updated.");
    }
}

void search_students(void) {
    int choice;
    char word[100];
    puts("1. Student ID\n2. First or last name\n3. Programme\n0. Back");
    if (!get_int("Search by: ", 0, 3, &choice) || choice == 0 ||
        !get_text("Search for: ", word, sizeof(word))) {
        return;
    }
    size_t found = 0;
    for (size_t i = 0; i < studentCount; ++i) {
        Student *person = &students[i];
        int match = choice == 1 ? compare_text(person->id, word) == 0 :
                    choice == 2 ? contains(person->first, word) || contains(person->last, word) :
                                  contains(person->programme, word);
        if (match) {
            if (found == 0) {
                show_heading();
            }
            show_row(person);
            ++found;
        }
    }
    printf("Matches: %zu\n", found);
}

int out_of_order(const Student *left, const Student *right, int field, int descending) {
    int comparison;
    if (field == 3 && (left->courseCount == 0 || right->courseCount == 0)) {
        return left->courseCount == 0 && right->courseCount != 0;
    }
    if (field == 1) {
        comparison = compare_text(left->id, right->id);
    } else if (field == 2) {
        comparison = compare_text(left->first, right->first);
        if (comparison == 0) {
            comparison = compare_text(left->last, right->last);
        }
    } else if (field == 3) {
        double first = average(left);
        double second = average(right);
        comparison = first > second ? 1 : first < second ? -1 : 0;
    } else {
        comparison = compare_text(left->programme, right->programme);
    }
    return descending ? comparison < 0 : comparison > 0;
}

void sort_students(void) {
    if (studentCount < 2) {
        puts("Register at least two students before sorting.");
        return;
    }
    int field, order;
    puts("1. Student ID\n2. Name (first, then last)\n3. Academic average\n4. Programme\n0. Back");
    if (!get_int("Sort by: ", 0, 4, &field) || field == 0 ||
        !get_int("Order (1 ascending, 2 descending): ", 1, 2, &order)) {
        return;
    }
    for (size_t end = studentCount; end > 1; --end) {
        int swapped = 0;
        for (size_t i = 0; i + 1 < end; ++i) {
            if (out_of_order(&students[i], &students[i + 1], field, order == 2)) {
                Student hold = students[i];
                students[i] = students[i + 1];
                students[i + 1] = hold;
                swapped = 1;
            }
        }
        if (!swapped) {
            break;
        }
    }
    changed = 1;
    puts("Students sorted. Students without courses appear last when sorting by average.");
    list_students();
}

int course_seen(size_t student, size_t course) {
    const char *code = students[student].courses[course].code;
    for (size_t i = 0; i <= student; ++i) {
        size_t end = i == student ? course : students[i].courseCount;
        for (size_t j = 0; j < end; ++j) {
            if (compare_text(students[i].courses[j].code, code) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

void write_report(FILE *file) {
    size_t courses = 0, graded = 0, passing = 0;
    size_t highest = studentCount, lowest = studentCount;
    long double total = 0;
    for (size_t i = 0; i < studentCount; ++i) {
        courses += students[i].courseCount;
        if (students[i].courseCount == 0) {
            continue;
        }
        double result = average(&students[i]);
        total += result;
        ++graded;
        if (result >= 50) {
            ++passing;
        }
        if (highest == studentCount || result > average(&students[highest])) {
            highest = i;
        }
        if (lowest == studentCount || result < average(&students[lowest])) {
            lowest = i;
        }
    }

    fprintf(file, "\nAcademic performance report\nStudents: %zu\nCourse registrations: %zu\n",
            studentCount, courses);
    if (graded > 0) {
        fprintf(file, "Overall average: %.2Lf%%\n", total / graded);
        fprintf(file, "Highest average: %s %s (%s) - %.2f%%\n",
                students[highest].first, students[highest].last, students[highest].id,
                average(&students[highest]));
        fprintf(file, "Lowest average: %s %s (%s) - %.2f%%\n",
                students[lowest].first, students[lowest].last, students[lowest].id,
                average(&students[lowest]));
    } else {
        fprintf(file, "Overall average: N/A\nHighest and lowest averages: N/A\n");
    }
    fprintf(file, "Passing students: %zu\nFailing students: %zu\nStudents without courses: %zu\n",
            passing, graded - passing, studentCount - graded);
    fprintf(file, "A student passes with an overall average of at least 50%%.\n");
    fprintf(file, "Student average = total marks / total maximum marks * 100.\n");
    fprintf(file, "Overall average is the mean of student averages, excluding students without courses.\n");

    fprintf(file, "\nStudents per programme\n");
    for (size_t i = 0; i < studentCount; ++i) {
        int seen = 0;
        for (size_t j = 0; j < i; ++j) {
            if (compare_text(students[i].programme, students[j].programme) == 0) {
                seen = 1;
                break;
            }
        }
        if (seen) {
            continue;
        }
        size_t count = 0;
        for (size_t j = 0; j < studentCount; ++j) {
            if (compare_text(students[i].programme, students[j].programme) == 0) {
                ++count;
            }
        }
        fprintf(file, "%s: %zu\n", students[i].programme, count);
    }

    fprintf(file, "\nAverage per course (mean of percentage marks)\n");
    for (size_t i = 0; i < studentCount; ++i) {
        for (size_t j = 0; j < students[i].courseCount; ++j) {
            if (course_seen(i, j)) {
                continue;
            }
            const Course *course = &students[i].courses[j];
            long double sum = 0;
            size_t count = 0;
            for (size_t k = 0; k < studentCount; ++k) {
                size_t match = find_course(&students[k], course->code);
                if (match != students[k].courseCount) {
                    const Course *other = &students[k].courses[match];
                    sum += other->marks / other->maximum * 100.0L;
                    ++count;
                }
            }
            fprintf(file, "%s | %s | %zu registrations | %.2Lf%%\n",
                    course->code, course->name, count, sum / count);
        }
    }
}

void academic_report(void) {
    write_report(stdout);
    if (!confirm("\nSave as academic_report.txt (overwrite if present)? [y/N]: ")) {
        return;
    }
    FILE *file = fopen("academic_report.txt", "w");
    if (file == NULL) {
        perror("Could not open academic_report.txt");
        return;
    }
    write_report(file);
    int failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    puts(failed ? "Could not finish writing the report." : "Report saved to academic_report.txt.");
}

int save_records(void) {
    FILE *file = fopen("students.txt.tmp", "w");
    if (file == NULL) {
        perror("Could not open the save file");
        return 0;
    }
    fprintf(file, "STUDENT_RECORDS_1\n%zu\n", studentCount);
    for (size_t i = 0; i < studentCount; ++i) {
        const Student *person = &students[i];
        fprintf(file, "%s\n%s\n%s\n%s\n%ld\n%zu\n", person->id, person->first,
                person->last, person->programme, person->year, person->courseCount);
        for (size_t j = 0; j < person->courseCount; ++j) {
            const Course *course = &person->courses[j];
            fprintf(file, "%s\n%s\n%.17g\n%.17g\n",
                    course->code, course->name, course->marks, course->maximum);
        }
    }
    int failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        puts("Could not finish saving the records. The previous save is unchanged.");
        remove("students.txt.tmp");
        return 0;
    }
    if (rename("students.txt.tmp", dataFile) != 0) {
        perror("Could not replace students.txt");
        remove("students.txt.tmp");
        return 0;
    }
    changed = 0;
    printf("Saved %zu student records to %s.\n", studentCount, dataFile);
    return 1;
}

int read_field(FILE *file, char *text, size_t size) {
    return read_line(file, text, size) == 1 && valid_text(text);
}

int read_count(FILE *file, size_t *count) {
    char text[100], *end;
    if (!read_field(file, text, sizeof(text))) {
        return 0;
    }
    for (size_t i = 0; text[i]; ++i) {
        if (!isdigit((unsigned char)text[i])) {
            return 0;
        }
    }
    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno || *end || value != (size_t)value) {
        return 0;
    }
    *count = (size_t)value;
    return 1;
}

int load_records(void) {
    FILE *file = fopen(dataFile, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            puts("No students.txt found. Current records are unchanged.");
        } else {
            perror("Could not open students.txt");
        }
        return 0;
    }

    char text[100];
    size_t expected = 0, loadedCount = 0;
    int memoryError = 0;
    int valid = read_field(file, text, sizeof(text)) &&
                strcmp(text, "STUDENT_RECORDS_1") == 0 &&
                read_count(file, &expected);
    Student *loaded = NULL;
    if (valid && expected > 0) {
        loaded = calloc(expected, sizeof(Student));
        if (loaded == NULL) {
            valid = 0;
            memoryError = 1;
        }
    }

    for (size_t i = 0; valid && i < expected; ++i) {
        Student *person = &loaded[i];
        loadedCount = i + 1;
        size_t courses = 0;
        valid = read_field(file, person->id, sizeof(person->id)) && valid_id(person->id) &&
                find_student(loaded, i, person->id) == i &&
                read_field(file, person->first, sizeof(person->first)) &&
                read_field(file, person->last, sizeof(person->last)) &&
                read_field(file, person->programme, sizeof(person->programme)) &&
                read_field(file, text, sizeof(text)) && parse_year(text, &person->year) &&
                read_count(file, &courses);
        if (!valid) {
            break;
        }
        if (courses > 0) {
            person->courses = calloc(courses, sizeof(Course));
            if (person->courses == NULL) {
                valid = 0;
                memoryError = 1;
                break;
            }
        }
        for (size_t j = 0; valid && j < courses; ++j) {
            Course *course = &person->courses[j];
            valid = read_field(file, course->code, sizeof(course->code)) && valid_id(course->code) &&
                    find_course(person, course->code) == person->courseCount &&
                    read_field(file, course->name, sizeof(course->name)) &&
                    read_field(file, text, sizeof(text)) && parse_number(text, &course->marks) &&
                    read_field(file, text, sizeof(text)) && parse_number(text, &course->maximum) &&
                    course->maximum > 0 && course->marks >= 0 && course->marks <= course->maximum;
            if (valid) {
                ++person->courseCount;
            }
        }
    }

    if (valid) {
        int result;
        while ((result = read_line(file, text, sizeof(text))) == 1) {
            if (*text != '\0') {
                valid = 0;
                break;
            }
        }
        if (result < 0) {
            valid = 0;
        }
    }
    if (fclose(file) != 0) {
        valid = 0;
    }
    if (!valid) {
        free_students(loaded, loadedCount);
        puts(memoryError ? "Not enough memory to load records. Current records are unchanged." :
                           "Invalid or incomplete save file. Current records are unchanged.");
        return 0;
    }

    free_students(students, studentCount);
    students = loaded;
    studentCount = expected;
    changed = 0;
    printf("Loaded %zu student records from %s.\n", studentCount, dataFile);
    return 1;
}

int main(void) {
    load_records();
    int running = 1;
    int result = 0;
    while (running) {
        puts("\nMy Student Records");
        puts("1. Register student\n2. List students\n3. View student and performance");
        puts("4. Update student\n5. Delete student\n6. Add course\n7. Update course marks");
        puts("8. Search students\n9. Sort students\n10. Academic report");
        puts("11. Save records\n12. Load records\n0. Save and exit");
        int choice;
        if (!get_int("Choose an option: ", 0, 12, &choice)) {
            puts("\nInput ended.");
            if (changed && !save_records()) {
                result = 1;
            }
            break;
        }
        switch (choice) {
            case 1: register_student(); break;
            case 2: list_students(); break;
            case 3: {
                Student *person = choose_student();
                if (person != NULL) {
                    show_student(person);
                }
                break;
            }
            case 4: update_student(); break;
            case 5: delete_student(); break;
            case 6: add_course(); break;
            case 7: update_marks(); break;
            case 8: search_students(); break;
            case 9: sort_students(); break;
            case 10: academic_report(); break;
            case 11: save_records(); break;
            case 12:
                if (!changed || confirm("Discard unsaved changes and reload? [y/N]: ")) {
                    load_records();
                }
                break;
            case 0:
                if (!changed || save_records() || confirm("Exit without saving? [y/N]: ")) {
                    running = 0;
                }
                break;
        }
    }
    free_students(students, studentCount);
    return result;
}
