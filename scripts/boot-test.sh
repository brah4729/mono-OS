#!/usr/bin/env bash
# scripts/boot-test.sh — boot an ISO headless in QEMU and assert serial-log markers
# usage: scripts/boot-test.sh <qemu-binary> <iso> <timeout-seconds> <marker>...
#        DISK=monoos-disk.img scripts/boot-test.sh ...   (attach IDE disk)
set -u
QEMU=$1; ISO=$2; T=$3; shift 3
LOG=$(mktemp)
DISK_ARGS=()
if [ -n "${DISK:-}" ]; then DISK_ARGS=(-drive "file=$DISK,format=raw,if=ide"); fi

timeout "$T" "$QEMU" -m 256M -vga std -display none -no-reboot \
    -serial "file:$LOG" -cdrom "$ISO" "${DISK_ARGS[@]}" >/dev/null 2>&1

fail=0
for m in "$@"; do
    if grep -qF -- "$m" "$LOG"; then echo "PASS: $m"; else echo "FAIL: missing '$m'"; fail=1; fi
done
if grep -qE 'PANIC|PAGE FAULT|\[TEST\].*FAIL' "$LOG"; then
    echo "FAIL: fault/test failure in log:"
    grep -E 'PANIC|PAGE FAULT|\[TEST\].*FAIL' "$LOG"
    fail=1
fi
if [ "$fail" != 0 ]; then echo "--- serial log ---"; cat "$LOG"; fi
rm -f "$LOG"
exit "$fail"
