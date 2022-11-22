#ifndef CHVM_ARC_H
#define CHVM_ARC_H

// chvm architecture specs!

// The chvm will seek to minimize all runtime checks.
// This will be done by off loading lots of common organizational
// design decisions onto compilers and users of the chvm.
//
// The chvm will implement some form of garbage collection.
// So, thus user will never need to worry about explicit memory
// management while using the chvm.
// 
// However garbage collection is implemented, it will
// most certainly rely on the following memory structuring
// rules of the chvm.
//
// The chvm will have the 3 following types.
//
// addr : 64-bit address.
// quad : 64-bit value.
// byte : 8-bit value.
//
// An addr will either be 0 (NULL) or the value of the address
// of a valid memory block.
//
// A valid memory block will look as follows :
//
// Block :
//      quad rt_len             (rt for reference table)
//      addr rt[rt_len - 1]           
//      quad da_len             (da for data array)
//      byte da[da_len]
//
// A block is broken into two areas: the reference table and
// the data array. This is done so that the garbage collector will
// always be able to identify references held inside a block
// regardless of what the block represents.
//


#endif
