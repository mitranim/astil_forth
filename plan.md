# Refactor Debloat Review Plan

Goal: review `main..HEAD` for verbosity, redundancy, and avoidable complexity while preserving correctness, intended behavior changes, and real features.

Sources used:
- `git diff --stat main..HEAD`: 87 files, +3264/-2869.
- `git diff --numstat main..HEAD`: biggest churn in `forth/lang.af` (+828/-544), `comp/comp_cc_reg.c` (+319/-519), `forth/test/test_const_fold.af` (+648), `comp/intrin_cc_reg.c` (+174/-182), `comp/arch_arm64.c` (+143/-182).
- `git diff --name-status main..HEAD`: broad test relocation to `forth/test/fail/`, `forth/test.af` moved to `forth/test/test.af`, old `forth/test_const_fold.af` replaced by larger `forth/test/test_const_fold.af`.
- `rg` anchors in current tree cited below.

## Review Rules

- Prefer keeping clearly intended behavioral changes that look like features.
- Prefer keeping documentation comments. Comments are review context, not bloat by default.
- Treat renaming as cosmetic, not suspect. Prefer new names when names are clearer.
- Prefer keeping features in Forth over moving them into C.
- Target real duplication, repeated choreography, stale transitional glue, and avoidable state coupling.
- For each chunk: ask whether helper boundaries can be simpler without reverting intended behavior or removing useful documentation.

## Phase 1: Register-CC Core

- [x] `comp/comp_cc_reg.h:13-167`: new local relocation model.
  - Suspect chunk: state surface around `Loc_reloc`, `Reg_imm`, `Comp_arg`, `Local.stable`, `Local.used`.
  - Keep documentation comments unless stale. Review whether state fields overlap or encode same lifecycle twice.
  - Source: `rg -n "Comp_arg|Loc_reloc|stable|used|imm|args\\[" comp/comp_cc_reg.h comp/comp_cc_reg.c`.
  - Verdict: keep. `reloc`, `stable`, `read`, `used`, and `Comp_arg { loc, imm }` encode separate lifecycle facts; implementation scan duplication belongs to the next chunk.

- [x] `comp/comp_cc_reg.c:138-147`, `223-271`, `295-361`: relocation writeback path.
  - Suspect chunk: `comp_local_confirm_relocs`, `comp_append_local_reloc_from_reg`, `comp_forget_reg`, `comp_assign_local_from_reg`.
  - Review for duplicated "does previous local still exist in another arg reg?" scans. Candidate helper: `comp_arg_loc_count/has_other_loc`.
  - Ignore rename itself. Check whether relocation lifecycle has avoidable extra machinery.
  - Verdict: simplify. Added one helper for the shared "arg reg still owns this local" check; kept relocation lifecycle intact.

- [x] `comp/comp_cc_reg.c:455-488`, `493-575`: immediate and local push path.
  - Suspect chunk: `comp_append_imm_to_reg`, `comp_append_push_imm`, `comp_append_push_from_local`.
  - Review for mixed responsibilities: register allocation, relocation, constant reuse, clobber registration, and `arg_len` mutation in one path.
  - Candidate simplification: split pure lookup from mutation, or remove constant-local association if marginal.
  - Verdict: simplify. Kept immediate path; simplified local push by clearing the target arg first, then using shared local-arg lookup for prior-local relocation.

- [x] `comp/comp_cc_reg.c:420-428`, `652-654`, `715-726`, `785-797`: `arg_len` mutation policy.
  - Suspect chunk: scattered direct `ctx->arg_len++`, `ctx->arg_len = ...`, and clearing `ctx->args`.
  - Review for single primitive that owns arg stack resize and forgetting. Current direct mutation risks hidden stale `args[]`.
  - Verdict: partial simplify. Extracted `comp_args_set` for explicit resize-and-forget behavior and fixed max-length validation; kept direct stack-height mutations where they intentionally preserve local-register associations above `arg_len`.

- [x] `comp/intrin_cc_reg.c:60-112`: brace assignment parser.
  - Suspect chunk: manual arrays `names/lens`, discard state `-1/1`, reverse assignment.
  - Review if this can use existing `read_valid_word`/`Word_str` path or be split into parse + apply.
  - Confirm whether `strncmp(buf, "}", len)` should be exact compare helper, not ad hoc.
  - Verdict: already simplified. Uses `read_valid_word`, `Word_str`, and exact `str_eq`; debug max-register assertion fixed with regression coverage.

