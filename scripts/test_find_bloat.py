#!/usr/bin/env python3

# BOT-GENERATED

import textwrap
import contextlib
import io
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import find_bloat


class FindBloatTest(unittest.TestCase):
    def test_tokenize_skips_comments_and_collapses_strings(self):
        src = textwrap.dedent(
            r'''
            fun: one { -- err }
              " hello world" 123 \ hidden words
              ( block words )
              0x10 end
            '''
        )

        vals = [tok.norm for tok in find_bloat.tokenize(src)]

        self.assertIn("STR", vals)
        self.assertIn("NUM", vals)
        self.assertNotIn("hidden", vals)
        self.assertNotIn("block", vals)

    def test_split_defs_handles_nested_end_blocks(self):
        src = textwrap.dedent(
            """
            fun: outer { -- err }
              loop
                again
              end
            end

            fun_comp: second { -- err } end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))

        self.assertEqual([defn.name for defn in defs], ["outer", "second"])
        self.assertIn("loop", [tok.raw for tok in defs[0].body])

    def test_split_defs_handles_if_end_blocks(self):
        src = textwrap.dedent(
            """
            fun: guarded { pred -- x }
              if pred then
                1
              else
                2
              end
            end

            fun: after { -- x } 3 end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))

        self.assertEqual([defn.name for defn in defs], ["guarded", "after"])
        self.assertIn("else", [tok.raw for tok in defs[0].body])
        self.assertEqual(8, defs[0].end_line)

    def test_split_defs_handles_then_end_blocks(self):
        src = textwrap.dedent(
            """
            fun: guarded { pred -- x }
              pred then
                1
              end
            end

            fun: after { -- x } 2 end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))

        self.assertEqual([defn.name for defn in defs], ["guarded", "after"])
        self.assertEqual(6, defs[0].end_line)

    def test_definition_body_omits_stack_signature(self):
        src = "fun: one { a b -- c } a b + end"

        defs = find_bloat.split_defs(find_bloat.tokenize(src))

        self.assertEqual(["a", "b", "+"], [tok.raw for tok in defs[0].body])

    def test_find_ngram_clusters_normalizes_names_and_numbers(self):
        src = textwrap.dedent(
            """
            fun: one { a b -- c }
              a b 1 foo
              c d 2 bar
              e f 3 baz
            end

            fun: two { x y -- z }
              x y 10 foo
              z q 20 bar
              m n 30 baz
            end
            """
        )

        clusters = find_bloat.find_ngram_clusters(find_bloat.tokenize(src), min_len=3, max_len=3, min_lines=1)

        self.assertTrue(any(len(cluster.spans) >= 2 for cluster in clusters))

    def test_definition_ngram_clusters_do_not_cross_definition_boundaries(self):
        src = textwrap.dedent(
            """
            fun: one { -- }
              left middle
            end

            fun: two { -- }
              right tail
            end

            fun: three { -- }
              left middle right tail
            end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))
        clusters = find_bloat.find_definition_ngram_clusters(defs, min_len=4, max_len=4, min_lines=1)

        self.assertEqual([], clusters)

    def test_ngram_clusters_can_fold_pure_util_shapes(self):
        src = textwrap.dedent(
            """
            fun_comp: one { -- }
              comp_emit_abs cf_fold_abs cf_apply_abs
            end

            fun_comp: two { -- }
              comp_emit_neg cf_fold_neg cf_apply_neg
            end
            """
        )

        tokens = find_bloat.tokenize(src)
        defs = find_bloat.split_defs(tokens)
        precise = find_bloat.find_definition_ngram_clusters(defs, min_len=3, max_len=3, min_lines=1)
        folded = find_bloat.find_definition_ngram_clusters(
            defs,
            min_len=3,
            max_len=3,
            min_lines=1,
            preserve_domain_names=False,
        )

        self.assertEqual([], precise)
        self.assertTrue(any(cluster.key == ("ID", "ID", "ID") for cluster in folded))

    def test_one_mode_prints_one_ranked_finding(self):
        src = textwrap.dedent(
            """
            fun: one { -- }
              a b 1 keep
              c d 2 keep
            end

            fun: two { -- }
              x y 10 keep
              z q 20 keep
            end
            """
        )

        with tempfile.NamedTemporaryFile("w+", suffix=".af") as tmp:
            tmp.write(src)
            tmp.flush()
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                rc = find_bloat.main([tmp.name, "--one", "--min-len", "3", "--max-len", "3", "--min-lines", "1"])

        text = out.getvalue()
        self.assertEqual(0, rc)
        self.assertIn("## finding 1/", text)
        self.assertEqual(1, sum(1 for line in text.splitlines() if line.startswith("- ")))

    def test_one_mode_skip_advances_ranked_finding(self):
        src = textwrap.dedent(
            """
            fun: one { -- }
              cf_a
              cf_b
              cf_c
            end

            fun: two { -- }
              cf_a
              cf_b
              cf_c
            end

            fun: three { -- }
              cf_x
              cf_y
              cf_z
            end

            fun: four { -- }
              cf_x
              cf_y
              cf_z
            end
            """
        )

        with tempfile.NamedTemporaryFile("w+", suffix=".af") as tmp:
            tmp.write(src)
            tmp.flush()
            first = io.StringIO()
            second = io.StringIO()
            with contextlib.redirect_stdout(first):
                find_bloat.main([tmp.name, "--one", "--min-len", "3", "--max-len", "3"])
            with contextlib.redirect_stdout(second):
                find_bloat.main([tmp.name, "--one", "--skip", "1", "--min-len", "3", "--max-len", "3"])

        self.assertIn("## finding 1/", first.getvalue())
        self.assertIn("## finding 2/", second.getvalue())
        self.assertNotEqual(first.getvalue(), second.getvalue())

    def test_default_line_threshold_filters_single_line_noise(self):
        src = textwrap.dedent(
            """
            fun: one { -- }
              { reg } val
            end

            fun: two { -- }
              { imm } val
            end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))
        clusters = find_bloat.find_definition_ngram_clusters(defs, min_len=4, max_len=4)

        self.assertEqual([], clusters)

    def test_three_line_duplicate_passes_default_threshold(self):
        src = textwrap.dedent(
            """
            fun: one { -- }
              a
              b
              1 cf_keep
            end

            fun: two { -- }
              x
              y
              2 cf_keep
            end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))
        clusters = find_bloat.find_definition_ngram_clusters(defs, min_len=4, max_len=4)

        self.assertTrue(any(cluster.key == ("ID", "ID", "NUM", "cf_keep") for cluster in clusters))

    def test_generic_id_num_duplicates_are_noise(self):
        src = textwrap.dedent(
            """
            fun: one { -- }
              a
              b
              1 keep
            end

            fun: two { -- }
              x
              y
              2 keep
            end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))
        clusters = find_bloat.find_definition_ngram_clusters(defs, min_len=4, max_len=4)

        self.assertEqual([], clusters)

    def test_overlapping_duplicate_windows_collapse_to_one_finding(self):
        src = textwrap.dedent(
            """
            fun: one { -- }
              cf_a
              cf_b
              cf_c
              cf_d
              cf_e
            end

            fun: two { -- }
              cf_a
              cf_b
              cf_c
              cf_d
              cf_e
            end
            """
        )

        tokens = find_bloat.tokenize(src)
        defs = find_bloat.split_defs(tokens)
        findings = find_bloat.collect_findings(
            tokens,
            defs,
            min_len=3,
            max_len=5,
            min_lines=3,
            preserve_domain_names=True,
            whole_file_ngrams=False,
            kind="duplicates",
        )

        self.assertEqual(1, len(findings))
        self.assertEqual(("cf_a", "cf_b", "cf_c", "cf_d", "cf_e"), findings[0].cluster.key)

    def test_struct_field_boilerplate_is_not_duplicate_bloat(self):
        src = textwrap.dedent(
            """
            struct: One
              U32 1 field: One_a
              U32 1 field: One_b
              U8  0 field: One_c
            end

            struct: Two
              U32 1 field: Two_a
              U32 1 field: Two_b
              U8  0 field: Two_c
            end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))
        clusters = find_bloat.find_definition_ngram_clusters(defs, min_len=4, max_len=10)

        self.assertEqual([], clusters)

    def test_local_binding_boilerplate_is_not_duplicate_bloat(self):
        src = textwrap.dedent(
            """
            fun: one { -- }
              make { left }
              use left { right }
              right done
            end

            fun: two { -- }
              make { alpha }
              use alpha { beta }
              beta done
            end
            """
        )

        defs = find_bloat.split_defs(find_bloat.tokenize(src))
        clusters = find_bloat.find_definition_ngram_clusters(defs, min_len=4, max_len=10)

        self.assertEqual([], [cluster for cluster in clusters if "{" in cluster.key or "}" in cluster.key])

    def test_single_use_definition_reported(self):
        src = textwrap.dedent(
            """
            fun: util { x -- y } x inc end
            fun: caller { x -- y } x util end
            fun: unused { x -- y } x dec end
            """
        )

        tokens = find_bloat.tokenize(src)
        defs = find_bloat.split_defs(tokens)
        findings = find_bloat.find_single_use_definitions(tokens, defs)

        self.assertEqual(["util"], [finding.cluster.key[1] for finding in findings])

    def test_single_use_ranks_asm_utils_last(self):
        src = textwrap.dedent(
            """
            fun: asm_one { x -- y } x inc end
            fun: util { x -- y } x dec end
            fun: caller { x -- y } x util x asm_one end
            """
        )

        tokens = find_bloat.tokenize(src)
        defs = find_bloat.split_defs(tokens)
        findings = find_bloat.find_single_use_definitions(tokens, defs)

        self.assertEqual(["util", "asm_one"], [finding.cluster.key[1] for finding in findings])

    def test_single_use_ranks_known_noise_after_plain_utils(self):
        src = textwrap.dedent(
            """
            fun: while_end_xt { -- err } finish_loop end
            fun_comp: leave { -- err } do_leave end
            fun: mem_map { size flags -- addr err } raw_map end
            fun: stack_log_cells { stack -- } stack log_cells end
            fun: stack: { -- } init_stack end
            fun: var: { -- } init_var end
            fun: cf_align_emit { x -- y } emit_align end
            fun: comp_loc_mut_align_down_fallback { loc imm -- err } fallback end
            fun: asm_brk { code -- instr } code enc end
            fun: util { x -- y } x dec end
            fun: caller { x -- y }
              x util while_end_xt leave mem_map stack_log_cells
              stack: var:
              cf_align_emit comp_loc_mut_align_down_fallback asm_brk
            end
            """
        )

        tokens = find_bloat.tokenize(src)
        defs = find_bloat.split_defs(tokens)
        findings = find_bloat.find_single_use_definitions(tokens, defs)

        self.assertEqual("util", findings[0].cluster.key[1])

    def test_single_use_can_hide_known_noise(self):
        src = textwrap.dedent(
            """
            fun: while_end_xt { -- err } finish_loop end
            fun_comp: leave { -- err } do_leave end
            fun: mem_map { size flags -- addr err } raw_map end
            fun: stack_log_cells { stack -- } stack log_cells end
            fun: stack: { -- } init_stack end
            fun: var: { -- } init_var end
            fun: cf_align_emit { x -- y } emit_align end
            fun: comp_loc_mut_align_down_fallback { loc imm -- err } fallback end
            fun: asm_brk { code -- instr } code enc end
            fun: util { x -- y } x dec end
            fun: caller { x -- y }
              x util while_end_xt leave mem_map stack_log_cells
              stack: var:
              cf_align_emit comp_loc_mut_align_down_fallback asm_brk
            end
            """
        )

        tokens = find_bloat.tokenize(src)
        defs = find_bloat.split_defs(tokens)
        findings = find_bloat.find_single_use_definitions(tokens, defs, hide_known_noise=True)

        self.assertEqual(["util"], [finding.cluster.key[1] for finding in findings])

    def test_one_mode_can_focus_single_use_findings(self):
        src = textwrap.dedent(
            """
            fun: util { x -- y } x inc end
            fun: caller { x -- y } x util end
            fun: repeated { x -- y } x inc end
            fun: repeated_user0 { x -- y } x repeated end
            fun: repeated_user1 { x -- y } x repeated end
            """
        )

        with tempfile.NamedTemporaryFile("w+", suffix=".af") as tmp:
            tmp.write(src)
            tmp.flush()
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                find_bloat.main([tmp.name, "--one", "--kind", "single-use"])

        text = out.getvalue()
        self.assertIn("single-use definitions", text)
        self.assertIn("single-use util", text)
        self.assertNotIn("single-use repeated", text)


if __name__ == "__main__":
    unittest.main()
