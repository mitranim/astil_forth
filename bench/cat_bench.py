"""Fixture generation and output validation for cat benchmarks."""

# BOT-GENERATED

import hashlib
import subprocess
from collections.abc import Callable, Iterable
from dataclasses import dataclass
from functools import cache
from pathlib import Path
from typing import ContextManager, Literal, Protocol


ROOT = Path(__file__).resolve().parent.parent
TMP = ROOT / ".tmp" / "cat"
FILE = "file"
STDIN = "stdin"
Source = Literal["file", "stdin"]
PATTERNS = {
    FILE: bytes(range(256)) * 256,
    STDIN: bytes(reversed(range(256))) * 256,
}


@dataclass(frozen=True)
class CatIO:
    input_bytes: int
    sources: tuple[Source, ...]


@dataclass(frozen=True)
class Implementation:
    name: str
    file: str
    cmd: tuple[str, ...]
    setup: tuple[tuple[str, ...], ...] = ()
    tools: tuple[str, ...] = ()


class CatItem(Protocol):
    name: str
    cmd: tuple[str, ...]
    cat: CatIO | None


def input_path(cat: CatIO, source: Source) -> Path:
    return TMP / f"{source}-{cat.input_bytes}.bin"


def chunks(size: int, source: Source):
    pattern = PATTERNS[source]
    remaining = size
    while remaining:
        chunk = pattern[: min(remaining, len(pattern))]
        yield chunk
        remaining -= len(chunk)


def prepare_inputs(items: Iterable[CatItem]) -> None:
    inputs = sorted({
        (item.cat.input_bytes, source)
        for item in items
        if item.cat is not None
        for source in item.cat.sources
    })
    if not inputs:
        return

    TMP.mkdir(parents=True, exist_ok=True)
    for size, source in inputs:
        path = input_path(CatIO(size, (source,)), source)
        temporary = path.with_suffix(".new")
        with temporary.open("wb") as output:
            for chunk in chunks(size, source):
                output.write(chunk)
        temporary.replace(path)


@cache
def expected_output(cat: CatIO) -> tuple[int, str]:
    digest = hashlib.sha256()
    for source in cat.sources:
        for chunk in chunks(cat.input_bytes, source):
            digest.update(chunk)
    return cat.input_bytes * len(cat.sources), digest.hexdigest()


def declare_section(
    add_section: Callable[..., None],
    add_benchmark: Callable[..., None],
    root: Path,
    implementations: Iterable[Implementation],
    title: str,
    suffix: str,
    cat: CatIO,
    args: tuple[str, ...],
    note: str,
) -> None:
    add_section(title, note=note)
    input_arg = str(input_path(cat, FILE).relative_to(root))
    for implementation in implementations:
        command_args = tuple(
            input_arg if arg == "{}" else arg for arg in args
        )
        add_benchmark(
            f"cat_{implementation.name}_{suffix}",
            implementation.file,
            (*implementation.cmd, *command_args),
            setup=implementation.setup,
            tools=implementation.tools,
            cat=cat,
        )


def validate(
    item: CatItem,
    timeout_seconds: float,
    alarm: Callable[[float], ContextManager[None]],
) -> None:
    assert item.cat is not None
    digest = hashlib.sha256()
    count = 0
    path = input_path(item.cat, STDIN)
    try:
        with (
            path.open("rb") as stdin,
            subprocess.Popen(
                item.cmd,
                cwd=ROOT,
                stdin=stdin,
                stdout=subprocess.PIPE,
            ) as process,
        ):
            assert process.stdout is not None
            try:
                with alarm(timeout_seconds):
                    while chunk := process.stdout.read(1024 * 1024):
                        count += len(chunk)
                        digest.update(chunk)
                    exit_code = process.wait()
            except BaseException:
                process.kill()
                process.wait()
                raise
    except OSError as err:
        raise RuntimeError(
            f"benchmark {item.name!r} validation failed to run command: "
            f"{item.cmd!r}"
        ) from err

    if exit_code:
        raise RuntimeError(
            f"benchmark {item.name!r} validation command exited with "
            f"status {exit_code}: {item.cmd!r}"
        )

    expected_count, expected_digest = expected_output(item.cat)
    actual_digest = digest.hexdigest()
    if (count, actual_digest) != (expected_count, expected_digest):
        raise RuntimeError(
            f"benchmark {item.name!r} output mismatch: "
            f"got {count} bytes sha256 {actual_digest}, expected "
            f"{expected_count} bytes sha256 {expected_digest}"
        )
