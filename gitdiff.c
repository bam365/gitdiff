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


enum {
        CLR_HEADER = 1,
        CLR_HEAD,
        CLR_TOSEL,
        CLR_FROMSEL
};


struct  gd_data {
        WINDOW *lwin, *fromwin, *towin, *statwin;
        int lref, fref, tref, sref;
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
void decorate_list_entry(struct gd_data *gdd, int lnum);
void draw_list(struct gd_data *gdd);
void draw_statbar(struct gd_data *gdd);
void draw_towin(struct gd_data *gdd);
void draw_fromwin(struct gd_data *gdd);
void refresh_windows(struct gd_data *gdd);
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
        draw_fromwin(&GDDATA);
        draw_towin(&GDDATA);
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
        gdd->lref = gdd->tref = gdd->fref = gdd->sref = 0;
}


void set_signal_handlers()
{
        struct sigaction sa_winch;
        struct sigaction sa_int;

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


void init_colors()
{
        init_pair(CLR_HEADER, COLOR_YELLOW, -1);
        init_pair(CLR_HEAD, COLOR_RED, -1);
        init_pair(CLR_TOSEL, COLOR_BLUE, -1);
        init_pair(CLR_FROMSEL, COLOR_GREEN, -1);
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
                use_default_colors(); 
                init_colors();
                CURSES_SCREEN = 1;
        }
}


void init_windows(struct gd_data *gdd) 
{
        int ymax, xmax, ypos;

        getmaxyx(stdscr, ymax, xmax);
        ypos = ymax-1;
        gdd->statwin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->fromwin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->towin = subwin(stdscr, 1, 0, ypos--, 0);
        gdd->lwin = subwin(stdscr, ypos, 0, 0, 0);
        box(gdd->lwin, 0, 0);
        gdd->lref = 1;
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
        decorate_list_entry(gdd, gdd->lsel);
        gdd->lref = 1;
}


void add_labeled_text(WINDOW *w, char *lbl, char *str, int attr)
{
        wattrset(w, A_REVERSE);
        mvwaddstr(w, 0, 0, lbl);
        wattrset(w, attr);
        waddstr(w, "  ");
        waddstr(w, str);
}


void decorate_list_entry(struct gd_data *gdd, int lnum)
{
        int attr, hdrc, cmtc;
        
        attr = (gdd->lsel == lnum) ? A_REVERSE : A_NORMAL;
        mvwchgat(gdd->lwin, lnum, 1, gdd->lw, attr, CLR_HEADER, NULL);
        mvwchgat(gdd->lwin, lnum+1, 1, gdd->lw, attr, 0, NULL);
} 
        

void draw_list(struct gd_data *gdd)
{
        int plines, alines, tlines;
        int lcount, li, lbsize;
        struct commit_node *n, *lastn;
        char *lbuf;

        tlines = gdd->lh / 2;
        plines = (gdd->lsel - 1) / 2;
        alines = tlines - plines - 1;
        n = gdd->csel;
        lcount = traverse_back(&n, plines);
        gdd->lsel -= (plines - lcount);
        lbsize = gdd->lw + 1;
        lbuf = (char*)malloc(lbsize);
        memset(lbuf, ' ', lbsize-1);
        lbuf[lbsize-1] = '\0';
        for (lcount = 1; lcount <= gdd->lh; lcount++) 
                mvwaddnstr(gdd->lwin, lcount, 1, lbuf, gdd->lw); 
        li = 1;
        for (lcount = 0; lcount < tlines && n; lcount++) {
                memset(lbuf, '\0', lbsize);
                snprintf(lbuf, lbsize, "%s | %s", n->date, n->author);
                mvwaddnstr(gdd->lwin, li++, 1, lbuf, gdd->lw); 
                mvwaddnstr(gdd->lwin, li++, 5, n->comment, gdd->lw - 4);
                decorate_list_entry(gdd, li-2);
                n = n->next;
        }

        free(lbuf);        
        gdd->lref = 1;
} 


void draw_towin(struct gd_data *gdd)
{
        char *txt;
        int attr;

        if (gdd->cto) {
                txt = gdd->cto->date;
                attr = COLOR_PAIR(CLR_TOSEL);
        } else {
                txt = "HEAD (Press 't' to use selected commit)";
                attr = COLOR_PAIR(CLR_HEAD);
        }
        werase(gdd->towin);
        add_labeled_text(gdd->towin, "  TO:", txt, attr);
        gdd->tref = 1;
}


void draw_fromwin(struct gd_data *gdd)
{
        char *txt;
        int attr;

        if (gdd->cfrom) {
                txt = gdd->cfrom->date;
                attr = COLOR_PAIR(CLR_FROMSEL);
        } else {
                txt = "HEAD (Press 'f' to use selected commit)";
                attr = COLOR_PAIR(CLR_HEAD);
        }
        werase(gdd->fromwin);
        add_labeled_text(gdd->fromwin, "FROM:", txt, attr);
        gdd->fref = 1;
}


void draw_statbar(struct gd_data *gdd)
{
        char sbuf[256];

        sprintf(sbuf, "\t%d commits", gdd->ccount);
        werase(gdd->statwin);
        waddstr(gdd->statwin, sbuf);
        gdd->sref = 1;
}


void end_curses()
{
        if (CURSES_SCREEN) {
                endwin();
                CURSES_SCREEN = 0;
        }
}


int change_selection(struct gd_data *gdd, int diff)
{
        int d, maxpos, prevsel;

        prevsel = gdd->lsel;
        if (diff > 0) {
                d = traverse_forward(&(gdd->csel), diff);
                gdd->lsel += d*2;
                maxpos = gdd->lh - ((gdd->lh % 2) ? 2 : 1); 
                if (gdd->lsel > maxpos) {
                        draw_list(gdd);
                        gdd->lsel = maxpos;
                } 
        } else if (diff < 0) {
                d = traverse_back(&(gdd->csel), -diff);
                gdd->lsel -= d*2;
                if (gdd->lsel < 1) {
                        draw_list(gdd);
                        gdd->lsel = 1;
                }
        }
        decorate_list_entry(gdd, prevsel);
        decorate_list_entry(gdd, gdd->lsel);
        gdd->lref = 1;

        return d;
}


void ev_loop(struct gd_data *gdd)
{
        int ch;

        refresh_windows(gdd);

        while ((ch = tolower(getch())) != 'q') {
                switch (ch) {
                case 'j':
                        change_selection(gdd, 1);
                        break;
                case 'k':
                        change_selection(gdd, -1);
                        break;
                case 't':
                        gdd->cto = gdd->csel;
                        gdd->tref = 1;
                        break;
                case 'f':
                        gdd->cfrom = gdd->csel;
                        gdd->fref = 1;
                        break;
                case '\n':
                case KEY_ENTER:
                        start_diff_tool(gdd);
                        break;
                }
                refresh_windows(gdd);
        }
}


void refresh_windows(struct gd_data *gdd)
{
        if (gdd->lref) {
                wrefresh(gdd->lwin);
                gdd->lref = 0;
        }
        if (gdd->tref) {
                draw_towin(gdd);
                wrefresh(gdd->towin);
                gdd->tref = 0;
        }
        if (gdd->fref) {
                draw_fromwin(gdd);
                wrefresh(gdd->fromwin);
                gdd->fref = 0;
        }
        if (gdd->sref) {
                wrefresh(gdd->statwin);
                gdd->sref = 0;
        }
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
