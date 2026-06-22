#!/usr/bin/env python3

# BOT-GENERATED

import argparse
import collections
import dataclasses
import pathlib
import re
import sys


DECL_WORDS = {
    "fun:",
    "fun_comp:",
    "struct:",
    "let:",
    "var:",
    "mem:",
    "buf:",
    "field:",
    "field_aligned:",
    "extern:",
    "int:",
    "indirect:",
}

END_WORDS = {"end"}
NEST_WORDS = {"then", "loop", "for", "do"}
EXEC_DEF_WORDS = {"fun:", "fun_comp:"}
# Keep this as an explicit review-derived list. Generalizing these public/API
# false positives would add parser complexity before the detector needs it.
SINGLE_USE_PUBLIC_WORDS = {
    ".s",
    ".sc",
    "=any",
    "again",
    "align_up:",
    "fill",
    "leave",
    "mem_map",
    "mem_prot",
    "mem_unmap",
    "mem_unprot",
    "slot",
    "stack_deinit",
    "stack_init",
    "stack_log_cells",
    "stack_trunc",
    "stderr",
    "stdin",
    "stdout",
    "stack:",
    "var:",
}
KEYWORDS = {
    "fun:",
    "fun_comp:",
    "struct:",
    "end",
    "if",
    "ifz",
    "then",
    "else",
    "elif",
    "elifz",
    "loop",
    "for",
    "while",
    "leave",
    "again",
    "ret",
    "throw",
    "try",
    "true",
    "false",
    "nil",
    "{",
    "}",
    "(",
    ")",
    "[",
    "]",
    "--",
    "->",
}

GENERIC_NORM_PARTS = {
    "ID",
    "NUM",
    "if",
    "ifz",
    "then",
    "else",
    "elif",
    "elifz",
    "end",
    "loop",
    "for",
    "while",
    "leave",
    "again",
    "ret",
    "true",
    "false",
    "nil",
    "[",
    "]",
}

NUM_RE = re.compile(r"^[+-]?(?:0b[01_]+|0o[0-7_]+|0x[0-9a-fA-F_]+|[0-9][0-9_]*)(?:\.[0-9a-fA-F_]+)?$")


@dataclasses.dataclass(frozen=True)
class Token:
    raw: str
    norm: str
    line: int
    col: int


@dataclasses.dataclass(frozen=True)
class Definition:
    kind: str
    name: str
    start_line: int
    end_line: int
    body: tuple[Token, ...]


@dataclasses.dataclass(frozen=True)
class Span:
    line: int
    text: str
    end_line: int | None = None


@dataclasses.dataclass(frozen=True)
class Cluster:
    key: tuple[str, ...]
    spans: tuple[Span, ...]


@dataclasses.dataclass(frozen=True)
class Finding:
    source: str
    cluster: Cluster


def normalize_word(raw: str, preserve_domain_names: bool = True) -> str:
    if raw == "STR":
        return raw
    if NUM_RE.match(raw):
        return "NUM"
    if raw in KEYWORDS:
        return raw
    if raw.startswith("ASM_") or raw.startswith("WORDLIST_"):
        return raw
    if preserve_domain_names and (raw.startswith("cf_") or raw.startswith("comp_")):
        return raw
    if raw.startswith("instr'") or raw.startswith("xt'") or raw.startswith("execute"):
        return raw
    return "ID"


def token_norm(tok: Token, preserve_domain_names: bool = True) -> str:
    if preserve_domain_names:
        return tok.norm
    return normalize_word(tok.raw, preserve_domain_names=False)


def noisy_key(key: tuple[str, ...], preserve_domain_names: bool) -> bool:
    return preserve_domain_names and "STR" not in key and all(part in GENERIC_NORM_PARTS for part in key)


def token_span_lines(tokens: tuple[Token, ...] | list[Token]) -> int:
    if not tokens:
        return 0
    return tokens[-1].line - tokens[0].line + 1


