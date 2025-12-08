#ifndef FILE_H
#define FILE_H

#include "board.h"

#define BUFFER_SIZE 256
#define LEVEL 'L'
#define GHOST 'M'
#define PACMAN 'P'

typedef struct {
    char type;
    void *ptr;
} handler_t;

/**
 * @brief Parses a level or entity file and updates the board accordingly.
 *
 * Reads the entire contents of @p fd into memory, splits it into lines and,
 * based on @p handler->type (LEVEL, PACMAN or GHOST), updates the fields of
 * @p board and the entity pointed to by @p handler->ptr.
 *
 * The file descriptor @p fd is always closed inside this function.
 *
 * @param fd       File descriptor opened for reading.
 * @param board    Pointer to the game board structure to be updated.
 * @param points   Base score used when initializing pacman data from a level file.
 * @param handler  Pointer to a ::handler_t describing what to parse and which entity to fill.
 * @param dir_path Base directory path for resolving referenced files.
 *
 * @note This documentation comment was generated with the assistance of an AI system.
 */
void file_parser(int fd, board_t* board, int points, handler_t *handler, char* dir_path);

#endif