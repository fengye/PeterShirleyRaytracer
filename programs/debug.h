#ifndef _H_DEBUGPRINT_
#define _H_DEBUGPRINT_


// This IP range should cover your home network
#define DEBUG_IP_MASK "192.168.20.255"
#define DEBUG_PORT 18194

extern void debug_printf(const char* fmt, ...);
extern void debug_init();
extern void debug_return_ps3loadx();

#endif //_H_DEBUGPRINT_