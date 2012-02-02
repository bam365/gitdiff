/* gitdiff.c - Git commit diff helper
 * Blake Mitchell, 2012
 */

#include <stdlib.h>
#include <string.h>
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
        struct commit_node *cfrom, *cto;
        int ccount;
};


/* Such is the curse of signal handling */
static int CURSES_SCREEN = 0;
/* Keep this program maintainable. Pass a pointer to GDDATA to any function
 * besides signal handlers that needs to use it 
 */
static struct gd_data GDDATA;



void init_gdd(struct gd_data *gdd);
void init_curses();
void set_signal_handlers();
void ev_loop(struct gd_data *gdd);
void end_curses();
void handle_resize(int sig);
void handle_sigint(int sig);
void stdin_from_tty();
void init_windows(struct gd_data *gdd);
void init_list(struct gd_data *gdd);
void draw_list(struct gd_data *gdd);
void draw_statbar(struct gd_data *gdd);
void start_diff_tool(struct gd_data *gdd);




main()
{
        init_gdd(&GDDATA);
        if (!GDDATA.ccount) {
                printf("No git commit data\n");
                return 0;
        }

        stdin_from_tty();
        set_signal_handlers();

        init_curses();
        init_windows(&GDDATA);
        init_list(&GDDATA);
        draw_statbar(&GDDATA);
        ev_loop(&GDDATA);
        end_curses();
        free_commit_list(&(GDDATA.cl));
        return 0;
}


void init_gdd(struct gd_data *gdd)
{
        gdd->cl = parse_commit_list(stdin);
        gdd->ccount = commit_list_count(gdd->cl);
        gdd->cto = gdd->cfrom = NULL;
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
                keypad(stdscr, 1);
                curs_set(0);
                start_color();
                init_pair(1, COLOR_GREEN, COLOR_BLACK);
                CURSES_SCREEN = 1;
        }
}


void init_windows(struct gd_data *gdd) 
{
        int ymax, xmax, ypos;

        getmaxyx(stdscr, ymax, xmax);
        ypos = ymax-1;
        gdd->statwin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->towin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->fromwin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->lwin = subwin(stdscr, ypos, 0, 0, 0);
        box(gdd->lwin, 0, 0);
}



void init_list(struct gd_data *gdd)
{
        struct commit_node *n;
        int ybeg, xbeg, ymax, xmax;
        int lnum;

        getbegyx(gdd->lwin, ybeg, xbeg);
        getmaxyx(gdd->lwin, ymax, xmax);
        
        gdd->lsel = 1;
        gdd->lw = xmax - xbeg - 2;
        gdd->lh = ymax - ybeg - 2;
        gdd->csel = gdd->cl;
        draw_list(gdd);
        wrefresh(gdd->lwin);
}


void draw_list(struct gd_data *gdd)
{
        /*TODO: This function is a bit hefty */
        int plines, alines, tlines;
        int lcount, li, lbsize;
        struct commit_node *n, *lastn;
        char *lbuf;

        plines = (gdd->lsel - 1) / 2;
        alines = (gdd->lh - gdd->lsel - 1) / 2;
        tlines = alines + 1 + plines;
        for (lcount = 0, n = gdd->csel; n && lcount <= plines; lcount++) {
                lastn = n;
                n = n->prev;
        }
        gdd->lsel -= (plines - (lcount - 1));
        lbsize = gdd->lw + 1;
        lbuf = (char*)malloc(lbsize);
        li = 1;
        for (lcount = 0, n = lastn; lcount < tlines && n; lcount++) {
                memset(lbuf, '\0', lbsize);
                snprintf(lbuf, lbsize, "%s | %s", n->date, n->author);
                wattron(gdd->lwin, COLOR_PAIR(1));
                mvwaddnstr(gdd->lwin, li++, 1, lbuf, gdd->lw); 
                wattroff(gdd->lwin, COLOR_PAIR(1));
                mvwaddnstr(gdd->lwin, li++, 5, n->comment, gdd->lw - 4);
                n = n->next;
        }
        if (li < gdd->lh) {
                memset(lbuf, ' ', lbsize-1);
                while (li < gdd->lh) 
                        mvwaddnstr(gdd->lwin, li++, 1, lbuf, gdd->lw);
        }
        free(lbuf);        
} 


