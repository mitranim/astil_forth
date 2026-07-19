import contextlib
import errno
import io
import signal
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from bench import bench


def samples(*walls: float) -> list[bench.Sample]:
    return [bench.Sample(wall, wall, 1024) for wall in walls]


class PlanningTest(unittest.TestCase):
    def test_filters_are_combined_as_and_groups(self) -> None:
        names = [
            item.name
            for item in bench.selected_benches(["bin_tree", "java"])
        ]
        self.assertEqual(names, ["bin_tree_java"])


class InstabilityTest(unittest.TestCase):
    def test_stable_samples_have_no_warning(self) -> None:
        self.assertIsNone(bench.wall_instability(samples(1, 1.01, 1, 0.99, 1)))

    def test_one_extreme_sample_has_no_warning(self) -> None:
        self.assertIsNone(bench.wall_instability(samples(1, 1, 1, 1, 100)))

    def test_unstable_middle_half_warns(self) -> None:
        self.assertGreater(
            bench.wall_instability(samples(1, 1, 1, 100, 100)) or 0,
            bench.MAX_IQR_RATIO,
        )


class MeasurementStabilityTest(unittest.TestCase):
    def test_requires_five_runs(self) -> None:
        self.assertFalse(
            bench.measurement_stable(samples(0.3, 0.3, 0.3, 0.3))
        )

    def test_requires_one_second_of_completed_runs(self) -> None:
        self.assertFalse(
            bench.measurement_stable(samples(*([0.1] * 5)))
        )

    def test_rejects_relative_standard_error_above_one_percent(self) -> None:
        self.assertFalse(
            bench.measurement_stable(
                samples(0.1, 0.1, 0.1, 0.1, 0.2, 0.4)
            )
        )

    def test_accepts_precise_mean_after_minimums(self) -> None:
        self.assertTrue(
            bench.measurement_stable(samples(*([0.1] * 10)))
        )


