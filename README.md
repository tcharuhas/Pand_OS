 # PandOS 

The PandOS operating system is based off the T.H.E. system out-lined by Dijkstra back in 1968. Dijkstra’s paper described an OS divided into six layers. Each layer was an ADT or abstract machine to layer i+ 1; successively building up the capabilities of the system for each new layer to build upon. The operating system also contains multiple layers, though PandOS is not as complete as Dijkstra’s. PandOS is split into 3 key phases with room for more additions.  High level overview of the phases: 

## Phase 1 - The Queues Manager
Based on the key operating systems concept that active entities at one layer are just data structures at lower layers, this layer supports the management of queues of structures: pcbs.  

# Phase 2 - The Kernel 
This level implements eight new kernel-mode process management and synchronization primitives in addition to multiprogramming, a process scheduler, device interrupt handlers, and deadlock detection.  

# Phase 3 - The Support Level 
This level is extended to support a system that can support multiple user-level processes that each run in their own virtual address space. Furthermore, support is provided to read/write to character-oriented devices.  


# Phases under construction:
-------------------------------------- 

# Phase 4 - DMA Devices 
An extension of Phase 3 providing I/O support for DMA devices: disk drives and flash devices. Furthermore, this level implements a more realistic backing store implementation.  

# Phase 5 - The Sleep/Delay Facility 
This level provides the Support Level with a sleep/delay facility

# Phase 6 - Cooperating User Processes 
This level introduces a shared memory space and user-level synchronization primitives to facilitate cooperating processes. 

# Phase 7 - The File System 
This level implements the abstraction of a flat file system by implementing primitives necessary to create, rename, delete, open, close, and modify files.
