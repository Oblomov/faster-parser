# Faster parsers

When dealing with text-based file formats, one of the first discoveries
is that loading (deserializing) the data is often not an I/O bound task.
For many file formats, the bottleneck is rather in _parsing_ the data
itself. Even when the data is little more than a collection of integers
or floating-point values, developers quickly realize that most of the
time is spent in standard library functions such as `strtof` or
`strtol`.

What I present here is a collection of routines to parse integers and
floating-point data which are considerably (in the whereabous of 2×)
faster than the standard library functions. The speed obviously comes at
a price, as my routines are less robust and less featureful than the
standard library ones: they are not be intended as _general purpose_
replacements for the standard library ones, but rather as faster
alternatives in contexts where the flexibility and robustness of the
standard library ones can be forgone (e.g. for known-good data).

## `fast_parse_{u,}int{32,64}`

These are functions that are used to parse a 32- or 64-bit signed or
unsigned integer from its base-10 ASCII representation. No attempt
is made at skipping leading whitespace, to allow faster parsing in case
other symbols are used as separators. There is also no overflow check
and no check for actual digits.

While they are presented here in function form, in my experience it is
possible to achieve measurably faster performance by manually inlining
the code in the hot path. This is made easier by the provided
`CHECK_SIGN` and `FAST_DIGIT_LOOP` macros.

## `fast_parse_float32`

This function mimicks the behavior of the standard `strtof`, under the
additional assumptions that:

* the source is a base-10 ASCII representation, without thousands
  separator and with the dot `.` (ASCII 46) as decimal separator;
* the result is expected to be a 32-bit (single precision) IEEE-754
  binary floating-point number.

Some of the speed gain is achieved by only actually parsing at most 9
significant digits of the mantissa into an unsigned 64-bit integer
(using a couple of slightly modified versions of the `fast_parse_uint64`
function above).

Note that while there is no risk of overflow for the mantissa (due to
the cap to 9 decimal digits, which are guaranteed to fit in a 64-bit
number), no overflow check is done for the exponent. This shouldn't be
an issue in practice (since a decimal exponent higher than 38 in
absolute value would result in floating-point overflow, i.e. infinity),
but may lead to misparsing of invalid values.

Currently the only observed discrepancies from the standard library are
in the parsing of some denormals when represented with a very low number
of digits.

### `base10_to_base2_float`

This function converts a combination of unsigned 64-bit integer mantissa
`M` and signed 32-bit integer base-10 exponent `e10` into the closest
32-bit (single precision) IEEE-754 binary floating-point approximatino
to `M*pow(10,e10)`. It is used to produce the final output by
`fast_parse_float32`.
