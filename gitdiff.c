/* gitdiff.c - Git commit diff helper
 * Blake Mitchell, 2012
 */

#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include "commitlist.h"

#define ARRYSIZE(x) (sizeof(x)/sizeof(x[0]))


struct  gd_data {
        WINDOW *lwin, *fromwin, *towin, *statwin;
        commit_list cl;
        int selected;

};



void init_curses();
void set_signal_handlers();
void ev_loop(int size);
void end_curses();
void handle_resize(int sig);
void handle_sigint(int sig);


/* Such is the curse of signal handling */
static int CURSES_SCREEN = 0;
static struct gd_data GDDATA;



main()
{
        struct commit_node *np;
        int i;

        GDDATA.cl = parse_commit_list(stdin);

        for (np = GDDATA.cl; np != NULL; np = np->next) {
                printf("\n\nCommit hash: ");
                for (i = 0; i < COMMIT_HASH_SIZE; i++) 
                        fputc(np->hash[i], stdout);
                printf("\nAuthor: %s\n", np->author);
                printf("Date: %s\n", np->date);
                printf("Comment: %s\n", np->comment);
                
        }
        free_commit_list(&(GDDATA.cl));

        /*set_signal_handlers();
        init_curses();
        ev_loop(ARRYSIZE(strings));
        end_curses();
        */
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


void ev_loop(int size)
{
        char ch;
        int ln, ref;

        ln = 0;
        mvchgat(ln, 0, -1, A_REVERSE, 0, NULL);
        wrefresh(stdscr);
        ref = 0;

        while ((ch = tolower(getch())) != 'q') {
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
