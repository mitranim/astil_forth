# BOT-GENERATED

import errno
import signal
import subprocess
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from bench import catalog as bench


def samples(*walls: float) -> list[bench.Sample]:
    return [bench.Sample(wall, wall, 0, 1024) for wall in walls]


class PlanningTest(unittest.TestCase):
    def test_filters_are_combined_as_and_groups(self) -> None:
        names = [
            item.name
            for item in bench.selected_benches(["bin_tree", "java"])
        ]
        self.assertEqual(names, ["bin_tree_java"])

    def test_java_async_tcp_benchmark_is_selectable(self) -> None:
        items = bench.selected_benches(["tcp_conn_java_async"])
        self.assertEqual(
            [(item.file, item.cmd) for item in items],
            [
                (
                    "bench/tcp_server_async.java",
                    ("java", "-cp", "bench", "tcp_server_async"),
                )
            ],
        )


class MeasurementTest(unittest.TestCase):
    def test_measure_keeps_user_and_kernel_cpu(self) -> None:
        item = bench.Bench("TEST", "cpu_split", "cpu_split", ("cpu_split",))
        usage = SimpleNamespace(ru_utime=2.0, ru_stime=3.0, ru_maxrss=4)
        with (
            mock.patch.object(bench.os, "posix_spawnp", return_value=1234),
            mock.patch.object(
                bench.os,
                "wait4",
                return_value=(1234, 0, usage),
            ),
        ):
            sample = bench.measure(item)

        self.assertEqual(
            (
                getattr(sample, "user_cpu_seconds", None),
                getattr(sample, "kernel_cpu_seconds", None),
                sample.cpu_seconds,
            ),
            (2.0, 3.0, 5.0),
        )

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

    def test_measurement_rounds_tiny_deviation_without_exponent(self) -> None:
        self.assertEqual(
            bench.format_measurement(1, 0.000_000_01, bench.format_seconds),
            "1.000 s ± 0.000 s",
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

class TcpServerTest(unittest.TestCase):
    def test_driver_reuses_connections_for_all_exchanges(self) -> None:
        clients = []

        class Client:
            def __init__(self, *_args):
                self.received = [bench.tcp_conn.READY]
                self.sent = []
                clients.append(self)

            def connect(self, _address):
                pass

            def sendall(self, data):
                self.sent.append(data)
                self.received.append(data)

            def recv(self, _size):
                if self.received:
                    return self.received.pop(0)
                return b""

            def close(self):
                pass

        with (
            mock.patch.object(bench.tcp_conn, "CONNECTIONS", 2),
            mock.patch.object(bench.tcp_conn, "EXCHANGES", 3),
            mock.patch.object(bench.tcp_conn.socket, "socket", Client),
        ):
            bench.tcp_conn.drive()

        self.assertEqual(len(clients), 2)
        for client in clients:
            self.assertEqual(
                client.sent,
                [bench.tcp_conn.DATA] * 2 + [bench.tcp_conn.CLOSE],
            )

    def test_java_async_server_handles_tcp_protocol(self) -> None:
        item = bench.selected_benches(["tcp_conn_java_async"])[0]
        compiled = subprocess.run(
            item.setup[0],
            cwd=bench.ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(compiled.returncode, 0, compiled.stderr)
        with mock.patch.object(bench.tcp_conn, "CONNECTIONS", 64):
            sample = bench.measure(item, timeout_seconds=10)
        self.assertGreater(sample.wall_seconds, 0)


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
    def test_make_clean_removes_project_temp_directory(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            temporary = root / ".tmp"
            temporary.mkdir()
            (temporary / "marker").touch()
            subprocess.run(
                (
                    "make",
                    "-f",
                    str(bench.ROOT / "makefile"),
                    "clean",
                    "GEN_DIR=generated",
                    "ARTIF=",
                    f"CLIB_DIR={bench.ROOT / 'clib'}",
                    f"COMP_DIR={bench.ROOT / 'comp'}",
                ),
                cwd=root,
                check=True,
            )
            self.assertFalse(temporary.exists())

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
