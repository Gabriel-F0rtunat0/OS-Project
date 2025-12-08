#include "parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static char *load_file_into_memory(int fd, ssize_t *out_size)
{
    // Checks if the file has an end and gets the size of the file as an offset value
    off_t size = lseek(fd, 0, SEEK_END); // POSIX type for files manipulation
    if (size <= 0)
    {
        // TODO:
        return NULL;
    }

    // Reallocate the file descriptor of the open file to a offset bytes value
    lseek(fd, 0, SEEK_SET);

    // Saves a place in the memory using dynamic memory allocation of size size
    char *buffer = malloc(size + 1); // Saves a byte for the null terminator
    if (!buffer)
    {
        // TODO:
        return NULL;
    }

    // Tries to read size bytes of the open file onto the buffer and returns the value of bytes effectively read
    ssize_t n = read(fd, buffer, size);
    if (n != size)
    {
        free(buffer);
        // TODO:
        return NULL;
    }

    // Set the null terminator
    buffer[size] = '\0';
    if (out_size)
        *out_size = size;
    return buffer;
}

static int fast_atoi(const char **p)
{
    int v = 0; // Local variable that saves the value pointed by p
    while (**p >= '0' && **p <= '9')
    {                             // **p is the current character (digit) in the string pointed to by *p
        v = v * 10 + (**p - '0'); // Define each value of each unit digit
        (*p)++;                   // Goes to the next value of the array
    }
    return v;
}

static void skip_spaces(const char **p)
{
    while (**p == ' ' || **p == '\t')
        (*p)++; // Skips every space and tabs charaters
}

static void read_word(const char **p, char *out)
{
    int i = 0;
    while (**p > ' ' && **p != '\0' && **p != '\n' && i < BUFFER_SIZE)
    { //
        out[i++] = **p;
        (*p)++;
    }
    out[i] = '\0';
}

static void handle_DIM(const char *line, board_t *board, board_pos_t **iter_board)
{
    skip_spaces(&line); // Goes to the first word of the line
    board->width = fast_atoi(&line);
    skip_spaces(&line); // Goes to the next word of the line
    board->height = fast_atoi(&line);

    board->board = calloc(board->width * board->height, sizeof(board_pos_t)); // Allocates memory for the board
    *iter_board = board->board;                                               // Associates a pointer to the board_pos_t structure. It will help to iterate the board without messing the stored data
}

static void handle_PAC(const char *line, board_t *board, int points, const char *dir_path)
{
    skip_spaces(&line); // Goes to the first word of the line

    board->n_pacmans++; // Adds a pacmant to the board

    char filename[MAX_FILENAME]; // Defines a vector to save the filename
    read_word(&line, filename);  // Gets the filename

    snprintf(board->pacman_file, MAX_FILENAME, "%s%s", dir_path, filename); // Saves the file path of a particular pacman

    if (board->n_pacmans == 1)
        board->pacmans = malloc(sizeof(pacman_t)); // Allocates memory to store all the data related to the pacman_t structure

    pacman_t *pacman = &board->pacmans[0]; // Associates a pointer to the pacman_t structure that exists in the board_t structure
    pacman->alive = 1;
    pacman->points = points;
    pacman->current_move = 0;
    pacman->n_moves = 0;
    pacman->waiting = 0;

    // To be initialized:
    // pacman->pox_x
    // pacman->pox_y
    // pacman->passo
    // pacman->moves

    handler_t handler = {PACMAN, (void *)pacman};
    int fd = open(board->pacman_file, O_RDONLY); // System call to open the file specified by the pathname
    if (fd >= 0)
        file_parser(fd, board, points, &handler, (char *)dir_path); // Parse the opened file. Cast the const char* to a char*
}

static void handle_MON(const char *line, board_t *board, int points, const char *dir_path)
{
    while (board->n_ghosts < MAX_GHOSTS && *line != '\0')
    {
        char filename[MAX_FILENAME]; // Defines a vector to save the filename

        skip_spaces(&line); // Goes to the first/next filename of the line

        read_word(&line, filename); // Gets the filename

        snprintf(board->ghosts_files[board->n_ghosts], MAX_FILENAME, "%s%s", dir_path, filename); // Saves the file path of a particular ghost

        board->n_ghosts++; // Adds one ghost to the board
    }

    board->ghosts = calloc(board->n_ghosts, sizeof(ghost_t)); // Allocates memory to store all the data related to the ghost_t structures

    for (int i = 0; i < board->n_ghosts; i++)
    {                                       // Iterates the ghosts array of the board structure
        ghost_t *ghost = &board->ghosts[i]; // Associates a pointer to the i'th ghost
        ghost->current_move = 0;
        ghost->n_moves = 0;
        ghost->waiting = 0;
        ghost->charged = 0;
        // FALTA ALGUNS ELEMENTOS ESTRUTURAIS

        handler_t handler = {GHOST, (void *)ghost};
        int fd = open(board->ghosts_files[i], O_RDONLY); // System call to open the file specified by the pathname
        if (fd >= 0)
            file_parser(fd, board, points, &handler, (char *)dir_path); // Parse the opened file. Cast the const char* to a char*
    }
}

static void handle_TEMPO(const char *line, board_t *board)
{
    skip_spaces(&line);

    board->tempo = fast_atoi(&line);
}

