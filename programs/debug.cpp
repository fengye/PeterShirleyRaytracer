#include "debug.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <net/net.h>
#include <netinet/in.h>

#include <sys/process.h>


static int SocketFD;
static struct sockaddr_in stSockAddr;

extern char g_self_addr[256];

void debug_printf(const char* fmt, ...)
{
#ifdef _PPU_
  char buffer[0x800];
  va_list arg;
  va_start(arg, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, arg);
  va_end(arg);

  char buffer2[0x900];
  snprintf(buffer2, sizeof(buffer2), "[%s] %s", g_self_addr, buffer);

  netSendTo(SocketFD, buffer2, strlen(buffer2), 0, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr));
#endif
}
void debug_init()
{
  netInitialize();
  
  SocketFD = netSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  memset(&stSockAddr, 0, sizeof(stSockAddr));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(DEBUG_PORT);
  // both should work
//  stSockAddr.sin_addr.s_addr = inet_addr(DEBUG_IP_MASK);
  inet_pton(AF_INET, DEBUG_IP_MASK, &stSockAddr.sin_addr);

  int broadcast = 1;
  netSetSockOpt(SocketFD, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

  debug_printf("network debug module initialized, broadcast to %s\n", DEBUG_IP_MASK);
  debug_printf("ready to have a lot of fun\n") ;
}
void debug_return_ps3loadx()
{
  sysProcessExitSpawn2("/dev_hdd0/game/PSL145310/RELOAD.SELF", NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
}
