#!/usr/bin/env bash
# Compatible with the Bash 3.2 shipped with macOS.
set -euo pipefail

project_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)

usage() {
    cat <<'HELP'
Usage:
  ./benchmark.sh --width RANGE --height RANGE --mines RANGE --runs N [options]

RANGE is a single integer or START:END[:STEP], including END when reachable.
Example:
  ./benchmark.sh --width 16:32:8 --height 16 --mines 20:60:20 --runs 1000

Options:
  --output FILE    CSV destination; existing CSV/log files are never overwritten.
                  Default: a new build/benchmark.XXXXXX/results.csv directory.
  --bot FILE       Use an existing executable instead of compiling current sources.
  --timeout N      Stop the entire batch after N seconds (0 = unlimited, default).
  --dry-run        Print test commands without compiling or running the bot.
  -h, --help       Show this help.

Each test command runs N games. All groups use one bot process.
Width and height must be at least 9 because the bot starts at (8, 8).
Groups that cannot have a zero at that starting cell are skipped.
The bot regenerates unsuccessful starting positions; results describe accepted
boards only. "stuck" means the solver found no move, not necessarily that no
safe move exists. Losses/errors abort the batch and remain in the log.
HELP
}

die() { printf 'Error: %s\n' "$*" >&2; exit 1; }

positive_int() {
    local label=$1 value=$2 number
    [[ $value =~ ^[0-9]{1,10}$ ]] || die "$label must be a positive integer."
    number=$((10#$value))
    (( number > 0 && number <= 2147483647 )) || die "$label must be in 1..2147483647."
    printf '%s' "$number"
}

parse_range() {
    local label=$1 spec=$2 first last step
    [[ $spec =~ ^([0-9]+)(:([0-9]+)(:([0-9]+))?)?$ ]] ||
        die "$label must be NUMBER or START:END[:STEP]."
    first=${BASH_REMATCH[1]}
    last=${BASH_REMATCH[3]:-$first}
    step=${BASH_REMATCH[5]:-1}
    first=$(positive_int "$label start" "$first") || return 1
    last=$(positive_int "$label end" "$last") || return 1
    step=$(positive_int "$label step" "$step") || return 1
    (( first <= last )) || die "$label start must not exceed end."
    printf '%s %s %s\n' "$first" "$last" "$step"
}

width_spec= height_spec= mines_spec= runs= output= bot_file=
time_limit=0
dry_run=0
while (( $# )); do
    case $1 in
        -h|--help) usage; exit 0 ;;
        --dry-run) dry_run=1; shift ;;
        --width|--height|--mines|--runs|--output|--bot|--timeout)
            (( $# >= 2 )) || die "Missing value for $1."
            case $1 in
                --width) width_spec=$2 ;;
                --height) height_spec=$2 ;;
                --mines) mines_spec=$2 ;;
                --runs) runs=$2 ;;
                --output) output=$2 ;;
                --bot) bot_file=$2 ;;
                --timeout) time_limit=$2 ;;
            esac
            shift 2 ;;
        *) die "Unknown argument: $1. Use --help." ;;
    esac
done
[[ -n $width_spec && -n $height_spec && -n $mines_spec && -n $runs ]] || {
    usage >&2
    exit 1
}
width_range=$(parse_range width "$width_spec") || exit 1
height_range=$(parse_range height "$height_spec") || exit 1
mines_range=$(parse_range mines "$mines_spec") || exit 1
read -r width_first width_last width_step <<< "$width_range"
read -r height_first height_last height_step <<< "$height_range"
read -r mines_first mines_last mines_step <<< "$mines_range"
runs=$(positive_int runs "$runs") || exit 1
if [[ $time_limit != 0 ]]; then
    time_limit=$(positive_int timeout "$time_limit") || exit 1
fi
(( width_first >= 9 && height_first >= 9 )) ||
    die "The current bot requires width and height >= 9 (start cell is (8, 8))."
(( width_last * height_last <= 2147483647 )) ||
    die "The field area must fit in a C int."

