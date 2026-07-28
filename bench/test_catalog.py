# BOT-GENERATED

import errno
import signal
import subprocess
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from bench import cat_bench
from bench import catalog as bench


def samples(*walls: float) -> list[bench.Sample]:
    return [bench.Sample(wall, wall, 0, 1024) for wall in walls]


class PlanningTest(unittest.TestCase):
    def test_fib_loop_big_note_explains_integer_representation(self) -> None:
        section = next(
            section
            for section in bench.SECTIONS
            if section.title == "FIB_LOOP_BIG"
        )
        self.assertEqual(
            section.note,
            "C and Astil use `uint128`. Other languages use actual bigints.",
        )

    def test_cat_sections_immediately_precede_tcp(self) -> None:
        titles = [section.title for section in bench.SECTIONS]
        tcp_index = titles.index("TCP CONNECTIONS")
        self.assertEqual(
            titles[tcp_index - 2 : tcp_index + 1],
            ["CAT SMALL", "CAT LARGE", "TCP CONNECTIONS"],
        )

    def test_cat_implementations_follow_language_order(self) -> None:
        self.assertEqual(
            [
                item.name.removeprefix("cat_").removesuffix("_small")
                for item in bench.BENCHES
                if item.section == "CAT SMALL"
            ],
            [
                "global",
                "clang",
                "astil_aot",
                "astil_jit",
                "gforth",
                "zig",
                "go",
                "luajit",
                "js_bun",
                "java",
                "erlang_loop",
                "cl_sbcl",
                "pypy",
                "python",
            ],
        )

    def test_astil_cat_aot_setup_does_not_wait_for_stdin(self) -> None:
        item = bench.selected_benches(["cat_astil_aot_small"])[0]
        self.assertEqual(item.setup[-1][-2:], ("--", bench.os.devnull))

    def test_zig_cat_uses_one_source_for_both_sizes(self) -> None:
        items = bench.selected_benches(["cat_zig"])
        self.assertEqual(
            [(item.name, item.file) for item in items],
            [
                ("cat_zig_small", "bench/cat.zig"),
                ("cat_zig_large", "bench/cat.zig"),
            ],
        )

    def test_filters_are_combined_as_and_groups(self) -> None:
        names = [
            item.name
            for item in bench.selected_benches(["bin_tree", "java"])
        ]
        self.assertEqual(names, ["bin_tree_java"])

    def test_erlang_commands_encode_scheduler_policy(self) -> None:
        baselines = bench.selected_benches(["baseline_erlang"])
        self.assertEqual(
            [(item.name, item.cmd) for item in baselines],
            [
                (
                    "baseline_erlang_single",
                    (*bench.ERLANG_SERIAL, "-s", "erlang", "halt"),
                ),
                (
                    "baseline_erlang_default",
                    (*bench.ERLANG_DEFAULT, "-s", "erlang", "halt"),
                ),
            ],
        )
        self.assertEqual(
            bench.erlang_module("bench/example.erl"),
            (("erlc", "-Werror", "-o", "bench", "bench/example.erl"),),
        )
        self.assertEqual(
            bench.erlang_run("tcp_server_passive", serial=False),
            (
                *bench.ERLANG_DEFAULT,
                "-s",
                "tcp_server_passive",
                "main",
                "-s",
                "erlang",
                "halt",
            ),
        )
        self.assertEqual(
            bench.erlang_run(
                "tcp_server_passive",
                serial=False,
                runtime_flags=("+sbwt", "none"),
            ),
            (
                *bench.ERLANG_DEFAULT,
                "+sbwt",
                "none",
                "-s",
                "tcp_server_passive",
                "main",
                "-s",
                "erlang",
                "halt",
            ),
        )

