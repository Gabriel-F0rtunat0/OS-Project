#include "board.h"
#include "display.h"
#include "parser.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#define CONTINUE_PLAY 0
#define NEXT_LEVEL 1
#define QUIT_GAME 2
#define LOAD_BACKUP 3
#define CREATE_BACKUP 4
#define BUF_SIZE 1024

void screen_refresh(board_t *game_board, int mode)
{
    debug("REFRESH\n");
    draw_board(game_board, mode);
    refresh_screen();
    if (game_board->tempo != 0)
        sleep_ms(game_board->tempo);
}

int play_board(board_t *game_board)
{
    pacman_t *pacman = &game_board->pacmans[0];
    command_t *play;
    if (pacman->n_moves == 0)
    { // if is user input
        command_t c;
        c.command = get_input();

        if (c.command == '\0')
            return CONTINUE_PLAY;

        c.turns = 1;
        play = &c;
    }
    else
    { // else if the moves are pre-defined in the file
        // avoid buffer overflow wrapping around with modulo of n_moves
        // this ensures that we always access a valid move for the pacman
        play = &pacman->moves[pacman->current_move % pacman->n_moves];
    }

    debug("KEY %c\n", play->command);

    if (play->command == 'Q')
    {
        return QUIT_GAME;
    }

    if (play->command == 'G')
    {
        
        return CREATE_BACKUP;
    }

    int result = move_pacman(game_board, 0, play);
    if (result == REACHED_PORTAL)
    {
        // Next level
        return NEXT_LEVEL;
    }

    if (result == DEAD_PACMAN)
    {
        if (game_board->saved == 1) return LOAD_BACKUP;
        return QUIT_GAME;
    }

    for (int i = 0; i < game_board->n_ghosts; i++)
    {
        ghost_t *ghost = &game_board->ghosts[i];
        // avoid buffer overflow wrapping around with modulo of n_moves
        // this ensures that we always access a valid move for the ghost
        move_ghost(game_board, i, &ghost->moves[ghost->current_move % ghost->n_moves]);
    }

    if (!game_board->pacmans[0].alive)
    {
        return QUIT_GAME;
    }

    return CONTINUE_PLAY;
}

void saves_progress(board_t *board) {
    if (board->saved == 0){
            board->saved = 1;
            pid_t pid = fork();
            if (pid > 0) {
                waitpid(pid, NULL, 0);
                board->saved = 0;
            }
        }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <level_directory>\n", argv[0]);
        return 1;
    }

    // Random seed for any random movements
    srand((unsigned int)time(NULL));

    open_debug_file("debug.log");

    terminal_init();

    int accumulated_points = 0;
    bool end_game = false;
    board_t game_board;
    char lvl_filename[MAX_FILENAME];

    DIR *dir = opendir(argv[1]);
    struct dirent *dir_info;
    game_board.saved = 0;

    

    while ((dir_info = readdir(dir)) != NULL)
    {
        char *extension;
        if ((extension = memchr(dir_info->d_name, '.', MAX_FILENAME)) != NULL)
        {
            if (strcmp(extension, ".lvl") == 0)
            {
                sprintf(lvl_filename, "%s%s", argv[1], dir_info->d_name);
            }
        }
    }

    while (!end_game)
    {
        if (strcmp(argv[1], "Default") == 0)
        {
            load_level(&game_board, accumulated_points);
        }
        else
        {
            handler_t handler = {LEVEL, NULL};
            file_parser(open(lvl_filename, O_RDONLY), &game_board, accumulated_points, &handler, argv[1]);
        }
        draw_board(&game_board, DRAW_MENU);
        refresh_screen();

        while (true)
        {
            int result = play_board(&game_board);

            if (result == NEXT_LEVEL)
            {
                screen_refresh(&game_board, DRAW_WIN);
                sleep_ms(game_board.tempo);
                break;
            }

            if (result == QUIT_GAME)
            {
                screen_refresh(&game_board, DRAW_GAME_OVER);
                sleep_ms(game_board.tempo);
                end_game = true;
                break;
            }

            if (result == CREATE_BACKUP) {
                saves_progress(&game_board);
            }

            if (result == LOAD_BACKUP) {
                exit(EXIT_FAILURE);
            }

            screen_refresh(&game_board, DRAW_MENU);

            accumulated_points = game_board.pacmans[0].points;
        }
        print_board(&game_board);
        unload_level(&game_board);
    }

    terminal_cleanup();

    close_debug_file();

    printf("%s\n", argv[1]);
    return 0;
}