static void handle_board_row(const char *line, board_t *board, board_pos_t **iter_board)
{
    for (int i = 0; i < board->width; i++)
    {                    // Goes to each position of a specific line of the board
        board_pos_t pos; // Instanciate a board_pos_t structure to save information about that position

        switch (line[i])
        {
        case 'X':
            pos = (board_pos_t){'W', 0, 0};
            break;
        case 'o':
            pos = (board_pos_t){' ', 1, 0};
            break;
        case '@':
            pos = (board_pos_t){' ', 0, 1};
            break;
        }

        if ((**iter_board).content != PACMAN && (**iter_board).content != GHOST)
            **iter_board = pos; // Associates the current position to a board_pos_t structure
        (*iter_board)++;        // Goes to the next position of the line
    }
}

static void handle_POS(const char *line, board_t *board, handler_t *handler)
{
    int pos_x, pos_y;
    skip_spaces(&line);
    pos_x = fast_atoi(&line);
    skip_spaces(&line);
    pos_y = fast_atoi(&line);

    if (handler->type == PACMAN)
    {
        pacman_t *pacman = (pacman_t *)handler->ptr;
        pacman->pos_x = pos_x;
        pacman->pos_y = pos_y;
    }
    else if (handler->type == GHOST)
    {
        ghost_t *ghost = (ghost_t *)handler->ptr;
        ghost->pos_x = pos_x;
        ghost->pos_y = pos_y;
    }

    board_pos_t *pos = &board->board[pos_y * board->width + pos_x];
    pos->content = handler->type;
    pos->has_dot = 1;
    pos->has_portal = 0;
}

static void handle_PASSO(const char *line, handler_t *handler)
{
    skip_spaces(&line);

    if (handler->type == PACMAN)
    {
        pacman_t *pacman = (pacman_t *)handler->ptr;
        pacman->passo = fast_atoi(&line);
    }
    else if (handler->type == GHOST)
    {
        ghost_t *ghost = (ghost_t *)handler->ptr;
        ghost->passo = fast_atoi(&line);
    }
}

static void handle_MOV(const char *line, handler_t *handler)
{
    command_t command = {0, 0, 0};

    skip_spaces(&line);

    command.command = *line;

    if (command.command == 'T')
    {
        line++;
        skip_spaces(&line);
        command.turns = command.turns_left = fast_atoi(&line);
    }

    if (handler->type == PACMAN)
    {
        pacman_t *pacman = (pacman_t *)handler->ptr;
        pacman->moves[pacman->n_moves] = command;
        pacman->n_moves++;
    }
    else if (handler->type == GHOST)
    {
        ghost_t *ghost = (ghost_t *)handler->ptr;
        ghost->moves[ghost->n_moves] = command;
        ghost->n_moves++;
    }
}

void file_parser(int fd, board_t *board, int points, handler_t *handler, char *dir_path)
{
    ssize_t size = 0;                              // Stores the size of the file in bytes
    char *file = load_file_into_memory(fd, &size); // Loads the file information into a static memory
    close(fd);                                     // Closes the file descriptor

    if (!file)
        return; // If the file does not have nothing inside it returns

    board_pos_t *iter_board = NULL; // Instanciate and initializes a pointer to iterate every position in the board

    if (handler->type == LEVEL)
    {                                                                        // Initialize the first level or reset the following levels
        board->level++;                                                      // Adds one level
        snprintf(board->level_name, BUFFER_SIZE, "%d", board->level); // Changes the level name

        board->width = board->height = 0;
        board->board_pos = 0;
        board->tempo = 0;
        board->n_pacmans = 0;

        memset(board->pacman_file, 0, MAX_FILENAME); // Sets every byte of the pacman file to 0

        for (int i = 0; i < board->n_ghosts; i++)
            memset(board->ghosts_files[i], 0, MAX_FILENAME); // Sets every byte of every ghost file to 0

        board->n_ghosts = 0;
    }

    char *saveptr;                                 // Saves the rest of the stream
    char *line = __strtok_r(file, "\n", &saveptr); // Gets the token corresponding to a line

    while (line)
    {
        if (line[0] != '#')
        {
            if (handler->type == LEVEL)
            {
                if (strncmp(line, "DIM", 3) == 0)
                    handle_DIM(line + 3, board, &iter_board);
                else if (strncmp(line, "PAC", 3) == 0)
                    handle_PAC(line + 3, board, points, dir_path);
                else if (strncmp(line, "MON", 3) == 0)
                    handle_MON(line + 3, board, points, dir_path);
                else if (strncmp(line, "TEMPO", 5) == 0)
                    handle_TEMPO(line + 5, board);
                else
                    handle_board_row(line, board, &iter_board);
            }
            else
            {
                if (strncmp(line, "POS", 3) == 0)
                    handle_POS(line + 3, board, handler);
                else if (strncmp(line, "PASSO", 5) == 0)
                    handle_PASSO(line + 5, handler);
                else
                    handle_MOV(line, handler);
            }
        }

        line = __strtok_r(NULL, "\n", &saveptr); // Gets the next token
    }

    if (handler->type == LEVEL && board->n_pacmans == 0)
    {
        board->pacmans = malloc(sizeof(pacman_t));
        load_pacman(board, points);
    }
}
