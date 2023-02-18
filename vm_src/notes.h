

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
// Memory Organization (Local vs Dynamic Memory) :
//
// An "object" is a piece of data stored in dynamic memory.
// All objects come with a read/write lock which must be used
// while accessing the object.
// All objects are refered to using a "virtual address". This identifier
// can be used to acquire an object's instantaneous location in memory.
// All objects have a "reference table" of virtual addresses.
// All objects have a "data array" of free use bytes.
//
// All objects are subject to garbage collection and memory shifting.
// If an object stores a reference to another object, the virtual address
// must be stored in the reference table of the inital object. 
// The GC algorithm depends on this constant.
//
// The benefit of using objects is that they can have dynamic size
// and indefinite life spans.
// The cost of using objects is needing to acquire their locks 
// during every use.
//
// The chvm should have a notion of a call stack.
// This will be used when functions are called.
// Functions should have a notion of "local memory".
// This will be memory which is allocated when a function is called
// and freed when a function returns.
// This memory will only be accessible by the thread which called the function.
//
// This memory should be unknown to the GC algorithm. Thus, allowing
// the user to use said memory without acquiring a lock.
//
// Prim Data Types :
// 
// 8-bit integer                : byte
// 64-bit signed integer        : int
// 64-bit floating point number : float
// 64-bit physical address      : ptr
// 64-bit function pointer      : func
//
// 128-bit virtual address      : ref
//
// There will have to be rigorous compile time checking to verify 
// how ptr's and func's are used.
//
// Also, there will be structures which hold any combination of 
// the types above.
//
// A function will need to describe the arguments it receives and the
// data it needs. (Maybe this can be part of the instructions???)
//
// The byte code should contain enough information to output
// relevant error code information regardless of what language was
// compiled to the byte code? Maybe there is a way around this??
// Like there could be a specific file for piping low level error
// information to higher level error information the user an interpret?
//
// Maybe I should make a video for the garbage collector before I move any further???
//
//
// 
//
//
// Current Issue :
// How will object creation be weaved into the local memory of function
// calls. For example, can I store a virtual address in a local variable?
// If so, How will the Garbage Collector know not to free said object?
//
// Maybe virtual addresses cannot be stored in normal local memory of
// the function call? There could be a separate area for virtual addresses
// that never gets GC'd?
//
// Could there be a stack of function reference tables??
//
//
//
// 
//
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