def _line_col(src: str, pos: int) -> tuple[int, int]:
    line = src.count("\n", 0, pos) + 1
    last = src.rfind("\n", 0, pos)
    col = pos + 1 if last < 0 else pos - last
    return line, col


def _word_boundary(src: str, pos: int) -> bool:
    return pos <= 0 or src[pos - 1].isspace()


def _read_word(src: str, pos: int) -> tuple[str, int]:
    start = pos
    while pos < len(src) and not src[pos].isspace():
        pos += 1
    return src[start:pos], pos


def _read_string(src: str, pos: int, word: str) -> int:
    quote = word[-1]
    delim_len = len(word) - len(word.rstrip(quote))
    delim = quote * delim_len

    while pos < len(src):
        if src.startswith(delim, pos):
            end = pos + delim_len
            while end < len(src) and not src[end].isspace():
                end += 1
            return end
        pos += 1

    return len(src)


def tokenize(src: str) -> list[Token]:
    out: list[Token] = []
    pos = 0

    while pos < len(src):
        char = src[pos]

        if char.isspace():
            pos += 1
            continue

        if _word_boundary(src, pos) and char == "\\":
            end = src.find("\n", pos)
            pos = len(src) if end < 0 else end + 1
            continue

        if _word_boundary(src, pos) and char == "(":
            end = src.find(")", pos + 1)
            pos = len(src) if end < 0 else end + 1
            continue

        raw, end = _read_word(src, pos)

        if raw.endswith('"') or raw.endswith("`"):
            quote = raw[-1]
            if quote in raw:
                line, col = _line_col(src, pos)
                out.append(Token(raw="STR", norm="STR", line=line, col=col))
                pos = _read_string(src, end, raw)
                continue

        line, col = _line_col(src, pos)
        out.append(Token(raw=raw, norm=normalize_word(raw), line=line, col=col))
        pos = end

    return out


def _strip_stack_signature(tokens: tuple[Token, ...]) -> tuple[Token, ...]:
    if not tokens or tokens[0].raw != "{":
        return tokens

    depth = 0
    for ind, tok in enumerate(tokens):
        if tok.raw == "{":
            depth += 1
        elif tok.raw == "}":
            depth -= 1
            if depth == 0:
                return tokens[ind + 1 :]

    return tokens


def _strip_brace_groups(tokens: tuple[Token, ...]) -> tuple[Token, ...]:
    out = []
    depth = 0
    for tok in tokens:
        if tok.raw == "{":
            depth += 1
            continue
        if depth:
            if tok.raw == "}":
                depth -= 1
            continue
        out.append(tok)
    return tuple(out)


def split_defs(tokens: list[Token]) -> list[Definition]:
    defs: list[Definition] = []
    ind = 0

    while ind < len(tokens):
        tok = tokens[ind]
        if tok.raw not in DECL_WORDS:
            ind += 1
            continue

        kind = tok.raw
        name = tokens[ind + 1].raw if ind + 1 < len(tokens) else "<missing>"
        beg = ind
        depth = 1 if kind in EXEC_DEF_WORDS | {"struct:"} else 0
        ind += 1

        while ind < len(tokens):
            raw = tokens[ind].raw
            if depth and raw in NEST_WORDS:
                depth += 1
            elif depth and raw in END_WORDS:
                depth -= 1
                if depth == 0:
                    ind += 1
                    break
            elif not depth:
                ind += 1
                break
            ind += 1

        segment = tokens[beg:ind]
        body_tokens = tuple(segment[2:-1] if depth == 0 and segment and segment[-1].raw == "end" else segment[2:])
        body = _strip_stack_signature(body_tokens)
        defs.append(
            Definition(
                kind=kind,
                name=name,
                start_line=segment[0].line,
                end_line=segment[-1].line,
                body=body,
            )
        )

    return defs


