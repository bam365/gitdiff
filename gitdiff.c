/* gitdiff.c - Git commit diff helper
 * Blake Mitchell, 2012
 */

#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include "commitlist.h"

#define ARRYSIZE(x) (sizeof(x)/sizeof(x[0]))


struct  gd_data {
        WINDOW *lwin, *fromwin, *towin, *statwin;
        commit_list cl;
        int lsel, lw, lh;
        struct commit_node *csel;
};



void init_curses();
void set_signal_handlers();
void ev_loop(struct gd_data *gdd);
void end_curses();
void handle_resize(int sig);
void handle_sigint(int sig);
void stdin_from_tty();
void init_windows(struct gd_data *gdd);
void draw_list(struct gd_data *gdd);


/* Such is the curse of signal handling */
static int CURSES_SCREEN = 0;
static struct gd_data GDDATA;



main()
{
        struct commit_node *np;
        int i;

        GDDATA.cl = parse_commit_list(stdin);
        stdin_from_tty();
        set_signal_handlers();
        init_curses();
        init_windows(&GDDATA);
        draw_list(&GDDATA);
        ev_loop(&GDDATA);
        end_curses();
        free_commit_list(&(GDDATA.cl));
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


/* Since we piped in input, we must change the stdin fd back to 
 * the terminal so the user can give input to curses
 */
void stdin_from_tty()
{
        int newstdinfd;

        newstdinfd = open(ttyname(STDOUT_FILENO), O_RDONLY);
        dup2(newstdinfd, STDIN_FILENO);
}


void init_curses()
{
        if (!CURSES_SCREEN) {
                initscr();
                cbreak();
                noecho();
                curs_set(0);
                CURSES_SCREEN = 1;
        }
}


void init_windows(struct gd_data *gdd) 
{
        int ymax, xmax, ypos;

        getmaxyx(stdscr, ymax, xmax);
        ypos = ymax;
        gdd->statwin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->towin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->fromwin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->lwin = subwin(stdscr, ypos, 0, 0, 0);
        box(gdd->lwin, 0, 0);
}


void draw_list(struct gd_data *gdd)
{
        struct commit_node *n;
        int ybeg, xbeg, ymax, xmax ;
        int lnum;

        getbegyx(gdd->lwin, ybeg, xbeg);
        getmaxyx(gdd->lwin, ymax, xmax);
        
        gdd->lsel = 1;
        gdd->lw = xmax - xbeg;
        gdd->lh = ymax - ybeg;
        gdd->csel = gdd->cl;
        for (lnum = 1, n = gdd->cl; lnum < ymax && n != NULL; 
             n = n->next, lnum++) 
                mvwaddnstr(gdd->lwin, lnum, 1, n->comment, xmax-1);
        wrefresh(gdd->lwin);
}
       

void end_curses()
{
        if (CURSES_SCREEN) {
                endwin();
                CURSES_SCREEN = 0;
        }
}


int select_next(struct gd_data *gdd)
{
        if (gdd->csel->next) {
                mvchgat(gdd->lsel, 1, gdd->lw - 2, A_NORMAL, 0, NULL);
                gdd->lsel++;
                gdd->csel = gdd->csel->next;
                mvchgat(gdd->lsel, 1, gdd->lw - 2, A_REVERSE, 0, NULL);
                return 1;
        } else {
                return 0;
        }
}
        

int select_prev(struct gd_data *gdd)
{
        if (gdd->csel->prev) {
                mvchgat(gdd->lsel, 1, gdd->lw - 2, A_NORMAL, 0, NULL);
                gdd->lsel--;
                gdd->csel = gdd->csel->prev;
                mvchgat(gdd->lsel, 1, gdd->lw - 2, A_REVERSE, 0, NULL);
                return 1;
        } else {
                return 0;
        }
}


void ev_loop(struct gd_data *gdd)
{
        char ch;
        int ln, ref;

        ln = 1;
        mvchgat(gdd->lsel, 1, gdd->lw - 2, A_REVERSE, 0, NULL);
        wrefresh(gdd->lwin);
        wrefresh(gdd->statwin);
        wrefresh(gdd->fromwin);
        wrefresh(gdd->towin);
        refresh();
        ref = 0;

        while ((ch = tolower(getch())) != 'q') {
                switch (ch) {
                case 'j':
                        ref = select_next(gdd);
                        break;
                case 'k':
                        ref = select_prev(gdd);
                        break;
                }
                if (ref) {
                        wrefresh(gdd->lwin);
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
