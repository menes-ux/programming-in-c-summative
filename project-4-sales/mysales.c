#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

typedef struct {
    char id[32];
    char product[100];
    long quantity;
    double price;
    long double amount;
    char date[11];
} Sale;

typedef struct {
    double minimum;
    char product[100];
} Filter;

typedef long double (*Operation)(const Sale *, size_t);
typedef int (*Match)(const Sale *, const Filter *);
typedef int (*Compare)(const Sale *, const Sale *);

Sale *sales = NULL;
size_t saleCount = 0;
int changed = 0;
const char *dataFile = "sales.txt";

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

int parse_quantity(const char *text, long *quantity) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno || end == text || *end || value < 1) {
        return 0;
    }
    *quantity = value;
    return 1;
}

int get_quantity(long *quantity) {
    char text[100];
    while (get_text("Quantity sold: ", text, sizeof(text))) {
        if (parse_quantity(text, quantity)) {
            return 1;
        }
        puts("Quantity must be a positive whole number.");
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

int valid_date(const char *date) {
    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') {
        return 0;
    }
    for (size_t i = 0; i < 10; ++i) {
        if (i != 4 && i != 7 && !isdigit((unsigned char)date[i])) {
            return 0;
        }
    }
    int year, month, day;
    if (sscanf(date, "%4d-%2d-%2d", &year, &month, &day) != 3 ||
        year < 1 || month < 1 || month > 12) {
        return 0;
    }
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) {
        days[1] = 29;
    }
    return day >= 1 && day <= days[month - 1];
}

int get_date(char *date, size_t size) {
    while (get_text("Date (YYYY-MM-DD): ", date, size)) {
        if (valid_date(date)) {
            return 1;
        }
        puts("Enter a real date in YYYY-MM-DD format.");
    }
    return 0;
}

size_t find_sale(Sale *list, size_t count, const char *id) {
    for (size_t i = 0; i < count; ++i) {
        if (compare_text(list[i].id, id) == 0) {
            return i;
        }
    }
    return count;
}

Sale *choose_sale(void) {
    char id[32];
    if (!get_id("Transaction ID: ", id, sizeof(id))) {
        return NULL;
    }
    size_t position = find_sale(sales, saleCount, id);
    if (position == saleCount) {
        puts("No transaction has that ID.");
        return NULL;
    }
    return &sales[position];
}

void show_heading(void) {
    printf("\n%-14s %-24s %8s %12s %14s %-10s\n",
           "ID", "Product", "Quantity", "Unit price", "Total", "Date");
}

void show_sale(const Sale *sale) {
    printf("%-14s %-24s %8ld %12.2f %14.2Lf %s\n",
           sale->id, sale->product, sale->quantity, sale->price, sale->amount, sale->date);
}

void list_sales(void) {
    if (saleCount == 0) {
        puts("No transactions recorded yet.");
        return;
    }
    show_heading();
    for (size_t i = 0; i < saleCount; ++i) {
        show_sale(&sales[i]);
    }
}

int get_details(Sale *sale) {
    if (!get_text("Product name: ", sale->product, sizeof(sale->product)) ||
        !get_quantity(&sale->quantity)) {
        return 0;
    }
    do {
        if (!get_number("Unit price: ", &sale->price)) {
            return 0;
        }
        if (sale->price <= 0) {
            puts("Unit price must be greater than zero.");
        }
    } while (sale->price <= 0);
    if (!get_date(sale->date, sizeof(sale->date))) {
        return 0;
    }
    sale->amount = (long double)sale->quantity * sale->price;
    return 1;
}

void add_sale(void) {
    Sale sale = {0};
    if (!get_id("New transaction ID: ", sale.id, sizeof(sale.id))) {
        return;
    }
    if (find_sale(sales, saleCount, sale.id) != saleCount) {
        puts("That transaction ID is already recorded.");
        return;
    }
    if (!get_details(&sale)) {
        return;
    }
    Sale *bigger = realloc(sales, (saleCount + 1) * sizeof(Sale));
    if (bigger == NULL) {
        puts("Not enough memory to add this transaction.");
        return;
    }
    sales = bigger;
    sales[saleCount++] = sale;
    changed = 1;
    puts("Transaction recorded.");
    show_heading();
    show_sale(&sales[saleCount - 1]);
}

