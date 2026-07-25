"""
Cross-language benchmark policy, execution, and reporting.

Wall time is primary for CPU/mem benchmarks. Total CPU exposes compute and
power cost hidden by wall time. Peak mem is useful evidence, but not a claim
about live or strictly required memory, especially for GC runtimes. TCP is
the exception: total server CPU is primary, while user and kernel CPU are
shown separately to expose scheduling and I/O machinery.

The runner collects per-process user CPU, kernel CPU, and peak mem with `wait4`.
Subprocesses inherit stdio; successful benchmarks stay silent, validate their
own result, and report failures through stderr plus exit status. Benchmarks in
one family / section must perform identical work and produce identical results.

Timing policy is deliberately fixed and boring:

* Run each benchmark five times. No adaptive stopping or partial runs.
* Keep a benchmark's runs contiguous. Interleaving languages made CPU and
  memory results less repeatable by perturbing macOS CPU power/scheduler state.
* Run one unmeasured validation immediately before that benchmark's measured
  group. Besides checking correctness, this avoids measuring an idle/cold CPU
  state. Moving all validations before all measurements loses that benefit.
* Each individual subprocess gets a hard one-minute timeout. Let a run finish
  normally before applying any suite-level policy.
* Complete all compilation/setup before timing.

The TCP workload intentionally keeps 4096 connections alive for repeated
one-byte exchanges. This amortizes server bootstrap while retaining the
thread/task/event-loop scheduling cost under high concurrency.
"""

# BOT-GENERATED

import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from . import catalog


RUNS = 5


@dataclass(frozen=True)
class Metric:
    field: str
    label: str
    kind: str


WALL = Metric("wall_seconds", "Wall", "duration")
CPU = Metric("cpu_seconds", "CPU", "duration")
USER_CPU = Metric("user_cpu_seconds", "User CPU", "duration")
KERNEL_CPU = Metric("kernel_cpu_seconds", "Kernel CPU", "duration")
MEM = Metric("peak_rss_bytes", "Peak mem", "memory")


def grouped(items, measure) -> list[catalog.Result]:
    # Keep each language's five runs adjacent. Experiments showed that
    # round-robin interleaving increased CPU-bound variance on macOS.
    return [
        catalog.Result(item, [measure(item) for _ in range(RUNS)])
        for item in items
    ]


def interleaved(items, measure) -> list[catalog.Result]:
    # Retained as a section-level experiment hook, mainly for unusual I/O
    # workloads. It is intentionally not the default timing policy.
    samples = {item: [] for item in items}
    for _ in range(RUNS):
        for item in items:
            samples[item].append(measure(item))
    return [catalog.Result(item, samples[item]) for item in items]


@dataclass(frozen=True)
class SectionPlan:
    title: str
    note: str | None = None
    metrics: tuple[Metric, ...] = (WALL, CPU, MEM)
    primary: Metric = WALL
    runner: Callable = grouped
    measure: Callable | None = None


def collect(
    plan: SectionPlan,
    items,
    measure,
    progress=None,
    validate=None,
) -> list[catalog.Result]:
    counts = {item: 0 for item in items}
    measure_one = plan.measure or measure

    def measured(item):
        # Validation is a complete unmeasured run. Keep it immediately beside
        # the first measured run: a separate validation pass let CPU power
        # state cool before later groups and measurably increased variance.
        if counts[item] == 0 and validate is not None:
            validate(item)
        counts[item] += 1
        if progress is not None:
            progress(f"[{item.name}] run {counts[item]}/{RUNS}")
        return measure_one(item)

    return plan.runner(items, measured)


def metric_scale(
    metric: Metric,
    results: list[catalog.Result],
) -> tuple[str, float]:
    means = [
        catalog.summarize(result.samples, metric.field)[0]
        for result in results
    ]
    # One unit per column. Per-row units made equal spreads look unrelated
    # (for example, milliseconds beside seconds in the same benchmark family).
    if metric.kind == "duration":
        return catalog.duration_unit(min(means))
    if metric.kind == "memory":
        return "MiB", 1 / 1024**2
    raise ValueError(f"unknown metric kind: {metric.kind!r}")