class MeasurementTest(unittest.TestCase):
    def test_measure_records_wall_cpu_and_rss(self) -> None:
        item = bench.Bench(
            "TEST",
            "working_benchmark",
            "python3",
            ("python3", "-c", "sum(range(100000))"),
        )
        sample = bench.measure(item)
        self.assertGreater(sample.wall_seconds, 0)
        self.assertGreater(sample.cpu_seconds, 0)
        self.assertGreater(sample.peak_rss_bytes, 0)

    def test_measure_nonzero_exit_reports_benchmark_and_command(self) -> None:
        item = bench.Bench(
            "TEST",
            "failing_benchmark",
            "python3",
            ("python3", "-c", "raise SystemExit(7)"),
        )
        with self.assertRaises(RuntimeError) as caught:
            bench.measure(item)
        message = str(caught.exception)
        self.assertIn("failing_benchmark", message)
        self.assertIn(repr(item.cmd), message)
        self.assertIn("exit status 7", message)

    def test_measure_spawn_error_reports_benchmark_and_command(self) -> None:
        item = bench.Bench(
            "TEST",
            "missing_benchmark",
            "missing",
            ("/definitely/missing/benchmark-command", "--flag"),
        )
        with self.assertRaises(RuntimeError) as caught:
            bench.measure(item)
        message = str(caught.exception)
        self.assertIn("missing_benchmark", message)
        self.assertIn(repr(item.cmd), message)
        self.assertIsInstance(caught.exception.__cause__, OSError)

    def test_measure_wait_error_reports_context_and_reaps_child(self) -> None:
        item = bench.Bench(
            "TEST",
            "waiting_benchmark",
            "python3",
            ("python3", "-c", ""),
        )
        with (
            mock.patch.object(bench.os, "posix_spawnp", return_value=1234),
            mock.patch.object(
                bench.os, "wait4", side_effect=OSError("wait failed")
            ),
            mock.patch.object(bench.os, "kill") as kill,
            mock.patch.object(
                bench.os,
                "waitpid",
                side_effect=[(0, 0), (1234, 0)],
            ) as waitpid,
            self.assertRaises(RuntimeError) as caught,
        ):
            bench.measure(item)
        message = str(caught.exception)
        self.assertIn("waiting_benchmark", message)
        self.assertIn(repr(item.cmd), message)
        self.assertIsInstance(caught.exception.__cause__, OSError)
        kill.assert_called_once_with(1234, signal.SIGKILL)
        self.assertEqual(
            waitpid.call_args_list,
            [
                mock.call(1234, bench.os.WNOHANG),
                mock.call(1234, 0),
            ],
        )

    def test_measure_echild_reports_context_without_signaling_pid(self) -> None:
        item = bench.Bench(
            "TEST",
            "reaped_benchmark",
            "python3",
            ("python3", "-c", ""),
        )
        with (
            mock.patch.object(bench.os, "posix_spawnp", return_value=1234),
            mock.patch.object(
                bench.os,
                "wait4",
                side_effect=ChildProcessError(
                    errno.ECHILD, "No child processes"
                ),
            ),
            mock.patch.object(bench.os, "kill") as kill,
            mock.patch.object(bench.os, "waitpid") as waitpid,
            self.assertRaises(RuntimeError) as caught,
        ):
            bench.measure(item)
        message = str(caught.exception)
        self.assertIn("reaped_benchmark", message)
        self.assertIn(repr(item.cmd), message)
        self.assertIsInstance(caught.exception.__cause__, ChildProcessError)
        kill.assert_not_called()
        waitpid.assert_not_called()

    def test_timeout_kills_owned_child_and_restores_alarm(self) -> None:
        item = bench.Bench(
            "TEST",
            "timed_benchmark",
            "python3",
            ("python3", "-c", ""),
        )
        with (
            mock.patch.object(bench.os, "posix_spawnp", return_value=1234),
            mock.patch.object(
                bench.os,
                "wait4",
                side_effect=bench.MeasurementDeadline(),
            ),
            mock.patch.object(bench.os, "kill") as kill,
            mock.patch.object(
                bench.os,
                "waitpid",
                side_effect=[(0, 0), (1234, 0)],
            ) as waitpid,
            mock.patch.object(
                bench.signal,
                "signal",
                return_value=signal.SIG_DFL,
            ),
            mock.patch.object(bench.signal, "setitimer") as setitimer,
            self.assertRaises(bench.MeasurementDeadline),
        ):
            bench.measure(item, timeout_seconds=1)
        kill.assert_called_once_with(1234, signal.SIGKILL)
        self.assertEqual(
            waitpid.call_args_list,
            [
                mock.call(1234, bench.os.WNOHANG),
                mock.call(1234, 0),
            ],
        )
        self.assertEqual(
            setitimer.call_args_list[-1],
            mock.call(signal.ITIMER_REAL, 0),
        )

    def test_cleanup_does_not_signal_pid_after_ownership_is_lost(self) -> None:
        with (
            mock.patch.object(
                bench.os,
                "waitpid",
                side_effect=ChildProcessError(
                    errno.ECHILD, "No child processes"
                ),
            ),
            mock.patch.object(bench.os, "kill") as kill,
        ):
            bench.terminate_and_reap(1234)
        kill.assert_not_called()


