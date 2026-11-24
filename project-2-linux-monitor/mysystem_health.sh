#!/usr/bin/env bash

set -o pipefail
export LC_ALL=C

for tool in date dirname uname; do
    if ! command -v "$tool" > /dev/null 2>&1; then
        printf 'Missing command: %s\n' "$tool" >&2
        exit 1
    fi
done

if [[ "$(uname -s)" != Linux ]]; then
    printf 'Run this script on Linux, such as Ubuntu in WSL.\n' >&2
    exit 1
fi

folder=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd) || exit 1
logFile="$folder/activity.log"
cpuLimit=80
memoryLimit=80
diskLimit=80

if ! : >> "$logFile"; then
    printf 'Cannot write the activity log: %s\n' "$logFile" >&2
    exit 1
fi

log_activity() {
    printf '[%s] %s\n' "$(date '+%F %T')" "$*" >> "$logFile" ||
        printf 'Could not update the activity log.\n' >&2
}

notice() {
    printf '%s\n' "$*"
    log_activity "$*"
}

need() {
    local tool
    for tool in "$@"; do
        if ! command -v "$tool" > /dev/null 2>&1; then
            notice "Missing command: $tool. This option is unavailable."
            return 1
        fi
    done
}

ask_limit() {
    local answer
    while true; do
        read -r -p "$1 warning threshold [$2%]: " answer || return 1
        answer=${answer:-$2}
        if [[ "$answer" =~ ^([0-9]|[1-9][0-9]|100)$ ]]; then
            limit=$answer
            return 0
        fi
        notice 'Enter a whole percentage from 0 to 100.'
    done
}

system_info() {
    need hostname uname uptime awk || return 1
    printf '\nSystem information\n'
    printf 'Hostname: %s\n' "$(hostname)"
    if [[ -r /etc/os-release ]]; then
        printf 'Operating system: '
        awk -F= '$1 == "PRETTY_NAME" {gsub(/"/, "", $2); print $2}' /etc/os-release
    else
        printf 'Operating system: Linux (distribution information unavailable)\n'
    fi
    printf 'Kernel: %s\n' "$(uname -r)"
    printf 'Uptime: %s\n' "$(uptime -p)"
    printf 'Date and time: %s\n' "$(date '+%F %T %Z')"
    log_activity 'System information checked'
}

cpu_sample() {
    local label user nice system idle wait irq soft steal rest
    if ! read -r label user nice system idle wait irq soft steal rest < /proc/stat; then
        notice 'Cannot read CPU information from /proc/stat.'
        return 1
    fi
    total=$((user + nice + system + idle + wait + irq + soft + steal))
    idleTime=$((idle + wait))
}

cpu_info() {
    need sleep ps awk || return 1
    local before idleBefore elapsed busy cpu
    cpu_sample || return 1
    before=$total
    idleBefore=$idleTime
    sleep 1 || return 1
    cpu_sample || return 1
    elapsed=$((total - before))
    busy=$((elapsed - idleTime + idleBefore))
    if ((elapsed <= 0 || busy < 0 || busy > elapsed)); then
        notice 'Could not calculate CPU usage. Try again.'
        return 1
    fi
    cpu=$((100 * busy / elapsed))
    printf '\nCPU usage: %s%% (warning above %s%%)\n' "$cpu" "$cpuLimit"
    log_activity "CPU usage: $cpu%; threshold: $cpuLimit%"
    if ((cpu > cpuLimit)); then
        notice "Warning: CPU usage is above $cpuLimit%."
    fi
    printf '\nTop CPU processes\n'
    if ! ps -eo pid,comm,pcpu,pmem --sort=-pcpu | awk 'NR <= 6'; then
        notice 'Could not read the process list.'
        return 1
    fi
}

memory_info() {
    need free awk || return 1
    local stats memoryTotal memoryUsed memoryFree usage
    if ! stats=$(free -m); then
        notice 'Could not read memory information.'
        return 1
    fi
    read -r memoryTotal memoryUsed memoryFree < <(awk '/^Mem:/ {print $2, $3, $7}' <<< "$stats")
    if [[ ! "$memoryTotal" =~ ^[0-9]+$ ]] || ((memoryTotal == 0)); then
        notice 'Could not calculate memory usage.'
        return 1
    fi
    usage=$((100 * memoryUsed / memoryTotal))
    printf '\nMemory (MiB)\nTotal: %s\nUsed: %s\nAvailable: %s\n' "$memoryTotal" "$memoryUsed" "$memoryFree"
    printf 'Usage: %s%% (warning above %s%%)\n' "$usage" "$memoryLimit"
    log_activity "Memory usage: $usage%; threshold: $memoryLimit%"
    if ((usage > memoryLimit)); then
        notice "Warning: memory usage is above $memoryLimit%."
    fi
}

disk_info() {
    need df || return 1
    local rows disk size used available percent mount usage
    if ! rows=$(df -hP); then
        notice 'Could not read all mounted filesystems.'
        return 1
    fi
    printf '\nDisk usage (warning above %s%%)\n%s\n' "$diskLimit" "$rows"
    log_activity "Disk usage checked; threshold: $diskLimit%"
    while read -r disk size used available percent mount; do
        [[ "$percent" =~ ^[0-9]+%$ ]] || continue
        usage=${percent%%%}
        if ((usage > diskLimit)); then
            notice "Warning: $mount is $usage% full (threshold: $diskLimit%)."
        fi
    done <<< "$rows"
}