def find_ngram_clusters(
    tokens: list[Token],
    min_len: int = 4,
    max_len: int = 10,
    min_lines: int = 3,
    preserve_domain_names: bool = True,
) -> list[Cluster]:
    seen: dict[tuple[str, ...], list[int]] = collections.defaultdict(list)
    norms = [token_norm(tok, preserve_domain_names) for tok in tokens]

    for size in range(min_len, max_len + 1):
        for ind in range(0, len(norms) - size + 1):
            key = tuple(norms[ind : ind + size])
            if noisy_key(key, preserve_domain_names):
                continue
            if token_span_lines(tokens[ind : ind + size]) < min_lines:
                continue
            seen[key].append(ind)

    clusters: list[Cluster] = []
    for key, inds in seen.items():
        if len(inds) < 2:
            continue
        spans = []
        last = -999999
        for ind in inds:
            if ind - last < len(key):
                continue
            toks = tokens[ind : ind + len(key)]
            spans.append(Span(toks[0].line, " ".join(tok.raw for tok in toks), toks[-1].line))
            last = ind
        if len(spans) > 1:
            clusters.append(Cluster(key, tuple(spans)))

    clusters.sort(key=lambda item: (len(item.spans) * len(item.key), len(item.key)), reverse=True)
    return clusters


def find_definition_ngram_clusters(
    defs: list[Definition],
    min_len: int = 4,
    max_len: int = 10,
    min_lines: int = 3,
    preserve_domain_names: bool = True,
) -> list[Cluster]:
    seen: dict[tuple[str, ...], list[tuple[int, int]]] = collections.defaultdict(list)

    for def_ind, defn in enumerate(defs):
        if defn.kind not in EXEC_DEF_WORDS:
            continue
        body = _strip_brace_groups(defn.body)
        norms = [token_norm(tok, preserve_domain_names) for tok in body]
        for size in range(min_len, max_len + 1):
            for ind in range(0, len(norms) - size + 1):
                key = tuple(norms[ind : ind + size])
                if noisy_key(key, preserve_domain_names):
                    continue
                if token_span_lines(body[ind : ind + size]) < min_lines:
                    continue
                seen[key].append((def_ind, ind))

    clusters: list[Cluster] = []
    for key, vals in seen.items():
        if len(vals) < 2:
            continue
        spans = []
        last_by_def: dict[int, int] = {}
        for def_ind, ind in vals:
            last = last_by_def.get(def_ind, -999999)
            if ind - last < len(key):
                continue
            defn = defs[def_ind]
            toks = _strip_brace_groups(defn.body)[ind : ind + len(key)]
            spans.append(Span(toks[0].line, f"{defn.name}: {' '.join(tok.raw for tok in toks)}", toks[-1].line))
            last_by_def[def_ind] = ind
        if len(spans) > 1:
            clusters.append(Cluster(key, tuple(spans)))

    clusters.sort(key=lambda item: (len(item.spans) * len(item.key), len(item.key)), reverse=True)
    return clusters


def find_definition_clusters(defs: list[Definition], min_lines: int = 3, preserve_domain_names: bool = True) -> list[Cluster]:
    seen: dict[tuple[str, ...], list[Definition]] = collections.defaultdict(list)

    for defn in defs:
        if defn.kind not in EXEC_DEF_WORDS:
            continue
        body = _strip_brace_groups(defn.body)
        key = tuple(token_norm(tok, preserve_domain_names) for tok in body)
        if len(key) < 4:
            continue
        if token_span_lines(body) < min_lines:
            continue
        seen[key].append(defn)

    clusters = []
    for key, vals in seen.items():
        if len(vals) < 2:
            continue
        spans = tuple(Span(defn.start_line, f"{defn.kind} {defn.name}", defn.end_line) for defn in vals)
        clusters.append(Cluster(key, spans))

    clusters.sort(key=lambda item: (len(item.spans) * len(item.key), len(item.key)), reverse=True)
    return clusters


