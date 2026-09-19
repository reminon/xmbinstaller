#ifndef UI_GSKIT_H
#define UI_GSKIT_H
#include <tamtypes.h>

void gskit_ui_init(void);
void gskit_ui_flip(void);
unsigned int gskit_ui_read_press(void);
int gskit_ui_choose_list(const char *title, const char *subtitle,
                   const char **items, int count);

#endif