processes() {
    need ps awk || return 1
    local search matches
    if ! ps -eo pid,comm,pcpu,pmem --sort=-pcpu; then
        notice 'Could not read the process list.'
        return 1
    fi
    log_activity 'Running processes viewed'
    read -r -p 'Search by name or PID (Enter to return): ' search || return 1
    [[ -n "$search" ]] || return 0
    log_activity "Process search: $search"
    if [[ "$search" =~ ^[0-9]+$ ]]; then
        if ! ps -p "$search" -o pid,comm,pcpu,pmem; then
            notice "No available process with PID $search."
        fi
    else
        if ! matches=$(ps -eo pid,comm,pcpu,pmem | awk -v term="$search" '
            NR == 1 {header = $0; next}
            index(tolower($2), tolower(term)) {
                if (!found++) print header
                print
            }'); then
            notice 'Could not search the process list.'
            return 1
        fi
        if [[ -n "$matches" ]]; then
            printf '%s\n' "$matches"
        else
            notice "No processes matched: $search"
        fi
    fi
}

terminate_process() {
    need ps id || return 1
    local pid owner name parent answer
    read -r -p 'PID to terminate: ' pid || return 1
    if [[ ! "$pid" =~ ^[1-9][0-9]*$ ]]; then
        notice 'Invalid PID. Enter a positive whole number.'
        return 1
    fi
    if ! read -r owner name < <(ps -p "$pid" -o uid=,comm=); then
        notice "Process $pid is unavailable."
        return 1
    fi
    if [[ "$owner" == 0 || "$owner" != "$(id -u)" ]]; then
        notice 'Protected process: only your own non-root processes can be terminated.'
        return 1
    fi
    case "$name" in
        systemd*|init|sshd|dbus-daemon|dbus-broker|gnome-shell|gnome-session*|plasmashell|Xorg|Xwayland)
            notice 'This system or desktop process is protected.'
            return 1
            ;;
    esac
    parent=$$
    while [[ "$parent" =~ ^[0-9]+$ ]] && ((parent > 1)); do
        if [[ "$pid" == "$parent" ]]; then
            notice 'The monitor and its parent processes are protected.'
            return 1
        fi
        parent=$(ps -p "$parent" -o ppid=) || break
        parent=${parent//[[:space:]]/}
    done
    read -r -p "Terminate PID $pid ($name)? [y/N]: " answer || return 1
    if [[ "$answer" != y && "$answer" != Y ]]; then
        notice "Termination cancelled for PID $pid."
        return 0
    fi
    if kill -TERM -- "$pid"; then
        notice "Termination signal sent to PID $pid ($name)."
    else
        notice "Could not terminate PID $pid. It may have exited or permission was denied."
        return 1
    fi
}

health_report() {
    need mkdir || return 1
    local saveFolder stamp reportFile count content status
    read -r -p 'Save reports in which directory? (Enter for current directory): ' saveFolder || return 1
    saveFolder=${saveFolder:-$PWD}
    if ! mkdir -p -- "$saveFolder"; then
        notice "Cannot access or create directory: $saveFolder"
        return 1
    fi
    stamp=$(date '+%Y%m%d-%H%M%S')
    reportFile="$saveFolder/health-$stamp.txt"
    count=1
    while [[ -e "$reportFile" ]]; do
        reportFile="$saveFolder/health-$stamp-$count.txt"
        count=$((count + 1))
    done
    printf 'Collecting system health information...\n'
    if content=$(
        status=0
        printf 'System health report\nGenerated: %s\n' "$(date '+%F %T %Z')"
        system_info || status=1
        cpu_info || status=1
        memory_info || status=1
        disk_info || status=1
        exit "$status"
    ); then
        status=0
    else
        status=1
    fi
    if ! printf '%s\n' "$content" > "$reportFile"; then
        notice "Cannot write report: $reportFile"
        return 1
    fi
    if ((status == 0)); then
        notice "Health report saved: $reportFile"
    else
        notice "Health report saved with unavailable sections: $reportFile"
    fi
}

view_log() {
    need cat || return 1
    log_activity 'Activity log viewed'
    if ! cat -- "$logFile"; then
        notice 'Could not read the activity log.'
        return 1
    fi
}

clear_log() {
    local answer
    read -r -p 'Clear the activity log? [y/N]: ' answer || return 1
    if [[ "$answer" != y && "$answer" != Y ]]; then
        notice 'Log clearing cancelled.'
        return 0
    fi
    if ! : > "$logFile"; then
        notice 'Could not clear the activity log.'
        return 1
    fi
    notice 'Activity log cleared.'
}

log_activity 'Application started'

while true; do
    printf '\nMy System Health\n'
    printf '1. System information\n2. CPU usage\n3. Memory usage\n4. Disk usage\n'
    printf '5. View and search processes\n6. Terminate a process\n7. Generate health report\n'
    printf '8. View activity log\n9. Clear activity log\n0. Exit\n'
    read -r -p 'Choose an option: ' choice || break
    case "$choice" in
        1) system_info ;;
        2) ask_limit CPU "$cpuLimit" && { cpuLimit=$limit; cpu_info; } ;;
        3) ask_limit Memory "$memoryLimit" && { memoryLimit=$limit; memory_info; } ;;
        4) ask_limit Disk "$diskLimit" && { diskLimit=$limit; disk_info; } ;;
        5) processes ;;
        6) terminate_process ;;
        7) health_report ;;
        8) view_log ;;
        9) clear_log ;;
        0) log_activity 'Application closed'; printf 'Goodbye!\n'; exit 0 ;;
        *) notice 'Invalid option. Choose a number from the menu.' ;;
    esac
    printf '\n'
    read -r -p 'Press Enter to return to the menu...' || break
done

log_activity 'Application closed after input ended'
