#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build}"
case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$ROOT_DIR/$BUILD_DIR" ;;
esac
BIN="$BUILD_DIR/flatcore"
BIN_LINKS="$BUILD_DIR/bin"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

[[ -x "$BIN" ]] || { printf 'error: missing executable: %s\n' "$BIN" >&2; exit 1; }
"$BIN" true
if "$BIN" false; then
    printf 'false returned success unexpectedly\n' >&2
    exit 1
fi

expected='Flatcore smoke test'
actual="$("$BIN" echo "$expected")"
[[ "$actual" == "$expected" ]]
[[ "$("$BIN" echo -n "$expected")" == "$expected" ]]
[[ "$("$BIN" basename '/tmp/flatcore-test/')" == 'flatcore-test' ]]
[[ "$("$BIN" dirname '/tmp/flatcore-test/file.txt')" == '/tmp/flatcore-test' ]]
[[ "$("$BIN" pwd)" == "$(pwd -P)" ]]
[[ "$("$BIN" basename 'C:\folder\file.txt')" == 'file.txt' ]]
[[ "$("$BIN" basename '/')" == '/' ]]
[[ "$("$BIN" basename 'C:\')" == '\' ]]
[[ "$("$BIN" dirname 'file.txt')" == '.' ]]
[[ "$("$BIN" dirname '/file.txt')" == '/' ]]
[[ "$("$BIN" dirname 'C:\folder\file.txt')" == 'C:\folder' ]]
[[ "$("$BIN" dirname 'C:\file.txt')" == 'C:\' ]]
if "$BIN" pwd unexpected >/dev/null 2>&1; then
    printf 'pwd accepted an unexpected operand\n' >&2
    exit 1
fi

(
    cd "$TMP_DIR"
    [[ "$("$BIN" pwd)" == "$TMP_DIR" ]]
    "$BIN" touch sample.txt
    "$BIN" echo 'assembly userspace' > input.txt
    printf 'first\000chunk\n' > cat-first.bin
    printf 'second\r\n' > cat-second.bin
    : > cat-empty.bin
    cat cat-first.bin cat-second.bin > cat-expected.bin
    "$BIN" cat cat-first.bin cat-second.bin > cat-actual.bin
    cmp cat-expected.bin cat-actual.bin
    "$BIN" cat cat-empty.bin > cat-empty-output.bin
    cmp cat-empty.bin cat-empty-output.bin
    "$BIN" cat < cat-first.bin > cat-stdin-output.bin
    cmp cat-first.bin cat-stdin-output.bin
    "$BIN" cat - < cat-second.bin > cat-dash-output.bin
    cmp cat-second.bin cat-dash-output.bin
    if "$BIN" cat missing-cat-file cat-first.bin > cat-after-error.bin 2>/dev/null; then
        printf 'cat succeeded despite a missing input file\n' >&2
        exit 1
    fi
    cmp cat-first.bin cat-after-error.bin
    "$BIN" cp input.txt copied.txt
    cmp input.txt copied.txt
    "$BIN" mv copied.txt moved.txt
    cmp input.txt moved.txt

    "$BIN" mkdir ls-dir
    printf 'visible\n' > ls-dir/visible
    printf 'secret\n' > ls-dir/.hidden
    printf 'first\n' > ls-file-one
    printf 'second\n' > ls-file-two
    printf 'leading\n' > ./-ls-leading
    "$BIN" ls ls-dir > ls-default.txt
    grep -qx 'visible' ls-default.txt
    if grep -q '\.hidden' ls-default.txt; then
        printf 'ls showed a hidden file by default\n' >&2
        exit 1
    fi
    "$BIN" ls -la ls-dir > ls-long.txt
    grep -Eq '^d[rwx-]{9} ' ls-long.txt
    grep -q '\.hidden$' ls-long.txt
    grep -q 'visible$' ls-long.txt
    "$BIN" ls -A ls-dir > ls-almost.txt
    grep -qx '\.hidden' ls-almost.txt
    if grep -qx '\.' ls-almost.txt || grep -qx '\.\.' ls-almost.txt; then
        printf 'ls -A included dot entries\n' >&2
        exit 1
    fi
    "$BIN" ls -a ls-dir > ls-all.txt
    grep -qx '\.' ls-all.txt
    grep -qx '\.\.' ls-all.txt
    [[ "$("$BIN" ls -d ls-dir)" == 'ls-dir' ]]
    "$BIN" ls ls-file-one ls-file-two > ls-multiple.txt
    grep -qx 'ls-file-one' ls-multiple.txt
    grep -qx 'ls-file-two' ls-multiple.txt
    "$BIN" ls -- ./-ls-leading > ls-leading.txt
    grep -qx './-ls-leading' ls-leading.txt
    "$BIN" ls --version > ls-version.txt
    grep -q 'Flatcore ls' ls-version.txt
    "$BIN" ls --help > ls-help.txt
    grep -q -- '--almost-all' ls-help.txt
    if "$BIN" ls --not-a-real-option >/dev/null 2>&1; then
        printf 'ls accepted an unknown option\n' >&2
        exit 1
    fi

    # mkdir: recursive creation, existing parents, modes, verbose output.
    "$BIN" mkdir -p mkdir-tree/one/two
    [[ -d mkdir-tree/one/two ]]
    "$BIN" mkdir -p mkdir-tree/one/two
    "$BIN" mkdir -pv mkdir-verbose/a/b > mkdir-verbose.txt
    [[ -d mkdir-verbose/a/b ]]
    grep -q "created directory 'mkdir-verbose" mkdir-verbose.txt
    "$BIN" mkdir -m 700 mkdir-mode
    [[ "$(stat -c '%a' mkdir-mode)" == '700' ]]
    "$BIN" mkdir --mode=755 mkdir-mode-long
    [[ "$(stat -c '%a' mkdir-mode-long)" == '755' ]]
    "$BIN" mkdir -- -mkdir-leading
    [[ -d ./-mkdir-leading ]]
    if "$BIN" mkdir mkdir-mode; then
        printf 'mkdir succeeded for an existing directory without -p\n' >&2
        exit 1
    fi
    if "$BIN" mkdir -m 888 mkdir-invalid-mode; then
        printf 'mkdir accepted an invalid octal mode\n' >&2
        exit 1
    fi
    printf 'not a directory\n' > mkdir-blocker
    if "$BIN" mkdir -p mkdir-blocker/child; then
        printf 'mkdir -p accepted a non-directory parent\n' >&2
        exit 1
    fi
    "$BIN" mkdir --version > mkdir-version.txt
    grep -q 'Flatcore mkdir' mkdir-version.txt
    "$BIN" mkdir --help > mkdir-help.txt
    grep -q -- '--parents' mkdir-help.txt

    "$BIN" rm mkdir-blocker
    "$BIN" rm --help > rm-help.txt
    grep -q -- '--force' rm-help.txt
    "$BIN" rm --version > rm-version.txt
    grep -q 'Flatcore rm' rm-version.txt
    "$BIN" rm -f missing-rm-file
    ln -s missing-rm-target rm-dangling-link
    "$BIN" rm rm-dangling-link
    [[ ! -L rm-dangling-link ]]
    ln -s missing-rm-target rm-force-dangling-link
    "$BIN" rm -f rm-force-dangling-link
    [[ ! -L rm-force-dangling-link ]]
    printf 'keep target\\n' > rm-link-target
    ln -s rm-link-target rm-file-link
    "$BIN" rm rm-file-link
    [[ -f rm-link-target && ! -L rm-file-link ]]
    printf 'verbose removal\\n' > rm-verbose-file
    "$BIN" rm -v rm-verbose-file > rm-verbose.txt
    grep -q "removed 'rm-verbose-file'" rm-verbose.txt
    "$BIN" mkdir rm-empty-dir
    "$BIN" rm -dv rm-empty-dir > rm-dir-verbose.txt
    grep -q "removed 'rm-empty-dir'" rm-dir-verbose.txt
    "$BIN" mkdir rm-nonempty-dir
    printf 'keep\\n' > rm-nonempty-dir/file
    if "$BIN" rm -d rm-nonempty-dir; then
        printf 'rm -d removed a non-empty directory\\n' >&2
        exit 1
    fi
    "$BIN" rm rm-nonempty-dir/file
    "$BIN" rm -d rm-nonempty-dir
    printf 'leading\\n' > ./-rm-leading
    "$BIN" rm -- ./-rm-leading
    "$BIN" rm ls-dir/visible ls-dir/.hidden ls-file-one ls-file-two ./-ls-leading
    "$BIN" rmdir ls-dir
    "$BIN" rmdir -p mkdir-tree/one/two
    [[ ! -e mkdir-tree ]]

    "$BIN" mkdir -p rmdir-chain/one/two
    "$BIN" rmdir -p rmdir-chain/one/two
    [[ ! -e rmdir-chain ]]
    "$BIN" mkdir -p rmdir-dot/a/b
    "$BIN" rmdir -p ./rmdir-dot/a/b
    [[ ! -e rmdir-dot ]]
    "$BIN" mkdir -p rmdir-verbose/a/b
    "$BIN" rmdir -pv rmdir-verbose/a/b > rmdir-verbose.txt
    grep -q "removed 'rmdir-verbose/a/b'" rmdir-verbose.txt
    grep -q "removed 'rmdir-verbose/a'" rmdir-verbose.txt
    grep -q "removed 'rmdir-verbose'" rmdir-verbose.txt
    "$BIN" rmdir --help > rmdir-help.txt
    grep -q -- '--parents' rmdir-help.txt
    "$BIN" rmdir --version > rmdir-version.txt
    grep -q 'Flatcore rmdir' rmdir-version.txt
    if "$BIN" rmdir --not-a-real-option >/dev/null 2>&1; then
        printf 'rmdir accepted an unknown option\n' >&2
        exit 1
    fi
    "$BIN" mkdir rmdir-nonempty
    printf 'keep\n' > rmdir-nonempty/file
    if "$BIN" rmdir rmdir-nonempty; then
        printf 'rmdir removed a non-empty directory\n' >&2
        exit 1
    fi
    [[ -d rmdir-nonempty ]]
    "$BIN" rm rmdir-nonempty/file
    "$BIN" rmdir rmdir-nonempty
    "$BIN" rmdir mkdir-verbose/a/b
    "$BIN" rmdir mkdir-verbose/a
    "$BIN" rmdir mkdir-verbose
    "$BIN" rmdir mkdir-mode
    "$BIN" rmdir mkdir-mode-long
    "$BIN" rmdir ./-mkdir-leading
    "$BIN" mkdir sample-dir
    "$BIN" stat sample-dir > stat-directory-output.txt
    grep -q '^  Type: directory$' stat-directory-output.txt
    "$BIN" rmdir sample-dir

    "$BIN" ln -s input.txt input-link
    [[ "$("$BIN" readlink input-link)" == 'input.txt' ]]
    "$BIN" stat input-link > stat-link-output.txt
    grep -q '^  Type: symbolic link$' stat-link-output.txt
    "$BIN" ln input.txt hard-link
    cmp input.txt hard-link

    printf 'stat-data\n' > stat-file.txt
    "$BIN" stat stat-file.txt > stat-output.txt
    grep -q '^  Size: 10 bytes$' stat-output.txt
    grep -q '^  Type: regular file$' stat-output.txt
    grep -q '^  Hidden: no$' stat-output.txt
    grep -q '^  Read-only: no$' stat-output.txt
    printf 'hidden\n' > .stat-hidden
    "$BIN" stat .stat-hidden > stat-hidden-output.txt
    grep -q '^  Hidden: yes$' stat-hidden-output.txt

    # Hidden status follows the basename, not an earlier path component.
    "$BIN" mkdir stat-hidden-dir
    printf 'nested hidden\n' > stat-hidden-dir/.nested-hidden
    printf 'nested visible\n' > stat-hidden-dir/visible
    "$BIN" stat stat-hidden-dir/.nested-hidden > stat-nested-hidden-output.txt
    grep -q '^  Hidden: yes$' stat-nested-hidden-output.txt
    "$BIN" stat stat-hidden-dir/visible > stat-nested-visible-output.txt
    grep -q '^  Hidden: no$' stat-nested-visible-output.txt
    grep -q '^  Native hidden: no$' stat-nested-visible-output.txt
    "$BIN" mkdir .stat-hidden-dir
    "$BIN" stat .stat-hidden-dir/ > stat-hidden-directory-output.txt
    grep -q '^  Hidden: yes$' stat-hidden-directory-output.txt

    # Read-only follows the native permission representation.
    chmod 444 stat-file.txt
    "$BIN" stat stat-file.txt > stat-readonly-output.txt
    grep -q '^  Read-only: yes$' stat-readonly-output.txt
    chmod 644 stat-file.txt

    if "$BIN" stat missing-stat-file >/dev/null 2>&1; then
        printf 'stat succeeded for a missing path\n' >&2
        exit 1
    fi
    if "$BIN" stat >/dev/null 2>&1; then
        printf 'stat accepted a missing operand\n' >&2
        exit 1
    fi

    # touch: create, multiple operands, clustered flags, no-create, and options.
    "$BIN" touch touch-created.txt
    [[ -f touch-created.txt ]]
    "$BIN" touch touch-one.txt touch-two.txt
    [[ -f touch-one.txt && -f touch-two.txt ]]
    "$BIN" touch -a touch-created.txt
    "$BIN" touch -m touch-created.txt
    "$BIN" touch -am touch-created.txt
    printf 'reference\n' > touch-reference.txt
    printf 'target\n' > touch-reference-target.txt
    "$BIN" touch -r touch-reference.txt touch-reference-target.txt
    [[ "$(stat -c '%X %Y' touch-reference.txt)" == "$(stat -c '%X %Y' touch-reference-target.txt)" ]]
    "$BIN" touch -c missing-touch-file
    [[ ! -e missing-touch-file ]]
    "$BIN" touch --no-create missing-touch-file-long
    [[ ! -e missing-touch-file-long ]]
    printf "leading\\n" > ./-touch-leading
    "$BIN" touch -- ./-touch-leading
    "$BIN" touch --help > touch-help.txt
    grep -q -- "--no-create" touch-help.txt
    "$BIN" touch --version > touch-version.txt
    grep -q "Flatcore touch" touch-version.txt
    if "$BIN" touch --not-a-real-option >/dev/null 2>&1; then
        printf "touch accepted an unknown option\\n" >&2
        exit 1
    fi
    "$BIN" mkdir touch-error-parent
    if "$BIN" touch touch-error-parent/missing/child.txt touch-after-error.txt >/dev/null 2>&1; then
        printf "touch succeeded despite an operand error\\n" >&2
        exit 1
    fi
    [[ -f touch-after-error.txt ]]
    "$BIN" rm touch-after-error.txt
    "$BIN" rmdir touch-error-parent
    if "$BIN" touch >/dev/null 2>&1; then
        printf "touch accepted a missing operand\\n" >&2
        exit 1
    fi

    "$BIN" touch mode.txt
    "$BIN" chmod 600 mode.txt
    [[ "$(stat -c '%a' mode.txt)" == '600' ]]
    "$BIN" sync
    id_output="$("$BIN" id)"
    [[ "$id_output" == uid=* ]]
    [[ "$id_output" == *' gid='* ]]
    [[ -n "$("$BIN" hostname)" ]]
    [[ "$("$BIN" uptime)" == *' seconds' ]]
    "$BIN" sleep 0

    printf 'a b\nc\n' > wc.txt
    [[ "$("$BIN" wc wc.txt)" == '2 3 6' ]]

    for n in {1..12}; do printf 'line%s\n' "$n"; done > all-lines.txt
    for n in {1..10}; do printf 'line%s\n' "$n"; done > expected-head.txt
    "$BIN" head all-lines.txt > actual-head.txt
    cmp expected-head.txt actual-head.txt

    [[ "$("$BIN" echo symlink-dispatch)" == 'symlink-dispatch' ]]
    [[ "$("$BIN_LINKS/echo" symlink-dispatch)" == 'symlink-dispatch' ]]

    "$BIN" rm sample.txt input.txt moved.txt input-link hard-link mode.txt wc.txt stat-file.txt .stat-hidden touch-created.txt touch-one.txt touch-two.txt touch-reference.txt touch-reference-target.txt ./-touch-leading
    "$BIN" rm stat-hidden-dir/.nested-hidden stat-hidden-dir/visible
    "$BIN" rmdir stat-hidden-dir
    "$BIN" rmdir .stat-hidden-dir
    if "$BIN" rm missing-file; then
        printf 'rm succeeded for a missing file\n' >&2
        exit 1
    fi
    if "$BIN" cat missing-file >/dev/null 2>&1; then
        printf 'cat succeeded for a missing file\n' >&2
        exit 1
    fi
)

sleep 30 &
child_pid=$!
"$BIN" kill "$child_pid"
wait "$child_pid" 2>/dev/null || true
printf 'Flatcore Linux smoke tests passed.\n'