temporary_dir=$(mktemp -d "${TMPDIR:-/tmp}/minesweeper-benchmark.XXXXXX")
bot_pid= timer_pid=
cleanup() {
    if [[ -n $bot_pid ]]; then
        kill "$bot_pid" 2>/dev/null || true
        wait "$bot_pid" 2>/dev/null || true
    fi
    if [[ -n $timer_pid ]]; then
        kill "$timer_pid" 2>/dev/null || true
        wait "$timer_pid" 2>/dev/null || true
    fi
    rm -rf -- "$temporary_dir"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

groups=0
skipped=0
commands_file=$temporary_dir/commands.txt
parameters_file=$temporary_dir/parameters.txt
: > "$commands_file"
: > "$parameters_file"
for (( width=width_first; width<=width_last; width+=width_step )); do
    for (( height=height_first; height<=height_last; height+=height_step )); do
        # A corner/edge start needs 4/6 clear cells; an interior start needs 9.
        clear_width=3; clear_height=3
        if (( width == 9 )); then clear_width=2; fi
        if (( height == 9 )); then clear_height=2; fi
        max_mines=$((width * height - clear_width * clear_height))
        for (( mines=mines_first; mines<=mines_last; mines+=mines_step )); do
            if (( mines > max_mines )); then
                skipped=$((skipped + 1))
                continue
            fi
            printf 'test %d %d %d %d\n' "$width" "$height" "$mines" "$runs" >> "$commands_file"
            printf '%d %d %d %d\n' "$width" "$height" "$mines" "$runs" >> "$parameters_file"
            groups=$((groups + 1))
        done
    done
done
(( groups > 0 )) || die "No valid groups: the bot requires a mine-free starting neighbourhood."
printf 'Groups: %d; games per group: %d; skipped combinations: %d\n' "$groups" "$runs" "$skipped" >&2
if (( dry_run )); then
    cat "$commands_file"
    exit 0
fi
printf 'exit\n' >> "$commands_file"

if [[ -n $bot_file ]]; then
    [[ -f $bot_file && -x $bot_file ]] || die "Bot is not an executable file: $bot_file"
    bot_file=$(cd -- "$(dirname -- "$bot_file")" && printf '%s/%s' "$PWD" "$(basename -- "$bot_file")")
else
    bot_file=$temporary_dir/bot
    "${CC:-cc}" -std=c11 -O2 "$project_dir/src/bot.c" \
        "$project_dir/src/minesweeper.c" "$project_dir/src/lcg.c" -o "$bot_file"
fi
if [[ -z $output ]]; then
    mkdir -p "$project_dir/build"
    report_dir=$(mktemp -d "$project_dir/build/benchmark.XXXXXX")
    output=$report_dir/results.csv
fi
log_file=${output%.csv}.log
[[ $output != "$log_file" ]] || die "Output must have a filename different from its .log companion."
[[ ! -e $output && ! -e $log_file ]] || die "CSV or log already exists: $output / $log_file"
mkdir -p -- "$(dirname -- "$output")"
printf 'Running batch; log: %s\n' "$log_file" >&2

started_at=$SECONDS
"$bot_file" < "$commands_file" > "$log_file" 2>&1 &
bot_pid=$!
if (( time_limit > 0 )); then
    # Bash read supplies a portable timer without a GNU timeout dependency.
    mkfifo "$temporary_dir/timer"
    exec 3<> "$temporary_dir/timer"
    (
        if ! IFS= read -r -t "$time_limit" timer_signal <&3; then
            : > "$temporary_dir/timed-out"
            kill "$bot_pid" 2>/dev/null || true
        fi
    ) &
    timer_pid=$!
fi
status=0
wait "$bot_pid" || status=$?
bot_pid=
if [[ -n $timer_pid ]]; then
    printf 'done\n' >&3
    wait "$timer_pid" || true
    timer_pid=
    exec 3>&-
fi
[[ ! -e $temporary_dir/timed-out ]] || die "Batch exceeded ${time_limit}s; incomplete output is in $log_file"
(( status == 0 )) || die "Bot exited with status $status; details: $log_file"

# Read exact win counts; ignore the rounded percentage printed by the bot.
if ! LC_ALL=C awk '
    NR == FNR { width[NR]=$1; height[NR]=$2; mines[NR]=$3; runs[NR]=$4; groups=NR; next }
    /побед\/из:/ {
        line=$0
        sub(/^.*побед\/из:[[:space:]]*/, "", line)
        sub(/\).*$/, "", line)
        if (line !~ /^[0-9]+\/[0-9]+$/) { bad=1; exit 1 }
        split(line, counts, "/")
        n++
        if (n > groups || counts[2] != runs[n] || counts[1] > counts[2]) { bad=1; exit 1 }
        printf "%d,%d,%d,%d,%d,%d,%.2f\n", width[n],height[n],mines[n],runs[n],counts[1],runs[n]-counts[1],100*counts[1]/runs[n]
    }
    END { if (bad || n != groups) exit 1 }
' "$parameters_file" "$log_file" > "$temporary_dir/rows.csv"; then
    die "Unexpected/incomplete bot statistics; inspect $log_file"
fi
{
    printf 'width,height,mines,runs,wins,stuck,win_percent\n'
    cat "$temporary_dir/rows.csv"
} > "$output"
LC_ALL=C awk -F, '
    NR == 1 { printf "%7s %7s %7s %8s %8s %8s %8s\n", "WIDTH","HEIGHT","MINES","RUNS","WINS","STUCK","WIN %"; next }
    { printf "%7d %7d %7d %8d %8d %8d %8.2f\n", $1,$2,$3,$4,$5,$6,$7 }
' "$output"
printf '\nCSV: %s\nLog: %s\nElapsed: %ds\n' "$output" "$log_file" "$((SECONDS - started_at))"
