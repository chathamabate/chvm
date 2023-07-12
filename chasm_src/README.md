# Notes on CHVM Assembly (CHASM)

## Typing

There are 4 primitive types :
  * `int`  A signed 64-bit integer.
  * `flt`  A 64-bit floating point number.
  * `idx`  A 32-bit unsigned integer. (Used for offsets into arrays)
  * `chr`  A 8-bit character/byte.

All remaining types are built from these types.

An array or `arr` is a composite value containing a fixed length sequence of
identically typed values.

```
arr<int, 5>             // An array of 5 integers.
arr<arr<flt, 3>, 3>     // A 3x3 matrix of floats.
```

A record or `rec` is a value composed of named values.
The types of these values can be heterogeneous.

```
rec {
    int x;
    int y;
}

rec {
    arr<char, 6> id;

    rec {
        int x;
        int y;
    } coordinate;
}
```
