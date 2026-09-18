#ifndef ISO_READER_H
#define ISO_READER_H

/* Extract game ID from ISO's SYSTEM.CNF BOOT2 line.
   Returns 0 on success, negative on error.
   id_out should be at least 16 bytes. */
int iso_get_game_id(const char *iso_path, char *id_out, int id_max);

#endif
