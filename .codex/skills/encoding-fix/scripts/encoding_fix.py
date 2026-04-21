#!/usr/bin/env python3
"""Scan and optionally fix encoding/mojibake issues in repository text files."""

from __future__ import annotations

import argparse
import codecs
import json
import os
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path

REPLACEMENT_CHAR = "\ufffd"
DEFAULT_MAX_FILE_SIZE_MB = 5

MOJIBAKE_MARKERS = (
    "Ã§",
    "Ã£",
    "Ã¡",
    "Ã¢",
    "Ãª",
    "Ã©",
    "Ã­",
    "Ã³",
    "Ãµ",
    "Ãº",
    "Ã‰",
    "Ã‡",
    "Â",
    "â€“",
    "â€”",
    "â€œ",
    "â€",
    "â€™",
    "â€˜",
    "â€¢",
    "â€¦",
    "ï»¿",
    REPLACEMENT_CHAR,
)

IGNORED_DIRS = {
    ".git",
    ".hg",
    ".svn",
    ".idea",
    ".vscode",
    ".venv",
    "venv",
    "env",
    "node_modules",
    "dist",
    "build",
    "out",
    "target",
    "__pycache__",
    ".mypy_cache",
    ".pytest_cache",
    ".next",
    ".nuxt",
    ".cache",
    "coverage",
    "vendor",
    "vendor_imports",
}

RISKY_DIR_HINTS = {"fixtures", "__fixtures__", "snapshot", "snapshots", "testdata", "golden"}

BINARY_EXTENSIONS = {
    ".7z",
    ".avi",
    ".bmp",
    ".class",
    ".dll",
    ".doc",
    ".docm",
    ".docx",
    ".dylib",
    ".eot",
    ".exe",
    ".gif",
    ".gz",
    ".ico",
    ".jar",
    ".jpeg",
    ".jpg",
    ".lockb",
    ".mov",
    ".mp3",
    ".mp4",
    ".otf",
    ".pdf",
    ".png",
    ".pyc",
    ".pyd",
    ".pyo",
    ".so",
    ".tar",
    ".ttf",
    ".wav",
    ".webm",
    ".woff",
    ".woff2",
    ".xls",
    ".xlsm",
    ".xlsx",
    ".zip",
}


@dataclass
class ChangedFile:
    path: str
    detected_encoding: str
    issues: list[str]
    applied: bool


@dataclass
class SkippedFile:
    path: str
    reason: str


@dataclass
class NormalizeResult:
    text: str | None
    detected_encoding: str | None
    issues: list[str]
    risk: str | None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Find and fix encoding/mojibake issues in text files."
    )
    parser.add_argument("--root", default=".", help="Repository root to scan")
    parser.add_argument(
        "--write",
        action="store_true",
        help="Apply changes in-place. Without this flag, run in dry-run mode.",
    )
    parser.add_argument(
        "--report",
        help="Optional markdown report path (example: encoding_reports/2026-03-04_10-30_encoding-fix.md)",
    )
    parser.add_argument("--json", action="store_true", help="Print JSON summary to stdout")
    parser.add_argument(
        "--max-file-size-mb",
        type=int,
        default=DEFAULT_MAX_FILE_SIZE_MB,
        help=f"Skip files larger than this size in MB (default: {DEFAULT_MAX_FILE_SIZE_MB})",
    )
    parser.add_argument(
        "--ignore-dir",
        action="append",
        default=[],
        help="Additional directory name to ignore (repeatable)",
    )
    return parser.parse_args()