def single_use_noise_rank(name: str) -> int:
    if name.startswith(("asm_", "asm_pattern_")):
        return 4
    if name.endswith("_end_xt") or name in {"loop_frame_init"}:
        return 3
    if name.startswith("comp_loc_mut_") and name.endswith("_fallback"):
        return 2
    if name.startswith("cf_") and name.endswith(("_emit", "_validate")):
        return 2
    if name in SINGLE_USE_PUBLIC_WORDS:
        return 1
    if name.startswith("assert_"):
        return 1
    return 0


def find_single_use_definitions(
    tokens: list[Token],
    defs: list[Definition],
    hide_known_noise: bool = False,
) -> list[Finding]:
    def_by_name = {
        defn.name: defn
        for defn in defs
        if defn.kind in EXEC_DEF_WORDS
    }
    refs: dict[str, list[Token]] = collections.defaultdict(list)
    for ind, tok in enumerate(tokens):
        if tok.raw not in def_by_name:
            continue
        if ind > 0 and tokens[ind - 1].raw == def_by_name[tok.raw].kind:
            continue
        refs[tok.raw].append(tok)

    findings = []
    for name, vals in refs.items():
        if len(vals) != 1:
            continue
        defn = def_by_name[name]
        if hide_known_noise and single_use_noise_rank(name) > 0:
            continue
        findings.append(
            Finding(
                "single-use definitions",
                Cluster(
                    ("single-use", name),
                    (
                        Span(defn.start_line, f"{defn.kind} {defn.name}", defn.end_line),
                        Span(vals[0].line, f"call: {vals[0].raw}", vals[0].line),
                    ),
                ),
            )
        )
    findings.sort(key=lambda item: (single_use_noise_rank(item.cluster.key[1]), item.cluster.spans[0].line))
    return findings


def cluster_score(cluster: Cluster) -> tuple[int, int, int]:
    return (len(cluster.spans) * len(cluster.key), len(cluster.key), len(cluster.spans))


def cluster_lines(cluster: Cluster) -> frozenset[int]:
    lines = set()
    for span in cluster.spans:
        end_line = span.end_line if span.end_line is not None else span.line
        lines.update(range(span.line, end_line + 1))
    return frozenset(lines)


def collapse_overlapping_findings(findings: list[Finding]) -> list[Finding]:
    out: list[Finding] = []
    seen_duplicate_lines: list[frozenset[int]] = []

    for finding in findings:
        if finding.source == "single-use definitions":
            out.append(finding)
            continue

        lines = cluster_lines(finding.cluster)
        if any(lines and len(lines & seen) / min(len(lines), len(seen)) >= 0.8 for seen in seen_duplicate_lines):
            continue
        seen_duplicate_lines.append(lines)
        out.append(finding)

    return out


def collect_findings(
    tokens: list[Token],
    defs: list[Definition],
    min_len: int,
    max_len: int,
    min_lines: int,
    preserve_domain_names: bool,
    whole_file_ngrams: bool,
    kind: str = "all",
    hide_known_single_use_noise: bool = False,
) -> list[Finding]:
    findings: list[Finding] = []
    if kind in {"all", "duplicates"}:
        findings.extend(
            Finding("definition bodies", cluster)
            for cluster in find_definition_clusters(defs, min_lines, preserve_domain_names)
        )
        findings.extend(
            Finding("definition token ngrams", cluster)
            for cluster in find_definition_ngram_clusters(defs, min_len, max_len, min_lines, preserve_domain_names)
        )
        if whole_file_ngrams:
            findings.extend(
                Finding("whole-file token ngrams", cluster)
                for cluster in find_ngram_clusters(tokens, min_len, max_len, min_lines, preserve_domain_names)
            )
    if kind in {"all", "single-use"}:
        findings.extend(find_single_use_definitions(tokens, defs, hide_known_single_use_noise))
    findings.sort(key=lambda item: cluster_score(item.cluster), reverse=True)
    return collapse_overlapping_findings(findings)