class RenderingTest(unittest.TestCase):
    def test_summary_reports_mean_and_sample_deviation(self) -> None:
        got = bench.summarize(samples(1, 2, 3), "wall_seconds")
        self.assertEqual(got, (2, 1))

    def test_measurement_preserves_small_nonzero_deviation(self) -> None:
        self.assertEqual(
            bench.format_measurement(1, 0.001, bench.format_seconds),
            "1.000 s ± 0.001 s",
        )

    def test_measurement_mean_matches_deviation_precision(self) -> None:
        self.assertEqual(
            bench.format_measurement(8.198, 0.028, bench.format_seconds),
            "8.198 s ± 0.028 s",
        )

    def test_measurement_uses_significant_digits_for_tiny_deviation(self) -> None:
        self.assertEqual(
            bench.format_measurement(1, 0.000_000_01, bench.format_seconds),
            "1.000 s ± 1.0e-08 s",
        )

    def test_one_sample_has_no_deviation(self) -> None:
        self.assertEqual(bench.summarize(samples(1), "wall_seconds"), (1, None))
        self.assertEqual(
            bench.format_measurement(1, None, bench.format_seconds),
            "1.0 s ± —",
        )
        item = bench.Bench("TEST", "single", "single", ("single",))
        self.assertTrue(
            bench.format_done(bench.Result(item, samples(1))).endswith(
                "; 1 run"
            )
        )

    def test_tty_estimate_reports_running_means_and_count(self) -> None:
        progress_samples = [
            bench.Sample(1, 2, 3 * 1024**2),
            bench.Sample(3, 4, 5 * 1024**2),
        ]
        self.assertEqual(
            bench.format_estimate("bin_tree_java", 2, progress_samples),
            (
                "[bench] [bin_tree_java] 2 runs; "
                "wall ≈2.0 s; CPU ≈3.0 s; mem ≈4.0 MiB"
            ),
        )

    def test_section_sorts_by_wall_mean(self) -> None:
        slow = bench.Bench("S", "slow", "slow", ("slow",))
        fast = bench.Bench("S", "fast", "fast", ("fast",))
        text = bench.render_section(
            "S",
            [
                bench.Result(slow, samples(2, 2, 2, 2, 2)),
                bench.Result(fast, samples(1, 1, 1, 1, 1)),
            ],
        )
        self.assertLess(text.index("`fast`"), text.index("`slow`"))
        self.assertIn(
            "| Command | Wall [s] | CPU [s] | Peak mem [MiB] | Relative |",
            text,
        )
        self.assertIn("| `fast` |", text)
        self.assertIn("| 1.00 |", text)
        self.assertIn("| 2.00 |", text)
        self.assertNotIn(" Min ", text)
        self.assertNotIn(" Max ", text)

    def test_section_uses_milliseconds_for_all_subsecond_rows(self) -> None:
        fast = bench.Bench("S", "fast", "fast", ("fast",))
        slow = bench.Bench("S", "slow", "slow", ("slow",))
        text = bench.render_section(
            "S",
            [
                bench.Result(fast, samples(0.0189, 0.0191)),
                bench.Result(slow, samples(0.6621, 0.6623)),
            ],
        )
        self.assertIn("| Wall [ms] | CPU [ms] |", text)
        self.assertIn("| `fast` | 19.00 ± 0.14 |", text)
        self.assertIn("| `slow` | 662.20 ± 0.14 |", text)

    def test_section_uses_seconds_when_typical_row_is_seconds(self) -> None:
        fast = bench.Bench("S", "fast", "fast", ("fast",))
        slow = bench.Bench("S", "slow", "slow", ("slow",))
        text = bench.render_section(
            "S",
            [
                bench.Result(fast, samples(0.304, 0.306)),
                bench.Result(slow, samples(8.197, 8.199)),
            ],
        )
        self.assertIn("| Wall [s] | CPU [s] |", text)
        self.assertIn("| `fast` | 0.305 ± 0.001 |", text)
        self.assertIn("| `slow` | 8.198 ± 0.001 |", text)

    def test_section_selects_wall_and_cpu_units_independently(self) -> None:
        item = bench.Bench("S", "mixed", "mixed", ("mixed",))
        result_samples = [
            bench.Sample(2, 0.02, 1024),
            bench.Sample(2, 0.02, 1024),
        ]
        text = bench.render_section(
            "S", [bench.Result(item, result_samples)]
        )
        self.assertIn("| Wall [s] | CPU [ms] |", text)
        self.assertIn("| `mixed` | 2.0 ± 0.0 | 20.0 ± 0.0 |", text)

    def test_section_warns_once_for_unstable_results(self) -> None:
        item = bench.Bench("S", "wild", "wild", ("wild",))
        text = bench.render_section(
            "S", [bench.Result(item, samples(1, 1, 1, 100, 100))]
        )
        self.assertEqual(text.count("Warning:"), 1)


