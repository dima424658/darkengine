#!/usr/bin/env python3
"""Inventory installed DML command families without executing patches."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re


def commands(text):
    if not text.startswith("DML1") or (len(text) > 4 and not text[4].isspace()):
        raise ValueError("missing DML1 header")
    depth = 0
    block_comment = False
    quoted = False
    command = []
    result = Counter()
    line = 1
    index = 4
    while index < len(text):
        char = text[index]
        pair = text[index:index + 2]
        if block_comment:
            if pair == "*/":
                block_comment = False
                index += 2
                continue
        elif quoted:
            if char == '"':
                quoted = False
            elif char == '\n':
                raise ValueError(f"unterminated quote at line {line}")
        elif pair == "/*":
            block_comment = True
            index += 2
            continue
        elif pair == "//":
            end = text.find('\n', index)
            index = len(text) if end < 0 else end
            continue
        elif char == '"' and (index == 0 or text[index - 1].isspace() or text[index - 1] == '='):
            quoted = True
        elif char == '{':
            if depth == 0 and command:
                result[command[0].lower()] += 1
                command = []
            depth += 1
        elif char == '}':
            depth -= 1
            if depth < 0:
                raise ValueError(f"unmatched closing brace at line {line}")
        elif depth == 0 and char not in '\r\n':
            match = re.match(r"[!+#A-Za-z_-][!+#A-Za-z_0-9-]*", text[index:])
            if match:
                command.append(match[0])
                index += len(match[0])
                continue
        if char == '\n':
            if depth == 0 and command:
                result[command[0].lower()] += 1
                command = []
            line += 1
        index += 1
    if block_comment or quoted or depth:
        raise ValueError("unterminated comment, quote or block")
    if command:
        result[command[0].lower()] += 1
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game", type=Path)
    args = parser.parse_args()
    total = Counter()
    errors = {}
    files = []
    for path in sorted(args.game.rglob("*")):
        if not path.is_file() or path.suffix.lower() != ".dml" or "DOC" in path.parts:
            continue
        relative = str(path.relative_to(args.game))
        try:
            counts = commands(path.read_text(encoding="cp1252"))
        except (ValueError, UnicodeError) as error:
            errors[relative] = str(error)
            continue
        total.update(counts)
        files.append({"path": relative, "commands": dict(sorted(counts.items()))})
    print(json.dumps({"files": files, "command_counts": dict(sorted(total.items())),
                      "errors": errors}, indent=2))
    raise SystemExit(bool(errors))


if __name__ == "__main__":
    main()