void draw_statbar(struct gd_data *gdd)
{
        char sbuf[256];

        sprintf(sbuf, "\t%d commits", gdd->ccount);
        waddstr(gdd->statwin, sbuf);
        wrefresh(gdd->statwin);
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
                gdd->csel = gdd->csel->next;
                if (gdd->lsel+1 < gdd->lh-1) {
                        mvchgat(gdd->lsel, 1, gdd->lw, A_NORMAL, 1, NULL);
                        mvchgat(gdd->lsel+1, 1, gdd->lw, A_NORMAL, 0, NULL);
                        gdd->lsel += 2;
                } else {
                        draw_list(gdd);
                }
                mvchgat(gdd->lsel, 1, gdd->lw, A_REVERSE, 1, NULL);
                mvchgat(gdd->lsel+1, 1, gdd->lw, A_REVERSE, 0, NULL);
                return 1;
        } else {
                return 0;
        }
}
        

int select_prev(struct gd_data *gdd)
{
        if (gdd->csel->prev) {
                gdd->csel = gdd->csel->prev;
                if (gdd->lsel > 1) {
                        mvwchgat(gdd->lwin, gdd->lsel, 1, gdd->lw, 
                                 A_NORMAL, 1, NULL);
                        mvwchgat(gdd->lwin, gdd->lsel+1, 1, gdd->lw, 
                                 A_NORMAL, 0, NULL);
                        gdd->lsel -= 2;
                } else {
                        draw_list(gdd);
                }
                mvchgat(gdd->lsel, 1, gdd->lw, A_REVERSE, 1, NULL);
                mvchgat(gdd->lsel+1, 1, gdd->lw, A_REVERSE, 0, NULL);
                return 1;
        } else {
                return 0;
        }
}


void ev_loop(struct gd_data *gdd)
{
        int ch;
        int ln, lref, tref, fref, sref;

        mvchgat(gdd->lsel, 1, gdd->lw, A_REVERSE, 1, NULL);
        mvchgat(gdd->lsel+1, 1, gdd->lw, A_REVERSE, 0, NULL);
        refresh();
        lref = tref = fref = sref = 0;

        while ((ch = tolower(getch())) != 'q') {
                switch (ch) {
                case 'j':
                        lref = select_next(gdd);
                        break;
                case 'k':
                        lref = select_prev(gdd);
                        break;
                case 't':
                        gdd->cto = gdd->csel;
                        break;
                case 'f':
                        gdd->cfrom = gdd->csel;
                        break;
                case '\n':
                case KEY_ENTER:
                        start_diff_tool(gdd);
                        break;
                }
                /* TODO: This is kind of stupid, find a better way */
                if (lref) {
                        wrefresh(gdd->lwin);
                        lref = 0;
                }
                if (tref) {
                        wrefresh(gdd->towin);
                        tref = 0;
                }
                if (fref) {
                        wrefresh(gdd->fromwin);
                        fref = 0;
                }
                if (sref) {
                        wrefresh(gdd->statwin);
                        sref = 0;
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


void start_diff_tool(struct gd_data *gdd)
{
        end_curses();
        char astr[256];

        memset(astr, '\0', sizeof(astr));
        if (gdd->cfrom)
                strncpy(astr, gdd->cfrom->hash, COMMIT_HASH_SIZE);
        else
                strcpy(astr, "HEAD");
        strcat(astr, "..");
        if (gdd->cto)
                strncat(astr, gdd->cto->hash, COMMIT_HASH_SIZE);
        else
                strcpy(astr, "HEAD");
        execlp("git", "git", "difftool", astr, NULL);
}
