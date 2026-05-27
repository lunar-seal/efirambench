#!/usr/bin/env bash
# Boot build/efiram-{bios,uefi}.iso in QEMU, capture serial output,
# and assert that the benchmark banner and at least one "Report N"
# line appear within the timeout.
#
# usage: tests/qemu/run.sh bios|uefi
#
# env overrides:
#   QEMU_TEST_TIMEOUT   seconds to wait for marker (default 120)
#   QEMU_TEST_MEMORY    QEMU -m value (default 4G)
#   OVMF_CODE           path to OVMF_CODE.fd  (UEFI mode only)
#   OVMF_VARS           path to OVMF_VARS.fd  (UEFI mode only)
#   QEMU                qemu-system-x86_64 binary (default: from PATH)

set -euo pipefail

mode="${1:-}"
case "$mode" in
    bios|uefi) ;;
    *) echo "usage: $0 bios|uefi" >&2; exit 2 ;;
esac

repo_root="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$repo_root"

iso="build/efiram-${mode}.iso"
if [ ! -f "$iso" ]; then
    echo "missing $iso (build it with 'make ${mode}-iso' first)" >&2
    exit 2
fi

QEMU="${QEMU:-qemu-system-x86_64}"
if ! command -v "$QEMU" >/dev/null 2>&1; then
    echo "qemu-system-x86_64 not found (set \$QEMU or install qemu)" >&2
    exit 2
fi

timeout_s="${QEMU_TEST_TIMEOUT:-120}"
memory="${QEMU_TEST_MEMORY:-4G}"

work="$(mktemp -d)"
serial_log="$work/serial.log"
pid_file="$work/qemu.pid"
: >"$serial_log"

cleanup() {
    if [ -f "$pid_file" ]; then
        local pid
        pid="$(cat "$pid_file" 2>/dev/null || true)"
        if [ -n "${pid:-}" ] && kill -0 "$pid" 2>/dev/null; then
            kill -TERM "$pid" 2>/dev/null || true
            for _ in 1 2 3 4 5 6 7 8 9 10; do
                kill -0 "$pid" 2>/dev/null || break
                sleep 0.2
            done
            kill -KILL "$pid" 2>/dev/null || true
        fi
    fi
    rm -rf "$work"
}
trap cleanup EXIT INT TERM

qemu_args=(
    -machine q35
    -m "$memory"
    -no-reboot
    -display none
    -monitor none
    -serial "file:$serial_log"
    -pidfile "$pid_file"
    -daemonize
)

if [ "$mode" = "uefi" ]; then
    # locate OVMF firmware
    if [ -z "${OVMF_CODE:-}" ]; then
        for c in \
            /usr/share/OVMF/OVMF_CODE_4M.fd \
            /usr/share/OVMF/OVMF_CODE.fd \
            /usr/share/ovmf/OVMF.fd \
            /usr/share/edk2-ovmf/x64/OVMF_CODE.fd \
            /usr/share/edk2/ovmf/OVMF_CODE.fd \
            /usr/share/qemu/OVMF.fd; do
            [ -f "$c" ] && OVMF_CODE="$c" && break
        done
    fi
    if [ -z "${OVMF_VARS:-}" ]; then
        for c in \
            /usr/share/OVMF/OVMF_VARS_4M.fd \
            /usr/share/OVMF/OVMF_VARS.fd \
            /usr/share/edk2-ovmf/x64/OVMF_VARS.fd \
            /usr/share/edk2/ovmf/OVMF_VARS.fd; do
            [ -f "$c" ] && OVMF_VARS="$c" && break
        done
    fi
    if [ -z "${OVMF_CODE:-}" ] || [ -z "${OVMF_VARS:-}" ]; then
        echo "OVMF firmware files not found (set OVMF_CODE / OVMF_VARS)" >&2
        exit 2
    fi

    vars_copy="$work/OVMF_VARS.fd"
    cp "$OVMF_VARS" "$vars_copy"
    chmod u+w "$vars_copy"

    qemu_args+=(
        -drive "if=pflash,format=raw,readonly=on,file=$OVMF_CODE"
        -drive "if=pflash,format=raw,file=$vars_copy"
        -cdrom "$iso"
    )
else
    qemu_args+=(-cdrom "$iso")
fi

echo "== launching: $QEMU ${qemu_args[*]}"
"$QEMU" "${qemu_args[@]}"

qemu_pid="$(cat "$pid_file")"
echo "== qemu pid=$qemu_pid  timeout=${timeout_s}s  mode=$mode"

banner_re='EFIRAM'
report_re='Report 1'

saw_banner=0
saw_report=0
deadline=$(( $(date +%s) + timeout_s ))

while [ "$(date +%s)" -lt "$deadline" ]; do
    if [ -s "$serial_log" ]; then
        if [ $saw_banner -eq 0 ] && grep -q "$banner_re" "$serial_log"; then
            saw_banner=1
            echo "== saw banner"
        fi
        if [ $saw_report -eq 0 ] && grep -q "$report_re" "$serial_log"; then
            saw_report=1
            echo "== saw Report"
            break
        fi
    fi
    if ! kill -0 "$qemu_pid" 2>/dev/null; then
        echo "== qemu exited before marker observed" >&2
        break
    fi
    sleep 1
done

echo "--- serial output (${serial_log}) ---"
cat "$serial_log" || true
echo "--- end serial output ---"

if [ $saw_banner -eq 1 ] && [ $saw_report -eq 1 ]; then
    echo "PASS ($mode): EFIRAM banner and Report observed"
    exit 0
fi

echo "FAIL ($mode): banner=$saw_banner report=$saw_report" >&2
exit 1