void update_sale(void) {
    Sale *sale = choose_sale();
    if (sale == NULL) {
        return;
    }
    Sale updated = *sale;
    if (get_details(&updated)) {
        *sale = updated;
        changed = 1;
        puts("Transaction updated.");
        show_heading();
        show_sale(sale);
    }
}

void delete_sale(void) {
    Sale *sale = choose_sale();
    if (sale == NULL || !confirm("Delete this transaction? [y/N]: ")) {
        return;
    }
    size_t position = (size_t)(sale - sales);
    for (size_t i = position + 1; i < saleCount; ++i) {
        sales[i - 1] = sales[i];
    }
    --saleCount;
    if (saleCount == 0) {
        free(sales);
        sales = NULL;
    } else {
        Sale *smaller = realloc(sales, saleCount * sizeof(Sale));
        if (smaller != NULL) {
            sales = smaller;
        }
    }
    changed = 1;
    puts("Transaction deleted.");
}

long double total_revenue(const Sale *list, size_t count) {
    long double total = 0;
    for (size_t i = 0; i < count; ++i) {
        total += list[i].amount;
    }
    return total;
}

long double average_value(const Sale *list, size_t count) {
    return count > 0 ? total_revenue(list, count) / count : 0;
}

long double highest_value(const Sale *list, size_t count) {
    long double highest = 0;
    for (size_t i = 0; i < count; ++i) {
        if (list[i].amount > highest) {
            highest = list[i].amount;
        }
    }
    return highest;
}

long double lowest_value(const Sale *list, size_t count) {
    if (count == 0) {
        return 0;
    }
    long double lowest = list[0].amount;
    for (size_t i = 1; i < count; ++i) {
        if (list[i].amount < lowest) {
            lowest = list[i].amount;
        }
    }
    return lowest;
}

long double total_units(const Sale *list, size_t count) {
    long double units = 0;
    for (size_t i = 0; i < count; ++i) {
        units += list[i].quantity;
    }
    return units;
}

void process_sales(void) {
    if (saleCount == 0) {
        puts("No transactions recorded yet.");
        return;
    }
    Operation operations[] = {total_revenue, average_value, highest_value, lowest_value, total_units};
    const char *names[] = {"Total revenue", "Average transaction value", "Highest transaction value",
                           "Lowest transaction value", "Total units sold"};
    puts("1. Total revenue\n2. Average transaction value\n3. Highest transaction\n4. Lowest transaction\n5. Total units sold\n0. Back");
    int choice;
    if (!get_int("Process sales: ", 0, 5, &choice) || choice == 0) {
        return;
    }
    Operation operation = operations[choice - 1];
    long double result = operation(sales, saleCount);
    printf(choice == 5 ? "%s: %.0Lf\n" : "%s: %.2Lf\n", names[choice - 1], result);
    if (choice == 3 || choice == 4) {
        for (size_t i = 0; i < saleCount; ++i) {
            if (sales[i].amount == result) {
                show_heading();
                show_sale(&sales[i]);
                break;
            }
        }
    }
}

int above_amount(const Sale *sale, const Filter *filter) {
    return sale->amount > filter->minimum;
}

int with_product(const Sale *sale, const Filter *filter) {
    return contains(sale->product, filter->product);
}

size_t matching_sales(Match matches, const Filter *filter) {
    size_t found = 0;
    for (size_t i = 0; i < saleCount; ++i) {
        if (matches(&sales[i], filter)) {
            if (found == 0) {
                show_heading();
            }
            show_sale(&sales[i]);
            ++found;
        }
    }
    return found;
}

void filter_sales(void) {
    Filter filter = {0};
    Match matches;
    int choice;
    puts("1. Transactions above an amount\n2. Transactions involving a product\n0. Back");
    if (!get_int("Filter by: ", 0, 2, &choice) || choice == 0) {
        return;
    }
    if (choice == 1) {
        do {
            if (!get_number("Amount: ", &filter.minimum)) {
                return;
            }
            if (filter.minimum < 0) {
                puts("Enter an amount of zero or more.");
            }
        } while (filter.minimum < 0);
        matches = above_amount;
    } else {
        if (!get_text("Product name or part of it: ", filter.product, sizeof(filter.product))) {
            return;
        }
        matches = with_product;
    }
    size_t found = matching_sales(matches, &filter);
    printf("Matching transactions: %zu\n", found);
}