class MeasurementBudgetTest(unittest.TestCase):
    def test_stable_samples_stop_before_deadline(self) -> None:
        item = bench.Bench("TEST", "stable", "stable", ("stable",))
        stable_samples = samples(*([0.1] * 10))
        with (
            mock.patch.object(
                bench,
                "measure",
                side_effect=stable_samples + [AssertionError()],
            ) as measure,
            contextlib.redirect_stderr(io.StringIO()),
        ):
            result = bench.measure_bench(item)
        self.assertEqual(result.samples, stable_samples)
        self.assertEqual(measure.call_count, 10)

    def test_deadline_with_imprecise_samples_warns(self) -> None:
        item = bench.Bench("TEST", "wild", "wild", ("wild",))
        complete = samples(1, 10)
        stderr = io.StringIO()
        with (
            mock.patch.object(
                bench,
                "measure",
                side_effect=complete + [bench.MeasurementDeadline()],
            ),
            contextlib.redirect_stderr(stderr),
        ):
            result = bench.measure_bench(item)
        self.assertEqual(result.samples, complete)
        self.assertIn(
            "precision target not reached before 60 second limit",
            stderr.getvalue(),
        )

    def test_completed_samples_survive_partial_timeout(self) -> None:
        item = bench.Bench("TEST", "some", "some", ("some",))
        sample = bench.Sample(1, 0.5, 1024)
        with (
            mock.patch.object(
                bench,
                "measure",
                side_effect=[sample, bench.MeasurementDeadline()],
            ),
            contextlib.redirect_stderr(io.StringIO()),
        ):
            result = bench.measure_bench(item)
        self.assertEqual(result.samples, [sample])

    def test_zero_completed_runs_fail(self) -> None:
        item = bench.Bench("TEST", "slow", "slow", ("slow",))
        with (
            mock.patch.object(
                bench,
                "measure",
                side_effect=bench.MeasurementDeadline(),
            ),
            contextlib.redirect_stderr(io.StringIO()),
            self.assertRaisesRegex(RuntimeError, "slow.*60"),
        ):
            bench.measure_bench(item)


class ValidationTest(unittest.TestCase):
    def test_reports_through_progress(self) -> None:
        item = bench.Bench("TEST", "working", "working", ("working",))
        with (
            mock.patch.object(bench, "measure"),
            mock.patch.object(bench, "progress") as progress,
        ):
            bench.validate([item])
        progress.assert_called_once_with("[working] validation")


class SmokeTest(unittest.TestCase):
    def test_noop_benchmarks_pass_explicit_programs(self) -> None:
        commands = {
            item.name: item.cmd
            for item in bench.BENCHES
            if item.name in {
                "none_astil_reg",
                "none_astil_stack",
                "baseline_js_bun",
            }
        }
        self.assertEqual(commands["none_astil_reg"], ("./astil.exe", "--eval="))
        self.assertEqual(commands["none_astil_stack"], ("./astil_s.exe", "--eval="))
        self.assertEqual(commands["baseline_js_bun"], ("bun", "-e", ";"))

    def test_smoke_does_not_touch_output(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "bench.md"
            output.write_text("keep", encoding="utf-8")
            with mock.patch.object(bench, "progress"):
                code = bench.main_for(
                    ["--smoke", "--output", str(output), "baseline_python"]
                )
            self.assertEqual(code, 0)
            self.assertEqual(output.read_text(encoding="utf-8"), "keep")

    def test_smoke_does_not_create_absent_output_parent(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "missing" / "nested" / "bench.md"
            with mock.patch.object(bench, "progress"):
                code = bench.main_for(
                    ["--smoke", "--output", str(output), "baseline_python"]
                )
            self.assertEqual(code, 0)
            self.assertFalse(output.parent.exists())

    def test_transitional_options_are_not_public(self) -> None:
        for option in ("--prototype", "--no-progress"):
            with (
                self.subTest(option=option),
                contextlib.redirect_stderr(io.StringIO()),
                self.assertRaises(SystemExit),
            ):
                bench.parse_args([option])