- [x] `comp/intrin_cc_reg.c:482-540`, `797-828`: intrinsic surface API.
  - Suspect chunk: old `comp_next_arg_reg`, `comp_scratch_reg`, `comp_clobber`, `comp_local_get/set/off` replaced by `comp_alloc_next_reg`, `comp_realloc_reg`, `comp_push_from_local`, `comp_pop_into_local`.
  - New names are preferred. Review whether API level is right: enough Forth control, but not repeated call-site choreography.
  - Candidate: fewer higher-level Forth helpers wrapping these intrinsics, not moving feature logic into C.
  - Verdict: keep API. C surface is thin and appropriately primitive; repeated choreography lives in Forth call sites and is covered by the next phase.

## Phase 2: Forth Bootstrap Bloat

- [x] `forth/lang.af:163-235`, `282-292`, `655-657`, `789-817`: widespread `comp_realloc_reg`.
  - Suspect chunk: many one-off register realloc calls around primitive arithmetic.
  - Review for helper words/macros that encode "clobber output reg and set arity" once.
  - Source: `rg -n "comp_realloc_reg|comp_args_set" forth/lang.af`.
  - Verdict: simplify. Added small Forth helpers for one-output instruction emission and replaced obvious arithmetic/boolean/asr boilerplate; left scratch-register `mod` explicit and kept existing register-selection helpers.

- [x] `forth/lang.af:1794-1810`: `comp_realloc_regs`.
  - Suspect chunk: explicit 16-line list of volatile regs.
  - Candidate simplification: Forth loop over `ASM_ARG_LEN_MAX` if available and clear.
  - Verdict: simplify. Moved helper after counted loop support and rewrote the explicit x0..x15 list as `ASM_ARG_LEN_MAX -for:`.

- [x] `forth/lang.af:2024-2162`, `3274-3446`: continuation metadata.
  - Suspect chunk: packed bitfield helpers and repeated loop-gap handling.
  - Review if metadata format is too clever. Candidate: store loop frame fields on `LOOP_AUX` separately, or factor patching helpers.
  - Keep comments that document current invariants or intended feature limits. Only trim stale repetition.
  - Verdict: simplify lightly. Kept packed metadata and explanatory loop-gap comments; extracted the duplicated input/output arity equality check shared by conditionals and loops.

- [x] `forth/lang.af:3463-3664`: counted loops `for`, `+for:`, `-loop:`, `+loop:`.
  - Suspect chunk: repeated `comp_pop_into_local`, `comp_push_from_local`, `comp_barrier`, compare, `comp_args_set`, `cont_meta_with_*`.
  - Candidate: one shared compile helper for counted loop init/update/compare. Review before touching correctness.
  - Verdict: simplify lightly. Factored repeated counted-loop condition metadata/pop construction; left loop init/update/compare bodies explicit because their stack/local choreography differs.

- [x] `forth/lang.af:3826-3843` and `3922-3940`: `wrapf` and `strf`.
  - Suspect chunk: same save three args to locals, reload three args, call `snprintf`, return buffer.
  - Candidate: shared helper for "preserve first N args across varargs call" or restore prior shorter implementation if safe.
  - Verdict: simplify. Restored a specific-register local assignment intrinsic so `wrapf`/`strf` stash only `buf` from x0 before compiling `snprintf`; kept their different tails explicit.

- [x] `forth/lang.af:3963-4046`: `comp_args_to_stack`, `>>s`, `stack{`.
  - Suspect chunk: stack transfer logic now mixes locals, manual temp regs, and argument resetting.
  - Review whether higher-level stack words `>s`/`s>` are enough after their simplification at `1518-1539`.
  - Verdict: keep. `>s`/`s>` are runtime stack words, while these words compile register-to-memory moves and local assignments. Removing the `>>s` local save/reload loses needed compiler metadata and breaks `test_to_stack_variadic`.

- [x] `forth/lang.af:4146-4173`, `4218-4249`: `indirect:` and `execute_raw`.
  - Suspect chunk: manual `comp_realloc_regs`, synthetic input modeling via `inp comp_args_set`, frame-record workaround.
  - Prefer keeping this in Forth. Review if a shared Forth trampoline/helper can remove duplication.
  - Verdict: keep. `comp_realloc_regs` declares the generated indirect word's unknown target clobbers to callers; without it, locals live across an indirect call can remain assigned to registers clobbered by the eventual target. Added a regression test for this.

