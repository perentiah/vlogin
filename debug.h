#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG debug_print
#define NF_DEBUG

void debug_print(char *s, ...);

void InitDebug(void);
void ExitDebug(void);

#endif
