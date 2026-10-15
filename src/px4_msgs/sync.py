#!/usr/bin/env python3

import argparse
import os
import re
import sys
from pathlib import Path

import yaml


PRIMITIVES = {
    "bool", "byte", "char", "float32", "float64", "int8", "uint8",
    "int16", "uint16", "int32", "uint32", "int64", "uint64", "string",
    "wstring", "time", "duration",
}
MESSAGE_TYPE = re.compile(r"(?:px4_msgs|px4)::msg::([A-Za-z][A-Za-z0-9_]*)")
MESSAGE_FILE = re.compile(r"[A-Za-z][A-Za-z0-9_]*\.msg")


def message_types(topics_file):
    with topics_file.open() as stream:
        topics = yaml.safe_load(stream)
    if not isinstance(topics, dict):
        raise ValueError(f"Expected a YAML mapping in {topics_file}")
    messages = set()
    for section in ("publications", "subscriptions", "subscriptions_multi"):
        for entry in topics.get(section) or []:
            match = MESSAGE_TYPE.fullmatch(entry["type"])
            if match:
                messages.add(match.group(1))
    if not messages:
        raise ValueError(f"No PX4 message types found in {topics_file}")
    return messages


def sync(source_dir, output_dir, topics_file):
    pending = sorted(message_types(topics_file))
    messages = {}
    while pending:
        name = pending.pop()
        filename = f"{name}.msg"
        if filename in messages:
            continue
        source = source_dir / filename
        if not source.is_file():
            source = source_dir / "versioned" / filename
        if not source.is_file():
            candidates = sorted(source_dir.rglob(filename))
            if len(candidates) != 1:
                raise ValueError(f"PX4 message missing or ambiguous: {name}")
            source = candidates[0]
        messages[filename] = source.read_bytes()
        for line in messages[filename].decode().splitlines():
            fields = line.split("#", 1)[0].split()
            if not fields:
                continue
            field_type = fields[0].split("[", 1)[0]
            if field_type not in PRIMITIVES:
                if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", field_type):
                    raise ValueError(f"Unsupported message dependency: {field_type}")
                pending.append(field_type)

    manifest = output_dir / ".px4_msgs_generated"
    previous = set(manifest.read_text().splitlines()) if manifest.is_file() else set()
    if any(not MESSAGE_FILE.fullmatch(filename) for filename in previous):
        raise ValueError(f"Invalid generated-message manifest: {manifest}")
    for filename in messages:
        if (output_dir / filename).exists() and filename not in previous:
            raise ValueError(f"Refusing to overwrite non-generated message: {filename}")

    output_dir.mkdir(parents=True, exist_ok=True)
    for filename, contents in sorted(messages.items()):
        destination = output_dir / filename
        if not destination.is_file() or destination.read_bytes() != contents:
            destination.write_bytes(contents)
    for filename in previous - messages.keys():
        (output_dir / filename).unlink(missing_ok=True)
    contents = "".join(f"{filename}\n" for filename in sorted(messages))
    if not manifest.is_file() or manifest.read_text() != contents:
        manifest.write_text(contents)
    print(f"Synced {len(messages)} PX4 messages from {topics_file}")


def main():
    package_dir = Path(__file__).resolve().parent
    px4_dir = package_dir.parent / "px4" / "PX4-Autopilot"
    parser = argparse.ArgumentParser(description="Sync PX4 DDS messages and their dependencies")
    parser.add_argument("--topics-file", default=os.environ.get("DDS_TOPICS_FILE") or "dds_topics.yaml")
    parser.add_argument("--source-dir", type=Path, default=px4_dir / "msg")
    parser.add_argument("--output-dir", type=Path, default=package_dir / "msg")
    args = parser.parse_args()
    topics_file = Path(args.topics_file)
    if not topics_file.is_absolute():
        topics_file = px4_dir / "src" / "modules" / "zenoh" / topics_file
    try:
        sync(args.source_dir, args.output_dir, topics_file)
    except (OSError, ValueError, KeyError, TypeError, yaml.YAMLError) as error:
        print(f"PX4 message sync failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
