# BOT-GENERATED

import io
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from bench import bench as driver
from bench import catalog


def bench(name: str) -> catalog.Bench:
    return catalog.Bench("S", name, name, (name,))


def sample(wall: float, cpu: float) -> catalog.Sample:
    return catalog.Sample(wall, cpu, 0, 2 * 1024**2)


class RunPolicyTest(unittest.TestCase):
    def test_run_section_validates_cat_before_warmup(self) -> None:
        item = catalog.Bench(
            "S",
            "cat",
            "cat",
            ("cat",),
            cat=catalog.CatIO(1, (catalog.cat_bench.STDIN,)),
        )
        events = []

        def validate(items):
            self.assertEqual(items, [item])
            events.append("validate")

        def measure(_item):
            events.append("measure")
            return sample(0.1, 0.05)

        with (
            mock.patch.object(catalog, "validate", side_effect=validate),
            mock.patch.object(driver, "measure", side_effect=measure),
            mock.patch.object(catalog, "progress"),
        ):
            driver.run_section(
                io.StringIO(),
                catalog.Section("S"),
                [item],
            )

        self.assertEqual(events, ["validate"] + ["measure"] * 6)

    def test_timed_out_run_fails_with_benchmark_context(self) -> None:
        item = bench("slow")
        with (
            mock.patch.object(
                catalog,
                "measure",
                side_effect=catalog.MeasurementDeadline(),
            ),
            self.assertRaisesRegex(RuntimeError, "slow.*60"),
        ):
            driver.measure(item)

    def test_validation_and_warmup_immediately_precede_each_group(self) -> None:
        items = [bench("one"), bench("two")]
        events = []

        def validate(item):
            events.append(f"validate {item.name}")

        def progress(message):
            events.append(message)

        def measure(item):
            events.append(f"measure {item.name}")
            return sample(0.1, 0.05)

        driver.collect(
            driver.SectionPlan("S", runner=driver.grouped),
            items,
            measure,
            progress=progress,
            validate=validate,
        )

        expected = []
        for item in items:
            expected.extend((
                f"validate {item.name}",
                f"[{item.name}] warmup",
                f"measure {item.name}",
            ))
            for run in range(1, driver.RUNS + 1):
                expected.extend((
                    f"[{item.name}] run {run}/{driver.RUNS}",
                    f"measure {item.name}",
                ))
        self.assertEqual(events, expected)

    def test_grouped_runner_keeps_each_benchmark_contiguous(self) -> None:
        items = [bench("one"), bench("two")]
        calls = []

        def measure(item):
            calls.append(item.name)
            return sample(0.1, 0.05)

        driver.collect(
            driver.SectionPlan("S", runner=driver.grouped),
            items,
            measure,
        )

        self.assertEqual(calls, ["one"] * 5 + ["two"] * 5)

    def test_runner_can_be_replaced_for_one_section(self) -> None:
        items = [bench("one"), bench("two")]
        calls = []

        def measure(item):
            calls.append(item.name)
            return sample(0.1, 0.05)

        driver.collect(
            driver.SectionPlan("S", runner=driver.interleaved),
            items,
            measure,
        )

        self.assertEqual(calls, ["one", "two"] * 5)

    def test_measurement_can_be_replaced_for_one_section(self) -> None:
        item = bench("one")
        default_calls = []
        custom_calls = []

        def default_measure(item):
            default_calls.append(item.name)
            return sample(0.1, 0.05)

        def custom_measure(item):
            custom_calls.append(item.name)
            return sample(float(len(custom_calls)), 0.1)

        results = driver.collect(
            driver.SectionPlan("S", measure=custom_measure),
            [item],
            default_measure,
            validate=lambda _item: None,
        )

        self.assertEqual(default_calls, [])
        self.assertEqual(custom_calls, ["one"] * 6)
        self.assertEqual(
            [sample.wall_seconds for sample in results[0].samples],
            [2.0, 3.0, 4.0, 5.0, 6.0],
        )

    def test_tcp_policy_changes_without_changing_cpu_policy(self) -> None:
        tcp_section = next(
            section
            for section in catalog.SECTIONS
            if section.title == driver.TCP_TITLE
        )
        tcp_plan = driver.SectionPlan(
            driver.TCP_TITLE,
            runner=driver.interleaved,
        )

        with mock.patch.dict(
            driver.SECTION_PLANS,
            {driver.TCP_TITLE: tcp_plan},
        ):
            selected_tcp_plan = driver.plan_for(tcp_section)
            cpu_plan = driver.plan_for(catalog.Section("CPU"))

        self.assertIs(selected_tcp_plan, tcp_plan)
        self.assertIs(cpu_plan.runner, driver.grouped)

    def test_tcp_note_does_not_claim_a_runner_policy(self) -> None:
        plan = driver.plan_for(driver.TCP_SECTION)

        self.assertNotIn("interleaved", plan.note)
        self.assertNotIn("grouped", plan.note)


