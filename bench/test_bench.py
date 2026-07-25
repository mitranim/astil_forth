# BOT-GENERATED

import unittest
import tempfile
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

    def test_validation_immediately_precedes_each_group(self) -> None:
        items = [bench("one"), bench("two")]
        events = []

        def validate(item):
            events.append(f"validate {item.name}")

        def measure(item):
            events.append(f"measure {item.name}")
            return sample(0.1, 0.05)

        driver.collect(
            driver.SectionPlan("S", runner=driver.grouped),
            items,
            measure,
            validate=validate,
        )

        self.assertEqual(
            events,
            ["validate one"]
            + ["measure one"] * 5
            + ["validate two"]
            + ["measure two"] * 5,
        )

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
            return sample(0.2, 0.1)

        results = driver.collect(
            driver.SectionPlan("S", measure=custom_measure),
            [item],
            default_measure,
        )

        self.assertEqual(default_calls, [])
        self.assertEqual(custom_calls, ["one"] * 5)
        self.assertEqual(results[0].samples[0].wall_seconds, 0.2)

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
            metrics=(driver.CPU, driver.RSS),
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
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "missing" / "bench.md"
            code = driver.main_for([
                "--smoke",
                "--output",
                str(output),
                "baseline_python",
            ])
            self.assertEqual(code, 0)
            self.assertFalse(output.parent.exists())


if __name__ == "__main__":
    unittest.main()
