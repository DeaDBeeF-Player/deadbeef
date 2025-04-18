#!/bin/bash

# Usage: ./glibc-check <binary> <max_glibc_version>
# Example: ./glibc-check libfoo.so 2.17

if [ $# -ne 2 ]; then
    echo "Usage: $0 <binary> <max_glibc_version>"
    exit 1
fi

file="$1"
max_version="$2"
bad_versions=()

# Extract GLIBC versions used in the binary
used_versions=$(readelf -W --dyn-syms "$file" 2>/dev/null |
    grep -o 'GLIBC_[0-9]\+\.[0-9]\+' |
    sort -uV)

# Compare each version to the max allowed
for version in $used_versions; do
    numeric=${version#GLIBC_}
    if [ "$(printf "%s\n%s\n" "$max_version" "$numeric" | sort -V | tail -n1)" != "$max_version" ]; then
        bad_versions+=("$version")
    fi
done

if [ ${#bad_versions[@]} -eq 0 ]; then
    echo -e "✅ OK: $file uses up to GLIBC_$max_version"
    exit 0
else
    echo -e "❌ FAIL: $file uses GLIBC versions newer than $max_version:"
    printf '   - %s\n' "${bad_versions[@]}"
    exit 1
fi