- [ ] `forth/lang.af:1701-1744`: string/comment parsing after `read_until_char` semantic change.
  - Suspect chunk: repeated `read_char { -- }` before/after parse in `s"`, `s\``, `read_interp_cstr`.
  - Candidate: add `read_delimited` helper to avoid every caller knowing delimiter consumption contract.

## Phase 3: Arm64 / Assembler

- [ ] `comp/arch_arm64.c:1044-1129`: immediate encoding rewrite.
  - Suspect chunk: deleted literal-pool fixups, added move-wide sequence.
  - Review for simpler lane selection and negative immediate handling. This is correctness-sensitive; simplify only with tests.
  - Source: `rg -n "asm_append_imm_to_reg|asm_backtrack_instrs_opt" comp/arch_arm64.c comp/arch_arm64_cc_reg.c comp/comp_cc_reg.c`.

- [ ] `comp/arch_arm64_cc_reg.c:496-558`: local reloc fixups and backtracking helper.
  - Suspect chunk: `asm_fixup_loc_reloc` still carries old "dirty hack" comment and NOP elision.
  - Keep documentation if it explains invariant. Review NOP elision and reloc patching for simpler control flow.

- [ ] `comp/arch_arm64.c:142-183`: param count/register validators.
  - Suspect chunk: split `*_reg`, `*_count`, `err_too_many_*`.
  - Review whether helpers reduce duplication enough. Maybe one validator with `requested_count` flag.

- [ ] `comp/comp.c:430-444`, `comp/intrin_cc_reg.c:186-200`: return now clears args in `comp_append_ret`.
  - Suspect chunk: behavior moved from intrinsic into shared compiler function under `#ifndef CALL_CONV_STACK`.
  - Review if this conditional belongs in CC-specific layer, not shared `comp.c`.

## Phase 4: Reader / Parsing

- [ ] `comp/read.c:56-94`, `128-239`, `256-304`: no-backtrack reader rewrite.
  - Suspect chunk: `read_char_at` plus manual `read->pos++` throughout number/word/string readers.
  - Review if backtracking removal simplified enough. Risk: repeated peeking logic and EOF validation.
  - Candidate: small `reader_peek_valid` / `reader_take_valid` pair.

- [ ] `comp/intrin.c:227-239`, `783-804`: `read_char` and `read_until_char` semantics.
  - Suspect chunk: `read_char` changed to return char+err; `read_until_char` now leaves delimiter unread.
  - Review for call-site burden in `forth/lang.af`. If many callers now do paired reads, helper belongs lower.

## Phase 5: Tests / Low-Value Review Scope

- [ ] `clib/err.h:13-51` and broad `assert_fatal` rename.
  - Rename is cosmetic and new names are preferred. Review only semantic additions: `err_assert`, `try_assert`, `try_fatal`, and `FAST_CRASH` behavior.

- [ ] `clib/dict.c:88-190`, `clib/map.c:50-146`, `clib/set.c:19-224`.
  - Suspect chunk: commented test code only changed by assert rename.
  - Rename-only churn is not suspect. Drop from de-bloat review unless commented mains are already planned for real tests.

- [ ] `comp/intrin_list_cc_reg.c:6-86`.
  - Mostly rename/list update. Review only grouping if it obscures feature ownership.

- [ ] `forth/test/test_const_fold.af` (+648) replacing `forth/test_const_fold.af` (-340).
  - Suspect chunk: test file nearly doubled.
  - Review for duplicated fixtures and broad cases that can be table-driven or split. Preserve coverage, reduce repetition.

- [ ] `forth/test/fail/*` renames.
  - Suspect chunk: many failure tests moved, few one-line changes.
  - Review only harness impact; ignore for de-bloat unless names/paths duplicate meaning.

## Suggested Review Order

1. Register-CC core state (`comp/comp_cc_reg.h`, `comp/comp_cc_reg.c`).
2. Forth call-site API fallout (`forth/lang.af` register/local helper churn).
3. Loop/control-flow metadata (`forth/lang.af` cont/loop blocks).
4. Reader delimiter semantics (`comp/read.c`, `comp/intrin.c`, affected Forth string words).
5. Arm64 immediate/reloc fixups.
6. Tests and semantic assertion helpers.

## Exit Criteria

- `plan.md` chunks reviewed one at a time, each ending with: keep / simplify / delete / defer.
- Any simplification has targeted test coverage before/after.
- No review chunk should treat cosmetic rename as de-bloat target.