int same_amount(long double first, long double second) {
    long double difference = first - second;
    if (difference < 0) {
        difference = -difference;
    }
    return difference < 0.005L;
}

void search_sales(void) {
    int choice;
    char word[100] = "";
    double amount = 0;
    puts("1. Transaction ID\n2. Product name\n3. Transaction amount\n0. Back");
    if (!get_int("Search by: ", 0, 3, &choice) || choice == 0) {
        return;
    }
    if (choice == 3) {
        do {
            if (!get_number("Transaction amount: ", &amount)) {
                return;
            }
            if (amount < 0) {
                puts("Enter an amount of zero or more.");
            }
        } while (amount < 0);
    } else if (!get_text("Search for: ", word, sizeof(word))) {
        return;
    }
    size_t found = 0;
    for (size_t i = 0; i < saleCount; ++i) {
        int match = choice == 1 ? compare_text(sales[i].id, word) == 0 :
                    choice == 2 ? contains(sales[i].product, word) :
                                  same_amount(sales[i].amount, amount);
        if (match) {
            if (found == 0) {
                show_heading();
            }
            show_sale(&sales[i]);
            ++found;
        }
    }
    printf("Matches: %zu\n", found);
}

int compare_id(const Sale *left, const Sale *right) {
    return compare_text(left->id, right->id);
}

int compare_product(const Sale *left, const Sale *right) {
    return compare_text(left->product, right->product);
}

int compare_amount(const Sale *left, const Sale *right) {
    return left->amount > right->amount ? 1 : left->amount < right->amount ? -1 : 0;
}

