

// What we have : 
//
// We have a memory space which has background garbage collection.
//
// Each object we make can only be read/written to when the
// corresponding lock is held on said object.
//
// Because of how the garbage collector works, we cannot
// hold an object lock for an indefinite amount of time.
//
// The concurrent garbage collector has laid most of the 
// ground work for allowing user spawned threads.
//
// Data Types and Organization :
//
// The chvm will respect the following data types...
//
// * byte (character)               : byte
// * signed 64-bit integer          : integer
// * 64-bit floating point number   : fraction 
// * Virtual Address    (16-byte)   : virtual
// * Physical Address   (8-byte)    : physical
//
// Lots of things to consider here...
// How should dynamic objects be passed around without being GC'd?
// Should procedure pointers be their own type???
// How will structures look in memory???
//
// Structure and Union Types???
// Can procedures be fields???
// Can types be fields???
// This is something to think about???
//
// A procedure could provide its :
// * Name
// * Number of arguments
// * Types of each argument
// * Number of additionaly local variables
// * Types of each local variable.
// * Return Type.
// * Length of text segment
// * text segment
//
// A procedure is called how???
// What are the text operations???
// Is there a call stack??
// Where will dynamic object creation lie???
// 
// Should arguments be given names???
// I would say yes!! VM then decides how to handle them???
//
// Maybe a procedure could have a name and a specific argument
// type it takes??? Then the user builds the corresponding argument
// and passes it to the procedure?? I actually do like this personally!
// Procedures could describe what arguments they need and then also how
// much local storage space the need on the stack??? (To run that is)
//
//
//
//
// 
// A user spawned thread will be given the following resources :
//
// * A Data Array useable for arbitrary arithmetic. 
// * A Table with reference slots. Each slot will hold a virtual
// address and a physical address. 
//
// * A Stack to be used as specified by the user? (Should function calling be built in?)
// (I think yes tbh)
//
// Procedure Call Protocal :
// 
// The chvm will have a notion of procedures. It will handle all
// book-keeping to make calling procedures and returning from procedures possible.
//
// The memory given to a procedure need not be in the collected space.
// As its lifespan is definite. Additionally, its data is never shared
// with other threads.
//
// A user defined procedure will specify how much space it needs for local
// variables. When calling the procedure said stack frame will be allocated
// outside of the collected space.
//
// How will arguments be passed?
//
// 
// 
//
//
//
