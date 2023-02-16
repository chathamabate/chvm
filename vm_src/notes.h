

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
//
//