class MeasurementTest(unittest.TestCase):
    def test_cat_measurement_reads_fixture_and_discards_stdout(self) -> None:
        item = bench.Bench(
            "TEST",
            "cat_timed",
            "cat",
            ("cat", "fixture", "-", "fixture"),
            cat=bench.CatIO(
                4,
                (cat_bench.FILE, cat_bench.STDIN, cat_bench.FILE),
            ),
        )
        usage = SimpleNamespace(ru_utime=0.0, ru_stime=0.0, ru_maxrss=0)
        with (
            tempfile.TemporaryDirectory() as directory,
            mock.patch.object(
                cat_bench,
                "input_path",
                return_value=Path(directory) / "input",
            ),
            mock.patch.object(
                bench.os, "posix_spawnp", return_value=1234
            ) as spawn,
            mock.patch.object(
                bench.os,
                "wait4",
                return_value=(1234, 0, usage),
            ),
        ):
            bench.measure(item)

        spawn.assert_called_once_with(
            "cat",
            item.cmd,
            bench.os.environ,
            file_actions=(
                (
                    bench.os.POSIX_SPAWN_OPEN,
                    0,
                    str(Path(directory) / "input"),
                    bench.os.O_RDONLY,
                    0,
                ),
                (
                    bench.os.POSIX_SPAWN_OPEN,
                    1,
                    bench.os.devnull,
                    bench.os.O_WRONLY,
                    0,
                ),
            ),
        )

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
    def test_erlang_active_n_server_rearms_after_batch(self) -> None:
        item = bench.selected_benches(["tcp_conn_erlang_active_n"])[0]
        compiled = subprocess.run(
            item.setup[0],
            cwd=bench.ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(compiled.returncode, 0, compiled.stderr)
        with (
            mock.patch.object(bench.tcp_conn, "CONNECTIONS", 64),
            mock.patch.object(bench.tcp_conn, "EXCHANGES", 65),
        ):
            try:
                sample = bench.measure(item, timeout_seconds=3)
            except bench.MeasurementDeadline:
                self.fail("Erlang active-N server did not rearm")
        self.assertGreater(sample.wall_seconds, 0)

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

    def test_python_thread_server_scales_without_gil_convoy(self) -> None:
        item = bench.selected_benches(["tcp_conn_python_thread"])[0]
        with (
            mock.patch.object(bench.tcp_conn, "CONNECTIONS", 2048),
            mock.patch.object(bench.tcp_conn, "EXCHANGES", 2),
        ):
            try:
                sample = bench.measure(item, timeout_seconds=5)
            except bench.MeasurementDeadline:
                self.fail("Python thread server hit GIL scheduling convoy")
        self.assertGreater(sample.wall_seconds, 0)

    def test_js_coroutine_server_handles_tcp_protocol(self) -> None:
        script = """
import { handlers } from "./bench/tcp_server_coro.mjs"
const bytes = []
let ended = false
const socket = {
  write(data) { bytes.push(...data) },
  end(data) {
    bytes.push(...data)
    ended = true
  },
}
handlers.open(socket)
handlers.data(socket, Uint8Array.of(68))
await Promise.resolve()
handlers.data(socket, Uint8Array.of(81))
await socket.data.task
console.log(JSON.stringify({ bytes, ended }))
"""
        result = subprocess.run(
            ("bun", "-e", script),
            cwd=bench.ROOT,
            capture_output=True,
            text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            result.stdout.strip(),
            '{"bytes":[82,68,81],"ended":true}',
        )


class ValidationTest(unittest.TestCase):
    def test_expected_cat_output_is_cached_by_workload(self) -> None:
        expected_output = cat_bench.expected_output
        self.assertTrue(
            hasattr(expected_output, "cache_info"),
            "expected_output has no workload cache",
        )
        expected_output.cache_clear()
        self.addCleanup(expected_output.cache_clear)
        cat = bench.CatIO(
            4,
            (cat_bench.FILE, cat_bench.STDIN, cat_bench.FILE),
        )

        first = expected_output(cat)
        before = expected_output.cache_info()
        second = expected_output(cat)
        after = expected_output.cache_info()

        self.assertEqual(second, first)
        self.assertEqual(after.hits, before.hits + 1)

    def test_cat_validation_rejects_replacing_stdin_with_file(self) -> None:
        cat = bench.CatIO(
            4,
            (cat_bench.FILE, cat_bench.STDIN, cat_bench.FILE),
        )
        with tempfile.TemporaryDirectory() as directory:
            input_path = Path(directory) / "file"
            stdin_path = Path(directory) / "stdin"
            input_path.write_bytes(cat_bench.PATTERNS[cat_bench.FILE][:4])
            stdin_path.write_bytes(cat_bench.PATTERNS[cat_bench.STDIN][:4])
            item = bench.Bench(
                "TEST",
                "bad_cat_sources",
                "python3",
                (
                    "python3",
                    "-c",
                    "import pathlib, sys; "
                    "data = pathlib.Path(sys.argv[1]).read_bytes(); "
                    "sys.stdout.buffer.write(data * 3)",
                    str(input_path),
                    "-",
                    str(input_path),
                ),
                cat=cat,
            )
            with (
                mock.patch.object(
                    cat_bench,
                    "input_path",
                    side_effect=lambda _cat, source: (
                        stdin_path
                        if source == cat_bench.STDIN
                        else input_path
                    ),
                ),
                self.assertRaisesRegex(
                    RuntimeError,
                    "bad_cat_sources.*output mismatch",
                ),
            ):
                bench.validate([item])

    def test_cat_validation_rejects_wrong_output(self) -> None:
        item = bench.Bench(
            "TEST",
            "bad_cat",
            "python3",
            ("python3", "-c", "import sys; sys.stdout.write('wrong')"),
            cat=bench.CatIO(4, (cat_bench.STDIN,)),
        )
        with tempfile.TemporaryDirectory() as directory:
            input_path = Path(directory) / "input"
            input_path.write_bytes(cat_bench.PATTERNS[cat_bench.STDIN][:4])
            with (
                mock.patch.object(
                    cat_bench,
                    "input_path",
                    return_value=input_path,
                ),
                self.assertRaisesRegex(
                    RuntimeError,
                    "bad_cat.*output mismatch",
                ),
            ):
                bench.validate([item])

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
                "none_astil_jit",
                "none_astil_stack",
                "baseline_js_bun",
            }
        }
        self.assertEqual(commands["none_astil_jit"], ("./astil.exe", "--eval="))
        self.assertEqual(commands["none_astil_stack"], ("./astil_s.exe", "--eval="))
        self.assertEqual(commands["baseline_js_bun"], ("bun", "-e", ";"))