class MetricTest(unittest.TestCase):
    def test_cat_section_splits_cpu_and_orders_by_wall(self) -> None:
        section = next(
            section
            for section in catalog.SECTIONS
            if section.title == "CAT"
        )
        plan = driver.plan_for(section)
        wall_fast = catalog.Result(
            bench("wall_fast"),
            [catalog.Sample(0.1, 0.4, 0.4, 1024)],
        )
        wall_slow = catalog.Result(
            bench("wall_slow"),
            [catalog.Sample(0.2, 0.1, 0.1, 1024)],
        )

        text = driver.render_section(plan, [wall_slow, wall_fast])

        self.assertEqual(
            plan.metrics,
            (driver.WALL, driver.USER_CPU, driver.KERNEL_CPU, driver.MEM),
        )
        self.assertIs(plan.primary, driver.WALL)
        self.assertIn(
            "| Command | Wall [ms] ↓ | User CPU [ms] | Kernel CPU [ms] | "
            "Peak mem [MiB] | Relative |",
            text,
        )
        self.assertNotIn("| CPU [", text)
        self.assertLess(text.index("`wall_fast`"), text.index("`wall_slow`"))

    def test_tcp_section_omits_total_cpu_column(self) -> None:
        item = bench("tcp")
        result_sample = SimpleNamespace(
            wall_seconds=0.3,
            cpu_seconds=0.1,
            user_cpu_seconds=0.06,
            kernel_cpu_seconds=0.04,
            peak_rss_bytes=2 * 1024**2,
        )

        text = driver.render_section(
            driver.plan_for(driver.TCP_SECTION),
            [catalog.Result(item, [result_sample, result_sample])],
        )

        self.assertIn(
            "| Command | Wall [ms] | User CPU [ms] | Kernel CPU [ms] | "
            "Peak mem [MiB] | CPU relative ↓ |",
            text,
        )
        self.assertNotIn("| CPU [", text)

    def test_hidden_primary_metric_still_orders_rows(self) -> None:
        fast = bench("fast")
        slow = bench("slow")
        plan = driver.SectionPlan(
            "S",
            metrics=(driver.USER_CPU, driver.KERNEL_CPU),
            primary=driver.CPU,
        )

        text = driver.render_section(
            plan,
            [
                catalog.Result(
                    slow,
                    [catalog.Sample(1, 0.04, 0.08, 1024)] * 2,
                ),
                catalog.Result(
                    fast,
                    [catalog.Sample(1, 0.06, 0.04, 1024)] * 2,
                ),
            ],
        )

        self.assertLess(text.index("`fast`"), text.index("`slow`"))

    def test_section_controls_columns_and_primary_metric(self) -> None:
        fast = bench("fast")
        slow = bench("slow")
        plan = driver.SectionPlan(
            "S",
            metrics=(driver.CPU, driver.MEM),
            primary=driver.CPU,
        )

        text = driver.render_section(
            plan,
            [
                catalog.Result(slow, [sample(0.1, 0.2)] * 2),
                catalog.Result(fast, [sample(0.2, 0.1)] * 2),
            ],
        )

        self.assertIn(
            "| Command | CPU [ms] ↓ | Peak mem [MiB] | Relative |",
            text,
        )
        self.assertNotIn("Wall [", text)
        self.assertLess(text.index("`fast`"), text.index("`slow`"))


class CliTest(unittest.TestCase):
    def test_scan_delims_aot_benchmarks_smoke_validate(self) -> None:
        self.assertEqual(
            driver.main_for([
                "--smoke",
                "scan_delims_astil_cell_aot scan_delims_astil_naive_aot",
            ]),
            0,
        )

    def test_scan_delims_cell_benchmark_smoke_validates(self) -> None:
        self.assertEqual(
            driver.main_for([
                "--smoke",
                "scan_delims_astil_cell_jit",
            ]),
            0,
        )

    def test_smoke_does_not_touch_existing_report(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "bench.md"
            output.write_text("keep", encoding="utf-8")
            with mock.patch.object(catalog, "progress"):
                code = driver.main_for([
                    "--smoke",
                    "--output",
                    str(output),
                    "baseline_python",
                ])
            self.assertEqual(code, 0)
            self.assertEqual(output.read_text(encoding="utf-8"), "keep")

    def test_smoke_validates_without_creating_report(self) -> None:
        selected = catalog.selected_benches(["baseline_python"])
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "missing" / "bench.md"
            with (
                mock.patch.object(catalog, "validate") as validate,
                mock.patch.object(driver, "measure") as measure,
            ):
                code = driver.main_for([
                    "--smoke",
                    "--output",
                    str(output),
                    "baseline_python",
                ])

            self.assertEqual(code, 0)
            validate.assert_called_once_with(selected)
            measure.assert_not_called()
            self.assertFalse(output.parent.exists())


if __name__ == "__main__":
    unittest.main()