def render_section(
    plan: SectionPlan,
    results: list[catalog.Result],
) -> str:
    results = sorted(
        results,
        key=lambda result: catalog.summarize(
            result.samples, plan.primary.field
        )[0],
    )
    scales = {
        metric: metric_scale(metric, results)
        for metric in plan.metrics
    }
    baseline = catalog.summarize(
        results[0].samples, plan.primary.field
    )[0]

    headers = [
        metric.label
        + f" [{scales[metric][0]}]"
        + (" ↓" if metric == plan.primary else "")
        for metric in plan.metrics
    ]
    relative_header = (
        "Relative"
        if plan.primary in plan.metrics
        else f"{plan.primary.label} relative ↓"
    )
    lines = [f"\n## {plan.title}\n"]
    if plan.note is not None:
        lines.append(plan.note + "\n")
    lines.extend((
        "| Command | "
        + " | ".join(headers)
        + f" | {relative_header} |",
        "| --- | " + " | ".join("---:" for _ in plan.metrics) + " | ---: |",
    ))

    for result in results:
        cells = []
        for metric in plan.metrics:
            mean, deviation = catalog.summarize(
                result.samples, metric.field
            )
            unit, scale = scales[metric]
            cells.append(
                catalog.format_table_measurement(
                    mean,
                    deviation,
                    unit,
                    scale,
                )
            )
        primary_mean = catalog.summarize(
            result.samples, plan.primary.field
        )[0]
        lines.append(
            f"| `{result.item.name}` | "
            + " | ".join(cells)
            + f" | {primary_mean / baseline:.2f} |"
        )

    return "\n".join(lines) + "\n"


TCP_TITLE = "TCP CONNECTIONS"
TCP_SECTION = next(
    section
    for section in catalog.SECTIONS
    if section.title == TCP_TITLE
)
TCP_NOTE = (
    catalog.TCP_NOTE
    + f"\n\nEach implementation is measured {RUNS} times."
)
SECTION_PLANS = {
    TCP_TITLE: SectionPlan(
        TCP_TITLE,
        note=TCP_NOTE,
        # Total CPU remains the sort/relative metric, but omitting its
        # redundant column keeps the wide TCP table readable.
        metrics=(WALL, USER_CPU, KERNEL_CPU, MEM),
        primary=CPU,
        runner=grouped,
    ),
}


def plan_for(section: catalog.Section) -> SectionPlan:
    if section.title in SECTION_PLANS:
        return SECTION_PLANS[section.title]
    primary = CPU if section.sort_by == "cpu_seconds" else WALL
    return SectionPlan(
        section.title,
        note=section.note,
        metrics=(WALL, CPU, MEM),
        primary=primary,
        runner=grouped,
    )


def measure(item: catalog.Bench) -> catalog.Sample:
    try:
        return catalog.measure(item, catalog.RUN_TIMEOUT_SECONDS)
    except catalog.MeasurementDeadline:
        raise RuntimeError(
            f"benchmark {item.name!r} run exceeded "
            f"{catalog.RUN_TIMEOUT_SECONDS:g} seconds: {item.cmd!r}"
        ) from None


def run_section(out, section, items) -> None:
    if not items:
        return
    plan = plan_for(section)
    results = collect(
        plan,
        items,
        measure,
        catalog.progress,
        lambda item: catalog.validate([item]),
    )
    for result in results:
        print(catalog.format_done(result), file=sys.stderr, flush=True)
    out.write(render_section(plan, results))
    out.flush()


def main_for(argv: list[str]) -> int:
    args = catalog.parse_args(argv)
    items = catalog.selected_benches(args.filters)
    if not items:
        print("no benchmarks match filters:", *args.filters, file=sys.stderr)
        return 2

    os.chdir(catalog.ROOT)
    if catalog.uses_astil(items):
        catalog.progress("setup " + " ".join(catalog.CLEAN))
        catalog.run(catalog.CLEAN)

    output = Path(args.output)
    if not output.is_absolute():
        output = catalog.ROOT / output

    # Build every selected implementation before any validation or timing.
    # Compilation therefore cannot perturb one language's measured group.
    for cmd in catalog.unique_setup(items):
        catalog.progress("setup " + " ".join(cmd))
        catalog.run(cmd)

    if args.smoke:
        # Smoke mode has no measured groups to trigger their adjacent
        # validations, so perform the validation-only pass explicitly.
        catalog.validate(items)
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8") as out:
        catalog.write_versions(out, items)
        for section in catalog.SECTIONS:
            run_section(
                out,
                section,
                [item for item in items if item.section == section.title],
            )

    shown = (
        output.relative_to(catalog.ROOT)
        if output.is_relative_to(catalog.ROOT)
        else output
    )
    catalog.progress(f"wrote {shown}")
    return 0


def main() -> int:
    return main_for(sys.argv[1:])


if __name__ == "__main__":
    raise SystemExit(main())
