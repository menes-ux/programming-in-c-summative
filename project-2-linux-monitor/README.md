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

The activity log is generated when you run the script. Health reports are generated only when requested from the menu.
