# Notes on CHVM Assembly (CHASM)

## Typing

### Types vs Values

In this document a *value* is a piece of data.
A value has a *type* which can be used to determine
how the value is interpreted.
The type of a value can often be used to determine
the __EXACT__ size of the value. However, as we'll see
later, this is not always the case.

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

### Variable Sized Array Type

As we will see later, there is a way to create an array whose
size is unknown at compile time.

Such a value has type `arr<T, ?>`. Note, that fixed size arrays 
also resolve to this type.

```
arr<int, ?>     // An array of integers with unknown length.

arr<int, 3>     // An array of 3 integers. 
                // A value of this type ALSO has type arr<int, ?>.
```

### Dynamic vs Static Data

From this point further, a piece of data managed by the garbage collector
will be known as *Dynamic* whereas a piece of data which resides on the 
call stack will be known as *Static*. 

### Address Types

There will be 2 different ~Address~ types. Values of these types have
a small fixed size, but can be used to reference other values of
any size.

A ~Physical Address~ is a 64-bit reference to any piece of 
data. 

A value of type `*T` is a physical address which references 
a value of type `T`.


A ~Virtual Address~ is a 128-bit reference used to acquire
the physical address of a dynamic piece of data. Such
an address is denoted using the `@` character.

A value of type `@T` is a virtual address which can be used 
to aquire a value of type `*T`.

```
*int x;     // A reference to an integer.
@int y;     // A "virtual" reference to an integer
            // which resides in the garbage collected
            // space.
```
### Dynamic vs Static Data Continued: GC and Memory Safety

Note that in order for the garbage collector to work as expected
it must be aware of all virtual address created by the user.

The garbage collector can only be aware of a virtual address iff
the address itself lives in a dynamic piece of data.

So, if a value contains any virtual addresses, the value must reside
in the garbage collected space. Such a value will thus only be 
accessible via the use of a virtual address.

```
rec { int x; int y; }
```

The above record contains no virtual addresses. Thus a value of
this type can reside on the call stack or the garbage collected space.

```
rec { @arr<chr, ?> name; int age; }
```

This record contains a virtual address referencing an unknown
length. Thus, we can __NEVER__ define a value of this type in
static memory. Instead, we must use a virtual address to reference
a value of this type.

```
// Illegal declaration!!!!
// The virtual refernce held in name will reside in the
// stack and be unknown to the garbage collector.

rec { @arr<chr, ?> name; int age; } student1;

// Acceptable declaration!!!
// 
@rec { @arr<chr, ?> name; int age; } student2;

```







