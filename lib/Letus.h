#ifndef _LETUS_H_
#define _LETUS_H_


typedef struct  Letus Letus;
extern struct Letus *OpenLetus();
void LetusPut(Letus* p, const char* key_c, const char* value_c);
char* LetusGet(Letus* p, const char* key_c);

#endif // _LETUS_H_