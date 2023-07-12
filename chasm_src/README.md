# Notes on CHVM Assembly (CHASM)

## Typing

### Types vs Values
In this document a *value* is a piece of data.
A value has a *type* which can be used to determine
how the value is interpreted.
The type of a value can often be used to determine
the __EXACT__ size of the value. However, as we'll see
later this is not always the case.

### Primitive Types

There are 4 primitive types :
  * `int`  A signed 64-bit integer.
  * `flt`  A 64-bit floating point number.
  * `idx`  A 32-bit unsigned integer. (Used for offsets into arrays)
  * `chr`  A 8-bit character/byte.

### Simple Composite Types

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

### Dynamic vs Static Data

From this point further, a piece of data managed by the garbage collector
will be known as "Dynamic" whereas a piece of data which resides on the 
call stack will be known as "Static". 

### Address Types