def print_clusters(title: str, clusters: list[Cluster], limit: int) -> None:
    print(f"## {title}")
    if not clusters:
        print("(none)")
        return

    for cluster in clusters[:limit]:
        print_cluster(cluster)


def print_cluster(cluster: Cluster) -> None:
    print(f"- {len(cluster.key)} tokens x {len(cluster.spans)}")
    print(f"  shape: {' '.join(cluster.key)}")
    for span in cluster.spans[:8]:
        print(f"  line {span.line}: {span.text}")


def print_finding(finding: Finding, index: int, total: int) -> None:
    print(f"## finding {index + 1}/{total}: {finding.source}")
    print_cluster(finding.cluster)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Find repeated normalized shapes in Astil Forth code.")
    parser.add_argument("path", type=pathlib.Path)
    parser.add_argument("--limit", type=int, default=30)
    parser.add_argument("--min-len", type=int, default=4)
    parser.add_argument("--max-len", type=int, default=10)
    parser.add_argument(
        "--min-lines",
        type=int,
        default=3,
        help="Minimum physical line span for duplicate-shape findings.",
    )
    parser.add_argument(
        "--fold-domain-names",
        action="store_true",
        help="Normalize cf_* and comp_* util names to ID for broader shape matches.",
    )
    parser.add_argument(
        "--whole-file-ngrams",
        action="store_true",
        help="Also report ngrams over the whole token stream, including root-level code.",
    )
    parser.add_argument(
        "--kind",
        choices=("all", "duplicates", "single-use"),
        default="all",
        help="Which finding class to report.",
    )
    parser.add_argument(
        "--hide-known-single-use-noise",
        action="store_true",
        help="Hide public/API/protocol/asm single-use findings known to be weak debloat targets.",
    )
    parser.add_argument(
        "--one",
        action="store_true",
        help="Print exactly one ranked finding instead of sectioned output.",
    )
    parser.add_argument(
        "--skip",
        type=int,
        default=0,
        help="Skip this many ranked findings in --one mode.",
    )
    args = parser.parse_args(argv)

    src = args.path.read_text()
    tokens = tokenize(src)
    defs = split_defs(tokens)

    print(f"# {args.path}")
    print(f"tokens: {len(tokens)}")
    print(f"definitions: {len(defs)}")
    preserve_domain_names = not args.fold_domain_names
    if args.one:
        findings = collect_findings(
            tokens,
            defs,
            args.min_len,
            args.max_len,
            args.min_lines,
            preserve_domain_names,
            args.whole_file_ngrams,
            args.kind,
            args.hide_known_single_use_noise,
        )
        print(f"findings: {len(findings)}")
        if args.skip < 0:
            raise SystemExit("--skip must be >= 0")
        if args.skip >= len(findings):
            print("(none)")
            return 0
        print_finding(findings[args.skip], args.skip, len(findings))
        return 0

    if args.kind in {"all", "duplicates"}:
        print_clusters("definition bodies", find_definition_clusters(defs, args.min_lines, preserve_domain_names), args.limit)
        print_clusters(
            "definition token ngrams",
            find_definition_ngram_clusters(defs, args.min_len, args.max_len, args.min_lines, preserve_domain_names),
            args.limit,
        )
    if args.kind in {"all", "single-use"}:
        print_clusters(
            "single-use definitions",
            [
                finding.cluster
                for finding in find_single_use_definitions(tokens, defs, args.hide_known_single_use_noise)
            ],
            args.limit,
        )
    if args.kind in {"all", "duplicates"} and args.whole_file_ngrams:
        print_clusters(
            "whole-file token ngrams",
            find_ngram_clusters(tokens, args.min_len, args.max_len, args.min_lines, preserve_domain_names),
            args.limit,
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
