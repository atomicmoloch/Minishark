#ifndef MAC_DATABASE_H
#define MAC_DATABASE_H

#include <stdint.h>

typedef struct {
   uint64_t prefix;
   uint8_t length;
   uint16_t man_id;
} oui_entry_t;

extern const char *manufacturers[];
extern const oui_entry_t oui_table[];
extern const size_t oui_table_length;

#endif
