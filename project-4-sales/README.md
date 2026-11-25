# My Sales Records

A menu-driven C program for recording sales, searching and sorting transactions, processing sales data, and generating reports.

## Run

From the repository root in an Ubuntu or WSL terminal:

```bash
cd project-4-sales
gcc -std=c11 -Wall -Wextra -Wpedantic mysales.c -o a.out
./a.out
```

Saved transactions load at startup. Option 10 saves changes, option 11 reloads transactions, and option 0 saves and exits. Option 9 displays a sales report and offers to save it as `sales_report.txt`.

## Code

`Sale` stores a transaction ID, product name, quantity, unit price, total amount, and date. The total is calculated from quantity and price when adding, updating, or loading a transaction.

Option 5 uses the `Operation` function-pointer type to select total revenue, average transaction value, highest value, lowest value, or total units at runtime. The selected pointer is called with the sales array and transaction count.

Option 6 passes a `Match` callback to `matching_sales()`. That function invokes the callback for each transaction, displays matches, and counts them. The callbacks filter by an amount threshold or a partial product name.

Searching uses manual linear searches for ID, product name, or amount. ID matches ignore case; product searches support partial matches. Amount searches allow small decimal rounding differences. Sorting uses manual bubble sort with a `Compare` callback for ID, product name, or amount, in either direction. No `qsort()` is used.

The sales array grows and shrinks using `realloc()`. Loading uses `calloc()`, and allocation results are checked. Records are released with `free()` when replaced or when the program exits.

Transactions are stored in `sales.txt`. Saving writes a temporary file before replacing the previous save. Loading validates new records before replacing the current array and recalculates transaction totals.

Validation rejects duplicate IDs, invalid menu input, non-positive quantities or prices, and invalid calendar dates. Missing records, file errors, and allocation failures produce messages. Empty data is handled without dividing by zero.

## Sample data and report

- [Sales records](sales.txt): T001, two Coffee units at 2500; T002, three Bread units at 1000; T003, four Coffee units at 2500.
- [Sales report](sales_report.txt): three transactions, revenue 18000.00, average value 6000.00, highest value 10000.00, lowest value 3000.00, nine units sold, and Coffee revenue 15000.00.

## Testing and outputs

Sixty-three checks passed in Ubuntu, covering CRUD operations, all five calculation operations, callback filtering, manual searches and sorts, reports, decimal prices, dates, persistence, invalid input, corrupt files, and allocation failures. The checks included compiler warnings, AddressSanitizer, and UndefinedBehaviorSanitizer.

Manual execution was recorded as terminal transcripts in `outputs/`. The temporary TTEST transaction used for updating and deletion is absent from the final sample data.

| Output | Result |
| --- | --- |
| [Processing](outputs/processing_output.txt) | Added and listed the three transactions, then ran all five function-pointer calculations with the expected results |
| [Callbacks](outputs/callbacks) | Filtering above 4000 returned T001 and T003, with two matches |
| [Search](outputs/search) | Searching by ID T002 returned the Bread transaction |
| [Sort](outputs/sort) | Sorted the three transaction IDs in descending and ascending order |
| [Update and validation](outputs/update) | Rejected invalid input, duplicate IDs, negative values, and an invalid date; updating TTEST recalculated its total to 1500.00 |
| [Delete](outputs/delete) | Cancelled deletion once, then confirmed it and listed the three remaining transactions |
| [Save and load](outputs/save-load) | Saved three transactions and loaded them after restarting the program |
