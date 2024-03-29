# Notes on CHVM Assembly (CHASM)

- [Notes on CHVM Assembly (CHASM)](#notes-on-chvm-assembly-chasm)
  - [Typing and Memory](#typing-and-memory)
    - [Types vs Values](#types-vs-values)
    - [Primitive Types](#primitive-types)
    - [Simple Composite Types](#simple-composite-types)
    - [Variable Sized Array Type](#variable-sized-array-type)
    - [Functions and Value Declarations](#functions-and-value-declarations)
    - [Dynamic vs Static Data](#dynamic-vs-static-data)
    - [Address Types](#address-types)
    - [Dynamic vs Static Data Continued](#dynamic-vs-static-data-continued)
    - [Function Types](#function-types)
    - [The `new` Keyword](#the-new-keyword)
    - [Value Paths and Dereferencing Physical Addresses](#value-paths-and-dereferencing-physical-addresses)
    - [Physical Address Safety: `acquire` and `ref`](#physical-address-safety-acquire-and-ref)
    - [Virtual Address Safety and `vnull`](#virtual-address-safety-and-vnull)


## Typing and Memory

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

All remaining types are built from these types.

### Simple Composite Types

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

### Functions and Value Declarations

To define a function, the user must provide its return type, name, parameter types,
and parameter names.

```
fun my_function(P1 param1, P2 param2, P3 param3, ...) => R {
    // Function Body
}

```

In the above declaration, `my_function` is the name of the function.
`param1, param2, param3, ...` are the names of the parameters.
`P1, P2, P3, ...` are the types of the parameters.
`R` is the return type of the function.

Note, if `=> R` is omitted, the function will have no return type and
thus return no value.

A function body will be able to preform many operations. 

For now, we just need to know how to declare a value in the function.
This is done by specifying the type and name of a value.

```
fun my_function(int x, int y) => int {
    // Declaring an integer with name z.
    int z;

    // Declaring a record with a minutes and
    // seconds field.
    rec { int minutes; int seconds; } time;
}

```

We can assign values using the `$` operator.
For example if we want to give `z` a value in the above function,
we can write `z $ 4;`. 

### Dynamic vs Static Data

When calling a function, a piece of memory will be allocated known as
the stack frame. A stack frame will contain a piece of memory which __IS__
managed by the garbage collector and a piece of
memory which __IS NOT__ managed by the garbage collector.

From this point further, a piece of data managed by the garbage collector
will be known as *Dynamic* whereas all other data will be known as *Static*.

### Address Types

There will be 2 different *Address* types. Values of these types have
a small fixed size, but can be used to reference other values of
any size.

A *Physical Address* is a 64-bit reference to any piece of 
data. 

A value of type `*T` is a physical address which references 
a value of type `T`.


A *Virtual Address* is a 128-bit reference used to acquire
the physical address of a dynamic piece of data. Such
an address is denoted using the `@` character.

A value of type `@T` is a virtual address which can be used 
to aquire a value of type `*T`.

```
*int x_phy;     // A "physical" reference to an integer.

@int y_vrt;     // A "virtual" reference to an integer
                // which resides in the garbage collected
                // space.
```

### Dynamic vs Static Data Continued

Note that in order for the garbage collector to work as expected
it must be aware of all virtual addresses created by the user.

The garbage collector can only be aware of a virtual address iff
the address itself lives in a dynamic piece of data.

Thus, when a user calls a function, the function will place all
virtual address values in the dynamic part of its stack frame, and
all other values in the static part of its stack frame.

```
func example() {
    int x;  // This value will reside in static memory and does
            // not need to be known by the garbage collector.
    
    @int y_vrt;     // This value will reside in dynamic memory as
                    // it MUST be known by the garbage collector.

    rec {
        @arr<chr, ?>    name_vrt;
        int             grade;
    } student;

    // The value of student will be divided into static and dynamic pieces.
    // Name will be placed in the garbage collected space, whereas grade
    // can stay in the static part of the stack frame.
}
```

### Function Types

While creating a new function dynamically is not currently supported,
you can still pass around references to defined functions.

```
fun mystery(int x, int y) => flt { ... }
```
The function `mystery` can be passed to another function, and has
the type `fun (int, int) => flt`.

```
fun normal_call(fun (int, int) => flt f) {
    flt x $ f(1, 2); // This is OK!
    
    ...
}

fun do_something() {
    // This is OK!
    fun (int, int) => flt f $ mystery;

    // Also OK!
    normal_call(f);
}
```
In the above `do_something` function, `f` has a function type.
While the value of `f` technically is an address, `f` is not considered
virtual or physical. Its value will either reference a piece of user code
or be `fnull` (for function null).

A value with a function type can reside anywhere! 

You can place function types in records and arrays, and it doesn't matter
whether a value of a function type is in dynamic or static memory!

### The `new` Keyword

`chasm` provides an operator for allocating garbage collected memory.
This operator is `new`, and will always return a virtual address.

```
new int;    // Creates a dynamic integer.
// Returns a value of type @int.

new rec { flt x; flt y; };   // Creates a dynamic coordinate.
// Returns a value of type @rec { flt x; flt  y; }.
```

`new` can also be used to create arrays of constant and non-consant sizes.

```
@arr<int, 4> const_arr $ new arr<int, 4>;

int len $ ... // Runtime value.

@arr<int, ?> variable_arr $ new arr<int, len>;
```

As seen above, we can pass an integral variable into the array allocation.
If this is done, the type returned will always be `@arr<int, ?>` as the 
length of the new array may not be known at compile time.

### Value Paths and Dereferencing Physical Addresses

A *Value Path* is text in an assembly file which represents a value.
Anywhere in the assembly file where a value is expected, a *Value Path* must
be given.

```
int x; int y;
rec { int z; } w;

x $ y;      // y is the value path used to get the integer value of y.
y $ w.z;    // w.z is the value path used to get the integer value
            // held in w.

w.z $ 5;    // constants are also value paths!
            // Note the left side of an assignment is also a value
            // path!
```

Each assembly operation will loosely coorespond to one or more bytecode 
instructions. Due to the structure of the bytecode, there are restrictions
on the structure of a singular value path.

As we explore value paths, we will also investigate how to gain access
to a value given its physical address. This is done using the `!` and 
`![]` operators.

If `x` has a value of type `*int` (A physical address of an integer), 
`x!` will have a value of type `int`. `x!` is used to represent
the value of the integer which `x` *points* to.

If `a` has type `*arr<int, ?>`, `a![i]` will have type `int` and represent 
the integer value at index `i` in the array pointed to by `a`.

Lastly `!` can be used to traverse fields when an address of a record is held.
If `p` has type `rec {int x; int y;}`, then `p!y` has type `int` and represents the
value in field `y` of the record pointed to by `p`. (Note that this is analagous to
the `->` operator in C)

Anyway, now let's talk about paths.

A *Constant Path* is a path whose relative location
can be determined at compile time. Such a path is only composed of
local fields and local constant array indexes.

```
x.y.z       // Accesing fields with the "dot" operator.
a.[4].x     // Using a "dot-index" with a constant.

a.b.c.[14].x.[2]

// All of the above value paths are constant paths.
```

A *Simple Path* contains only one physical address dereference.
That is it follows one of the following rules.
 * *Constant Path* ! 
 * *Constant Path* ! *Constant Path*
 * *Constant Path* .[ *Constant Path* ] *Constant Path*
 * *Constant Path* ![ *Integer Constant* ] *Constant Path*
 * *Constant Path* ![ *Constant Path* ] *Constant Path*

```
x.y!z.a
x.y.[z].r.p.[3]
q.r.[3]![i.j].[3]

a![23]  // Note that even though the index is a constant,
        // the use of the "!" operator forces this value path
        // to dereference the address stored in "a" at runtime.
        // Thus, this path is simple, not constant.

a.[i]
```
A *Complex Path* contains a dereference which
itself contains a *Simple Path*.
See the following rules.
 * *Constant Path* .[ *SimplePath* ] *Constant Path*
 * *Constant Path* ![ *SimplePath* ] *Constant Path*

```
x.y.z.[r!s].t.[3]
p.t![a![1]]
a.b![q![n]].x
```

Finally, a *Value Path* can be any of these types or a constant.

```
34          // Constant.

x.y         // Constant Path.

x.[n].z     // Simple Path. 
x![i.[32]]  // Simple Path.
y!k.j       // Simple Path.

k![i!j].j                 // Complex Path.
k.[45].x.a![p.k![n].f].l  // Complex Path.

// 2 or more dereferences which are not nested
// are not allowed!
x!y!z       // NOT OK
x.[n].[i]   // NOT OK
x.[k]!a     // NOT OK

// Nesting dereferences deeper than one level is not allowed!
x.[y.[z]]           // This is OK!
x.[y![p.[34]]]      // This is OK!      (p.[34] is a Constant Path)
x![y![i]]           // Also OK!

x.[y.[z.[i]]]       // This is NOT OK!  (z.[i] is not a Constant Path)
x![a![b!z]]         // This is NOT OK!
```

For a more rigorous definition of these paths, look at the `grammar.txt` file
in this directory. This file defines the exact syntax rules the assembler
follows.

### Physical Address Safety: `acquire` and `ref`

The presence of physical addresses introduces the potential for danger!

For example, what if a function call returns the physical address of a value which 
lives in the static part of the function call's stack frame?

On return, the static part of the stack frame will be deleted. The physical address returned
will point to a deleted value!

Similarly, what if we store the physical address of a dynamic value without telling
the garbage collector? The garbage collector only keeps track of virtual addresses.
It may delete our dynamic value without realizing we still have a way of accessing it.
We will once again have a pointer to a deleted value!

So, there will be a few restrictions on how physical addresses can be used
to gaurantee the above problems (and others) never can happen.

Most Notably, the value of a physical address can never be assigned using the `$`
operator. 

```
*int x;
*int y;

x $ y;  // This is NOT OK!
```

So, if this is the case, how do we assign a value to a physical address?

There are 2 ways.

First is using an `acquire` block. This construct is used to notify the garbage
collector that we will be working on a piece of garbage collector memory and that we
would like the piece's lock. The lock can be acquired in `write` or `read` mode.

```
@int x $ new int;
*int x_p;

acquire write x into x_p {
   x_p! $ 10;   // This is OK!
}
```

When exiting an `acquire` block, the corresponding lock and physical address are
both given up. For example, in the above code `x_p` only holds a value in
the acquire block. Attempting to use `x_p!` outside the `acquire` block will
result in a compile time error.

The second way is using the `ref` keyword. This keyword is made so arbitrary 
references can be passed to functions. There is no reason physical addresses
need only be used to access memory in the garbage collected space.

If the value path `vp` corresponds to a value of type `T`, then `ref vp` has a 
type of `*T` and is the physical address of `vp`'s corresponding value.
`ref` can only ever be used when passing an argument to a function.

```
// Function for swapping integers.
fun swap(*int x, *int y) {
    int temp $ x!;
    x! $ y!;
    y! $ temp;
}

fun example() {
    int num1 $ 1;
    int num2 $ 2;

    swap(ref num1, ref num2); 

    // NOTE, we don't need to use ref to pass a physical address
    // to a function. Passing a physical address from an 
    // acquire block works all the same.

    @int num3 $ new int;
    *int num3_p;

    acquire write num3 into num3_p {
        num3_p! $ 3;
        swap(num3_p, ref num2);
    }
}
```

By restricting physical addresses in this way, the assebler will
be able to confirm safe physical address use at compile time!

Note that while the assembler will allow the user to define potentially unsafe
types and functions, the physical address restrictions will prevent 
all possible misuse.

Here is an example.

```
fun bad_return() => *int {
    ...
}
```

The user can define a function which returns a physical address.
However, due to the assembler checks, it is impossible for the
return value of this function ever to be used since it cannot be
assigned using `$`. We will see that the return value of a function
can only be used iff it is immediately assigned.

### Virtual Address Safety and `vnull`

Since we know when a physical address is valid and invalid at compile time
we can prevent its misuse easily!

Is this the same with virtual addresses?

Well, I have decided that it is ok for a virtual address to hold a null value 
(specified by the keyword `vnull`. There will be no compile time checks for this.

The only restriction is that when the physical address of `vnull` is aquired, 
a run time error occurs and the program exits.

```
@int x_vrt;     // Default value of vnull.
*int x_phy;

aquire write x_vrt as x_phy {
    // ERROR!!!
    // Attempting to acquire vnull!
}
```

It is totally ok for functions to return virtual addresses (including `vnull`).

```
fun my_func1() => @int {
    return new int;     // OK!
}

fun my_func2() => @int { 
    return vnull;       // Also OK!
}
```








