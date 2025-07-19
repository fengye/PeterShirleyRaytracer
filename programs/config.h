#ifndef _H_CONFIG_
#define _H_CONFIG_

#define SPU_COUNT (6) // <- set to 0 to disable SPU computing
#define PPU_COUNT (6) // probably can squeeze a bit more than 2
#define SPU_PPU_COUNT (SPU_COUNT+PPU_COUNT)

#define SPU_CAP (4) // how many jobs a node's SPU can take at a time
#define PPU_CAP (4) // how many jobs a node's PPU can take at a time


// multisample
#define SAMPLES_PER_PIXEL (100)
// ray bounce
#define MAX_BOUNCE_DEPTH (50)

// Networking
#define SERV_PORT (21210)
#define LISTENQ   (1024)   /*  Backlog for listen()   */

// FORCE DISCARD SLAVE JOIN FOR FAST ITERATION
#define DEBUG_IGNORE_SLAVE_JOIN 0

// Set DEBUG_STOP_SPU_BY_TERMINATION to 1 when SPU is crashing
#define DEBUG_STOP_SPU_BY_TERMINATION 0

#endif //_H_CONFIG_ 