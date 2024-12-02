#ifndef _LETUS_H_
#define _LETUS_H_


typedef struct  Letus Letus;
extern struct Letus *OpenLetus();
void LetusPut(Letus* p);
char* LetusGet(Letus* p);

#endif // _LETUS_H_