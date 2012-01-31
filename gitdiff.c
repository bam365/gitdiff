/* gitdiff.c - Git commit diff helper
 * Blake Mitchell, 2012
 */

#include <stdlib.h>
#include <curses.h>
#include <signal.h>

#define ARRYSIZE(x) (sizeof(x)/sizeof(x[0]))


void init_curses();
void print_strings(char *strings[], int size);
void set_signal_handlers();
void ev_loop(int size);
void end_curses();
void handle_resize(int sig);
void handle_sigint(int sig);


static int CURSES_SCREEN = 0;


main()
{
        char *strings[] = {"Hello", "There", "World"};

        set_signal_handlers();
        init_curses();
        print_strings(strings, ARRYSIZE(strings));
        ev_loop(ARRYSIZE(strings));
        end_curses();
}


void set_signal_handlers()
{
        struct sigaction sa_winch;
        struct sigaction sa_int;

        /*TODO: Error checking, bitch */
        sigfillset(&sa_winch.sa_mask);
        sigfillset(&sa_int.sa_mask);
        sa_winch.sa_flags = sa_int.sa_flags = 0;
        sigaction(SIGINT, &sa_int, 0);
        sigaction(SIGWINCH, &sa_winch, 0);
}


void init_curses()
{
        if (!CURSES_SCREEN) {
                initscr();
                cbreak();
                noecho();
                CURSES_SCREEN = 1;
        }
}


void end_curses()
{
        if (CURSES_SCREEN) {
                endwin();
                CURSES_SCREEN = 0;
        }
}


void print_strings(char *strings[], int size) 
{
        int i;

        for (i = 0; i < size; i++) {
                mvprintw(i, 0, "%s\n", strings[i]);
        }
}


void ev_loop(int size)
{
        char ch;
        int ln, ref;

        ln = 0;
        mvchgat(ln, 0, -1, A_REVERSE, 0, NULL);
        wrefresh(stdscr);
        ref = 0;

        while ((ch = getch()) != 'q') {
                switch (ch) {
                case 'k':
                        chgat(-1, A_NORMAL, 0, NULL);
                        ln = (ln > 0) ? ln-1 : 0;
                        ref = 1;
                        break;
                case 'j':
                        chgat(-1, A_NORMAL, 0, NULL);
                        ln = (ln < size-1) ? ln+1 : size-1;
                        ref = 1;
                        break;
                }
                if (ref) {
                        mvchgat(ln, 0, -1, A_REVERSE, 0, NULL);
                        refresh();
                        ref = 0;
                }
        }
}


void handle_resize(int sig)
{
        /*TODO: Stuff. */
}


void handle_int(int sig)
{
        end_curses();
        exit(0);
}
