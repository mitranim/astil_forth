% BOT-GENERATED

-module(scan_delims_naive).
-export([main/0]).

% Kept: tuple lookup with eight-byte unrolling; avoids a call per byte.
% Rejected: bulk matches and repeated binary:match; allocation or BIF volume.

is_delimiter(${) -> 1;
is_delimiter($}) -> 1;
is_delimiter($[) -> 1;
is_delimiter($]) -> 1;
is_delimiter($:) -> 1;
is_delimiter($,) -> 1;
is_delimiter($\s) -> 1;
is_delimiter($\n) -> 1;
is_delimiter($\t) -> 1;
is_delimiter(_) -> 0.

delimiter_table() ->
  list_to_tuple([is_delimiter(Byte) || Byte <- lists:seq(0, 255)]).

scan(<<A, B, C, D, E, F, G, H, Rest/binary>>, Table, Count) ->
  scan(
    Rest,
    Table,
    Count +
      element(A + 1, Table) +
      element(B + 1, Table) +
      element(C + 1, Table) +
      element(D + 1, Table) +
      element(E + 1, Table) +
      element(F + 1, Table) +
      element(G + 1, Table) +
      element(H + 1, Table)
  );
scan(<<Byte, Rest/binary>>, Table, Count) ->
  scan(Rest, Table, Count + element(Byte + 1, Table));
scan(<<>>, _Table, Count) -> Count.

repeat(0, _Input, _Table, Count) -> Count;
repeat(Runs, Input, Table, Count) ->
  repeat(Runs - 1, Input, Table, Count + scan(Input, Table, 0)).

main() ->
  Input = binary:copy(<<"{a,b:c[d]e} \n\tfg">>, 4096),
  Table = delimiter_table(),
  1207959552 = repeat(32768, Input, Table, 0),
  ok.