void bubble_sort(Compare compare, int descending) {
    for (size_t end = saleCount; end > 1; --end) {
        int swapped = 0;
        for (size_t i = 0; i + 1 < end; ++i) {
            int comparison = compare(&sales[i], &sales[i + 1]);
            if ((!descending && comparison > 0) || (descending && comparison < 0)) {
                Sale hold = sales[i];
                sales[i] = sales[i + 1];
                sales[i + 1] = hold;
                swapped = 1;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

void sort_sales(void) {
    if (saleCount < 2) {
        puts("Record at least two transactions before sorting.");
        return;
    }
    Compare comparisons[] = {compare_id, compare_product, compare_amount};
    int choice, order;
    puts("1. Transaction ID\n2. Product name\n3. Total amount\n0. Back");
    if (!get_int("Sort by: ", 0, 3, &choice) || choice == 0 ||
        !get_int("Order (1 ascending, 2 descending): ", 1, 2, &order)) {
        return;
    }
    bubble_sort(comparisons[choice - 1], order == 2);
    changed = 1;
    puts("Transactions sorted.");
    list_sales();
}

void write_report(FILE *file, const char *product) {
    fprintf(file, "\nSales report\nTransactions: %zu\n", saleCount);
    fprintf(file, "Total revenue: %.2Lf\nAverage transaction value: %.2Lf\n",
            total_revenue(sales, saleCount), average_value(sales, saleCount));
    if (saleCount > 0) {
        long double highest = highest_value(sales, saleCount);
        long double lowest = lowest_value(sales, saleCount);
        for (size_t i = 0; i < saleCount; ++i) {
            if (sales[i].amount == highest) {
                fprintf(file, "Highest transaction: %s | %s | %.2Lf\n",
                        sales[i].id, sales[i].product, highest);
                break;
            }
        }
        for (size_t i = 0; i < saleCount; ++i) {
            if (sales[i].amount == lowest) {
                fprintf(file, "Lowest transaction: %s | %s | %.2Lf\n",
                        sales[i].id, sales[i].product, lowest);
                break;
            }
        }
    } else {
        fprintf(file, "Highest and lowest transactions: N/A\n");
    }
    fprintf(file, "Total units sold: %.0Lf\n", total_units(sales, saleCount));
    long double productRevenue = 0;
    size_t found = 0;
    for (size_t i = 0; i < saleCount; ++i) {
        if (compare_text(sales[i].product, product) == 0) {
            productRevenue += sales[i].amount;
            ++found;
        }
    }
    fprintf(file, "Revenue for %s: %.2Lf (%zu transactions)\n", product, productRevenue, found);
}

void sales_report(void) {
    char product[100];
    if (!get_text("Product name for revenue total (exact name): ", product, sizeof(product))) {
        return;
    }
    write_report(stdout, product);
    if (!confirm("\nSave as sales_report.txt (overwrite if present)? [y/N]: ")) {
        return;
    }
    FILE *file = fopen("sales_report.txt", "w");
    if (file == NULL) {
        perror("Could not open sales_report.txt");
        return;
    }
    write_report(file, product);
    int failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    puts(failed ? "Could not finish writing the report." : "Report saved to sales_report.txt.");
}

int save_records(void) {
    FILE *file = fopen("sales.txt.tmp", "w");
    if (file == NULL) {
        perror("Could not open the save file");
        return 0;
    }
    fprintf(file, "SALES_RECORDS_1\n%zu\n", saleCount);
    for (size_t i = 0; i < saleCount; ++i) {
        const Sale *sale = &sales[i];
        fprintf(file, "%s\n%s\n%ld\n%.17g\n%s\n",
                sale->id, sale->product, sale->quantity, sale->price, sale->date);
    }
    int failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        puts("Could not finish saving transactions. The previous save is unchanged.");
        remove("sales.txt.tmp");
        return 0;
    }
    if (rename("sales.txt.tmp", dataFile) != 0) {
        perror("Could not replace sales.txt");
        remove("sales.txt.tmp");
        return 0;
    }
    changed = 0;
    printf("Saved %zu transactions to %s.\n", saleCount, dataFile);
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
            puts("No sales.txt found. Current transactions are unchanged.");
        } else {
            perror("Could not open sales.txt");
        }
        return 0;
    }
    char text[100];
    size_t expected = 0;
    int memoryError = 0;
    int valid = read_field(file, text, sizeof(text)) &&
                strcmp(text, "SALES_RECORDS_1") == 0 && read_count(file, &expected);
    Sale *loaded = NULL;
    if (valid && expected > 0) {
        loaded = calloc(expected, sizeof(Sale));
        if (loaded == NULL) {
            valid = 0;
            memoryError = 1;
        }
    }
    for (size_t i = 0; valid && i < expected; ++i) {
        Sale *sale = &loaded[i];
        valid = read_field(file, sale->id, sizeof(sale->id)) && valid_id(sale->id) &&
                find_sale(loaded, i, sale->id) == i &&
                read_field(file, sale->product, sizeof(sale->product)) &&
                read_field(file, text, sizeof(text)) && parse_quantity(text, &sale->quantity) &&
                read_field(file, text, sizeof(text)) && parse_number(text, &sale->price) && sale->price > 0 &&
                read_field(file, sale->date, sizeof(sale->date)) && valid_date(sale->date);
        if (valid) {
            sale->amount = (long double)sale->quantity * sale->price;
        }
    }
    if (valid) {
        int result;
        while ((result = read_line(file, text, sizeof(text))) == 1) {
            if (*text) {
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
        free(loaded);
        puts(memoryError ? "Not enough memory to load transactions. Current transactions are unchanged." :
                           "Invalid or incomplete save file. Current transactions are unchanged.");
        return 0;
    }
    free(sales);
    sales = loaded;
    saleCount = expected;
    changed = 0;
    printf("Loaded %zu transactions from %s.\n", saleCount, dataFile);
    return 1;
}

int main(void) {
    load_records();
    int running = 1;
    int result = 0;
    while (running) {
        puts("\nMy Sales Records");
        puts("1. Add transaction\n2. List transactions\n3. Update transaction\n4. Delete transaction");
        puts("5. Process sales data\n6. Filter transactions\n7. Search transactions\n8. Sort transactions");
        puts("9. Sales report\n10. Save transactions\n11. Load transactions\n0. Save and exit");
        int choice;
        if (!get_int("Choose an option: ", 0, 11, &choice)) {
            puts("\nInput ended.");
            if (changed && !save_records()) {
                result = 1;
            }
            break;
        }
        switch (choice) {
            case 1: add_sale(); break;
            case 2: list_sales(); break;
            case 3: update_sale(); break;
            case 4: delete_sale(); break;
            case 5: process_sales(); break;
            case 6: filter_sales(); break;
            case 7: search_sales(); break;
            case 8: sort_sales(); break;
            case 9: sales_report(); break;
            case 10: save_records(); break;
            case 11:
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
    free(sales);
    return result;
}

