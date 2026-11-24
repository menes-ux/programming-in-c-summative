# My System Health

An interactive Bash script for checking a Linux computer's resources, searching processes, and saving health reports. It runs on Ubuntu, including Ubuntu in WSL.

## Run

From the repository root in an Ubuntu terminal:

```bash
cd project-2-linux-monitor
bash mysystem_health.sh
```

Choose an option from the menu, then press Enter to return. CPU, memory, and disk warnings default to 80%. Enter a whole percentage from 0 to 100 when prompted; the chosen threshold stays in use for the session. CPU measurement takes one second.

## What it does

- Shows hostname, operating system, kernel, uptime, date, and time.
- Checks CPU, memory, and mounted filesystem usage, with threshold warnings.
- Lists processes and searches by PID or name, ignoring case for names.
- Sends a termination signal after confirmation. Root-owned processes, other users' processes, selected system services, and the monitor and its parent processes are protected.
- Saves timestamped health reports in a directory you choose.
- Records actions, monitoring results, and warnings in `activity.log` beside the script. Viewing and clearing the log are menu options; clearing requires confirmation.

## Code

Each menu option calls a Bash function. `system_info`, `cpu_info`, `memory_info`, and `disk_info` collect monitoring data and are reused by `health_report`. `processes` handles listing and searching; `terminate_process` checks ownership and protected processes before asking for confirmation.

The script uses standard Ubuntu commands, including `hostname`, `uname`, `uptime`, `ps`, `free`, `df`, `awk`, and `date`. CPU usage is calculated from two readings of `/proc/stat`, one second apart. `log_activity` writes timestamped entries; `ask_limit` validates warning thresholds.

Invalid menu options and thresholds are rejected. Missing utilities, unavailable processes, permission errors, and inaccessible report directories produce a message and a log entry. A report notes unavailable sections if a monitoring command cannot run.

## Checks

Fifteen scenario checks passed in Ubuntu, covering monitoring, invalid inputs, process searches, protected processes, cancelled and confirmed termination, report creation and permission errors, log viewing and clearing, missing utilities, and closed input. Termination was tested using a temporary `sleep` process created for the check.

Manual testing on Ubuntu 24.04.1 in WSL also confirmed a memory warning at a 0% threshold, searching by PID, confirmed termination, rejection of a protected process, handling of an unavailable PID and invalid menu input, report generation, and log viewing.

## Screenshots

| Screenshot | Result |
| --- | --- |
| [CPU](screenshots/cpu.png) | CPU usage, 80% threshold, and top processes displayed |
| [Memory](screenshots/memory.png) | Total, used, and available memory displayed; 7% usage triggered a warning with a 0% threshold |
| [Disk](screenshots/disk.png) | Mounted filesystems, space, and percentages displayed; the Windows drive was 71% full |
| [Process search](screenshots/process.png) | Searching for PID 1832 returned the matching sleep process |
| [Process termination](screenshots/process_termination.png) | Confirmation accepted and a termination signal sent to the temporary sleep process, PID 2238 |

## Sample output

- [Health report](reports/health-20261124-192250.txt): timestamped system information, CPU, memory, disk, and uptime from the sample run.
- [Activity log](activity.log): monitoring results, a warning, process searches and termination, rejected operations, report generation, and log viewing.

The activity log is generated beside the script. Health reports are generated only when requested; the sample report is kept in `reports/`.