def looks_binary(raw: bytes) -> bool:
    if not raw:
        return False
    sample = raw[:4096]
    if b"\x00" in sample:
        return True
    control_count = 0
    for byte in sample:
        if byte < 32 and byte not in (9, 10, 13):
            control_count += 1
    return control_count > max(8, len(sample) // 10)


def count_mojibake_markers(text: str) -> int:
    return sum(text.count(marker) for marker in MOJIBAKE_MARKERS)


def text_quality_score(text: str) -> int:
    marker_score = count_mojibake_markers(text) * 10
    replacement_score = text.count(REPLACEMENT_CHAR) * 20
    control_score = sum(1 for ch in text if ord(ch) < 32 and ch not in "\n\r\t") * 5
    return marker_score + replacement_score + control_score


def decode_non_utf8(raw: bytes) -> tuple[str, str] | None:
    candidates: list[tuple[int, str, str]] = []
    for enc in ("cp1252", "latin-1"):
        try:
            text = raw.decode(enc)
        except UnicodeDecodeError:
            continue
        candidates.append((text_quality_score(text), enc, text))
    if not candidates:
        return None
    candidates.sort(key=lambda item: (item[0], 0 if item[1] == "cp1252" else 1))
    _, enc, text = candidates[0]
    return text, enc


def try_repair_mojibake(text: str) -> tuple[str, bool]:
    base_score = text_quality_score(text)
    if base_score == 0:
        return text, False

    candidates: list[tuple[int, str]] = []
    for enc in ("latin-1", "cp1252"):
        try:
            repaired = text.encode(enc).decode("utf-8")
        except (UnicodeEncodeError, UnicodeDecodeError):
            continue
        candidates.append((text_quality_score(repaired), repaired))

    if not candidates:
        return text, False

    candidates.sort(key=lambda item: item[0])
    best_score, best_text = candidates[0]
    if best_text != text and best_score + 2 <= base_score:
        return best_text, True
    return text, False


def normalize_to_utf8(raw: bytes) -> NormalizeResult:
    issues: list[str] = []
    detected_encoding = "utf-8"

    has_bom = raw.startswith(codecs.BOM_UTF8)
    if has_bom:
        issues.append("BOM")

    try:
        text = raw.decode("utf-8-sig" if has_bom else "utf-8")
    except UnicodeDecodeError:
        issues.append("invalid-utf8-bytes")
        decoded = decode_non_utf8(raw)
        if decoded is None:
            return NormalizeResult(
                text=None,
                detected_encoding=None,
                issues=sorted(set(issues)),
                risk="unable-to-decode-safely",
            )
        text, detected_encoding = decoded
        if detected_encoding == "cp1252":
            issues.append("cp1252")
        else:
            issues.append("latin1")

    repaired_text, repaired = try_repair_mojibake(text)
    if repaired:
        text = repaired_text
        issues.append("mojibake")

    return NormalizeResult(
        text=text,
        detected_encoding=detected_encoding,
        issues=sorted(set(issues)),
        risk=None,
    )


def is_risky_path(relative_path: str) -> bool:
    parts = {part.lower() for part in Path(relative_path).parts}
    return any(hint in parts for hint in RISKY_DIR_HINTS)


def should_ignore_path(path: Path, ignored_dir_names: set[str]) -> bool:
    parts = [part.lower() for part in path.parts]
    return any(part in ignored_dir_names for part in parts)


def iter_candidate_files(root: Path, ignored_dir_names: set[str]):
    for current_root, dirnames, filenames in os.walk(root):
        filtered_dirs = []
        for dirname in dirnames:
            if dirname.lower() in ignored_dir_names:
                continue
            filtered_dirs.append(dirname)
        dirnames[:] = filtered_dirs

        current_root_path = Path(current_root)
        for filename in filenames:
            full_path = current_root_path / filename
            if full_path.suffix.lower() in BINARY_EXTENSIONS:
                continue
            yield full_path


def render_report(summary: dict) -> str:
    changed = summary["changed_files"]
    skipped = summary["skipped_files"]
    ignored = ", ".join(summary["ignored_dirs"])
    mode = "write" if summary["write_mode"] else "dry-run"

    lines: list[str] = [
        "# Encoding Fix Report",
        "",
        f"- Generated at: {summary['generated_at']}",
        f"- Mode: {mode}",
        "",
        "## Escopo",
        f"- Root: `{summary['root']}`",
        f"- Pastas ignoradas: `{ignored}`",
        f"- Arquivos varridos: {summary['scanned_files']}",
        "",
        "## Lista de arquivos corrigidos (encoding detectado -> UTF-8)",
    ]

    if changed:
        for item in changed:
            applied_label = "aplicado" if item["applied"] else "sugerido (dry-run)"
            lines.append(
                f"- `{item['path']}`: `{item['detected_encoding']}` -> `utf-8` ({applied_label})"
            )
    else:
        lines.append("- Nenhum arquivo corrigido.")

    lines.extend(
        [
            "",
            "## Tipo de problema por arquivo",
        ]
    )
    if changed:
        for item in changed:
            issues = ", ".join(item["issues"]) if item["issues"] else "nao identificado"
            lines.append(f"- `{item['path']}`: {issues}")
    else:
        lines.append("- Nenhum problema corrigido.")

    lines.extend(
        [
            "",
            "## Observacoes de risco",
        ]
    )
    if skipped:
        for item in skipped:
            lines.append(f"- `{item['path']}`: {item['reason']}")
    else:
        lines.append("- Nenhum arquivo pulado por risco.")

    lines.extend(
        [
            "",
            "## Comandos executados (lint/test/build) e resultado",
            "- Este script nao executa validacoes do repositorio automaticamente.",
            "- Preencher no fluxo da skill com os comandos reais executados e seus resultados.",
            "",
            "## Lista de arquivos alterados",
        ]
    )
    if changed and summary["write_mode"]:
        for item in changed:
            if item["applied"]:
                lines.append(f"- `{item['path']}`")
    else:
        lines.append("- Nenhum arquivo alterado em disco.")

    return "\n".join(lines) + "\n"


def run(args: argparse.Namespace) -> dict:
    root = Path(args.root).resolve()
    if not root.exists():
        raise FileNotFoundError(f"Root path does not exist: {root}")
    if not root.is_dir():
        raise NotADirectoryError(f"Root path is not a directory: {root}")

    ignored_dir_names = {name.lower() for name in IGNORED_DIRS}
    ignored_dir_names.update(name.lower() for name in args.ignore_dir if name)

    max_size_bytes = max(1, args.max_file_size_mb) * 1024 * 1024
    changed_files: list[ChangedFile] = []
    skipped_files: list[SkippedFile] = []
    scanned_files = 0
    unchanged_files = 0

    for file_path in iter_candidate_files(root, ignored_dir_names):
        relative_path = file_path.relative_to(root).as_posix()
        if should_ignore_path(file_path.relative_to(root), ignored_dir_names):
            continue

        try:
            file_size = file_path.stat().st_size
        except OSError:
            skipped_files.append(SkippedFile(path=relative_path, reason="stat-failed"))
            continue

        if file_size > max_size_bytes:
            skipped_files.append(
                SkippedFile(
                    path=relative_path,
                    reason=f"file-too-large(>{args.max_file_size_mb}MB)",
                )
            )
            continue

        try:
            raw = file_path.read_bytes()
        except OSError:
            skipped_files.append(SkippedFile(path=relative_path, reason="read-failed"))
            continue

        if looks_binary(raw):
            continue

        scanned_files += 1
        normalize_result = normalize_to_utf8(raw)
        if normalize_result.text is None:
            skipped_files.append(
                SkippedFile(
                    path=relative_path,
                    reason=normalize_result.risk or "decode-failed",
                )
            )
            continue

        normalized_bytes = normalize_result.text.encode("utf-8")
        if normalized_bytes == raw:
            unchanged_files += 1
            continue

        if is_risky_path(relative_path):
            skipped_files.append(
                SkippedFile(
                    path=relative_path,
                    reason="possible-byte-exact-fixture",
                )
            )
            continue

        applied = False
        if args.write:
            try:
                file_path.write_bytes(normalized_bytes)
                applied = True
            except OSError:
                skipped_files.append(SkippedFile(path=relative_path, reason="write-failed"))
                continue

        changed_files.append(
            ChangedFile(
                path=relative_path,
                detected_encoding=normalize_result.detected_encoding or "unknown",
                issues=normalize_result.issues,
                applied=applied,
            )
        )

    summary = {
        "generated_at": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "root": str(root),
        "write_mode": bool(args.write),
        "ignored_dirs": sorted(ignored_dir_names),
        "scanned_files": scanned_files,
        "unchanged_files": unchanged_files,
        "changed_count": len(changed_files),
        "changed_files": [asdict(item) for item in changed_files],
        "skipped_count": len(skipped_files),
        "skipped_files": [asdict(item) for item in skipped_files],
    }

    if args.report:
        report_path = Path(args.report)
        if not report_path.is_absolute():
            report_path = root / report_path
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(render_report(summary), encoding="utf-8")
        summary["report_path"] = str(report_path)

    return summary


def main() -> int:
    args = parse_args()
    summary = run(args)

    if args.json:
        print(json.dumps(summary, ensure_ascii=False, indent=2))
    else:
        print(
            "Scanned: {scanned} | Changed: {changed} | Skipped: {skipped} | Unchanged: {unchanged}".format(
                scanned=summary["scanned_files"],
                changed=summary["changed_count"],
                skipped=summary["skipped_count"],
                unchanged=summary["unchanged_files"],
            )
        )
        if "report_path" in summary:
            print(f"Report: {summary['report_path']}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
