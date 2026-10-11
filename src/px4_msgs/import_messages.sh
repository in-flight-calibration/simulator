#!/usr/bin/env bash
set -euo pipefail

package_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
px4_dir="${package_dir}/../px4/PX4-Autopilot"
source_dir="${px4_dir}/msg"
output_dir="${package_dir}/msg"
topics_file="${DDS_TOPICS_FILE:-ifc_dds_topics.yaml}"
[[ "${topics_file}" = /* ]] || topics_file="${px4_dir}/src/modules/zenoh/${topics_file}"
[[ -f "${topics_file}" ]] || { printf 'DDS topics file not found: %s\n' "${topics_file}" >&2; exit 1; }

mkdir -p "${output_dir}"
manifest="${output_dir}/.px4_msgs_generated"
stage_dir="$(mktemp -d "${output_dir}/.import.XXXXXX")"
trap 'rm -rf -- "${stage_dir}"' EXIT
declare -A visited=() previous=()
declare -a pending=()

if [[ -f "${manifest}" ]]; then
    while IFS= read -r file; do [[ -z "${file}" ]] || previous["${file}"]=1; done < "${manifest}"
fi
mapfile -t pending < <(sed -nE 's/^[[:space:]]*type:[[:space:]]*(px4_msgs|px4)::msg::([[:alnum:]_]+).*$/\2/p' "${topics_file}" | sort -u)
[[ ${#pending[@]} -gt 0 ]] || { printf 'No PX4 message types found in %s\n' "${topics_file}" >&2; exit 1; }

while [[ ${#pending[@]} -gt 0 ]]; do
    name="${pending[0]}"
    pending=("${pending[@]:1}")
    [[ -n "${visited[${name}]:-}" ]] && continue
    visited["${name}"]=1

    source="${source_dir}/${name}.msg"
    [[ -f "${source}" ]] || source="${source_dir}/versioned/${name}.msg"
    [[ -f "${source}" ]] || { printf 'PX4 message not found: %s\n' "${name}" >&2; exit 1; }
    file="${name}.msg"
    if [[ -e "${output_dir}/${file}" && -z "${previous[${file}]:-}" ]]; then
        printf 'Refusing to overwrite non-generated message: %s\n' "${output_dir}/${file}" >&2
        exit 1
    fi

    cp -- "${source}" "${stage_dir}/${file}"
    printf '%s\n' "${file}" >> "${stage_dir}/manifest"
    while read -r type _; do
        type="${type%%\[*}"
        case "${type}" in
            ''|bool|byte|char|float32|float64|int8|uint8|int16|uint16|int32|uint32|int64|uint64|string|wstring|time|duration) continue ;;
        esac
        [[ -n "${visited[${type}]:-}" ]] || pending+=("${type}")
    done < <(sed 's/#.*//' "${source}")
done

sort -u "${stage_dir}/manifest" -o "${stage_dir}/manifest"
for file in "${!previous[@]}"; do
    [[ -f "${stage_dir}/${file}" ]] || rm -f -- "${output_dir}/${file}"
done
while IFS= read -r file; do cp -- "${stage_dir}/${file}" "${output_dir}/${file}"; done < "${stage_dir}/manifest"
mv -- "${stage_dir}/manifest" "${manifest}"
printf 'Imported %s PX4 messages from %s\n' "${#visited[@]}" "${topics_file}"