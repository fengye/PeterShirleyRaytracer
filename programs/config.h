#ifndef _H_CONFIG_
#define _H_CONFIG_

#define SPU_COUNT (6)
#define PPU_COUNT (6)
#define SPU_PPU_COUNT (12)

#define SPU_CAP (4)
#define PPU_CAP (4)


// multisample
#define SAMPLES_PER_PIXEL (100)
// ray bounce
#define MAX_BOUNCE_DEPTH (50)

// Networking
#define SERV_PORT (21210)
#define LISTENQ   (1024)   /*  Backlog for listen()   */

#endif //_H_CONFIG_ 