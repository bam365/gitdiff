/* gitdiff.c - Git commit diff helper
 * Blake Mitchell, 2012
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include "gitdiff.h"
#include "keys.h"


static int CURSES_SCREEN = 0;

struct defkey {
        int c; 
        void (*f)(struct gd_data*, char *arg);
        char *arg;
} DEFAULT_KEYS[] = {
        { 'j', scrolldown, NULL },
        { 'k', scrollup, NULL },
        { 'g', scrolltotop, NULL },
        { 'G', scrolltobottom, NULL },
        { ' ', pagedown, NULL },
        { KEY_NPAGE, pagedown, NULL },
        { KEY_PPAGE, pageup, NULL },
        { 'f', selfrom, NULL },
        { 't', selto, NULL }
};



void stdin_from_tty();
void init_gdd(struct gd_data *gdd);
void set_keys(struct keybindings *kb, struct defkey dk[], int size);
void init_curses();
void init_colors();
void init_windows(struct gd_data *gdd);
void init_list(struct gd_data *gdd);
void decorate_list_entry(struct gd_data *gdd, int lnum, 
                         struct commit_node* n);
void draw_list(struct gd_data *gdd);
void draw_statbar(struct gd_data *gdd);
void draw_towin(struct gd_data *gdd);
void draw_fromwin(struct gd_data *gdd);
void ev_loop(struct gd_data *gdd, struct keybindings *kb);
void refresh_windows(struct gd_data *gdd);
void resize_windows(struct gd_data *gdd);
void end_curses();
void start_diff_tool(struct gd_data *gdd);
void run_command(struct command *cmd, struct gd_data *gdd);



main()
{
        struct keybindings *keys;
        struct gd_data gddata;
        struct gd_data *gdd;

        gdd = &gddata;
        keys = new_keybindings();

        init_gdd(gdd);
        if (!gdd->ccount) {
                printf("No git commit data\n");
                return 0;
        }

        stdin_from_tty();

        set_keys(keys, DEFAULT_KEYS, ARRYSIZE(DEFAULT_KEYS));

        init_curses();
        init_windows(gdd);
        init_list(gdd);
        draw_statbar(gdd);
        draw_fromwin(gdd);
        draw_towin(gdd);
        ev_loop(gdd, keys);
        end_curses();
        free_commit_list(&(gdd->cl));
        free_keybindings(keys);
        return 0;
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


void init_gdd(struct gd_data *gdd)
{
        gdd->cl = parse_commit_list(stdin);
        gdd->ccount = commit_list_count(gdd->cl);
        gdd->cto = gdd->cfrom = NULL;
        gdd->lref = gdd->tref = gdd->fref = gdd->sref = 0;
}


void set_keys(struct keybindings *kb, struct defkey dk[], int size)
{
        struct defkey *k;
        struct command cmd;

        for (k = dk; (k - dk) < size; k++) {
                cmd.f = k->f;
                cmd.arg = k->arg;
                add_keybinding(kb, k->c, &cmd);
        }
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


void init_colors()
{
        init_pair(CLR_HEADER, COLOR_YELLOW, -1);
        init_pair(CLR_HEAD, COLOR_RED, -1);
        init_pair(CLR_TOSEL, COLOR_BLUE, -1);
        init_pair(CLR_FROMSEL, COLOR_GREEN, -1);
}


void init_windows(struct gd_data *gdd) 
{
        int ymax, xmax, ypos;

        getmaxyx(stdscr, ymax, xmax);
        ypos = 0;
        gdd->towin = subwin(stdscr, 1, 0, ypos++, 0);
        gdd->fromwin = subwin(stdscr, 1, 0, ypos++, 0);
        gdd->lwin = subwin(stdscr, (ymax - ypos - 1), 0, ypos, 0);
        ypos += (ymax - ypos - 1);
        gdd->statwin = subwin(stdscr, 1, 0, ypos, 0);
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
        gdd->lw = xmax - 2;
        gdd->lh = ymax - 2;
        gdd->csel = gdd->cl;
        draw_list(gdd);
        decorate_list_entry(gdd, gdd->lsel, gdd->csel);
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


void decorate_list_entry(struct gd_data *gdd, int lnum, struct commit_node *cn)
{
        int attr, hdrc;
        
        attr = (gdd->lsel == lnum) ? A_REVERSE : A_NORMAL;
        hdrc = CLR_HEADER;
        if (cn)
                if (cn == gdd->cto) 
                        hdrc = CLR_TOSEL;
                else if (cn == gdd->cfrom)
                        hdrc = CLR_FROMSEL;
        mvwchgat(gdd->lwin, lnum, 1, gdd->lw, attr, hdrc, NULL);
        mvwchgat(gdd->lwin, lnum+1, 1, gdd->lw, attr, 0, NULL);
} 
        

void clear_list(struct gd_data *gdd)
{
        int lbsize, lcount;
        char *lbuf;

        lbsize = gdd->lw + 1;
        lbuf = (char*)malloc(lbsize);
        memset(lbuf, ' ', lbsize-1);
        lbuf[lbsize - 1] = '\0';
        for (lcount = 1; lcount <= gdd->lh; lcount++) 
                mvwaddnstr(gdd->lwin, lcount, 1, lbuf, gdd->lw); 
        free(lbuf);
}


/* draw_list uses gdd->lsel and gdd->csel to determine which items should be in
 * the list, so make sure you set them appropriately before calling this
 */
void draw_list(struct gd_data *gdd)
{
        int plines, tlines;
        int lcount, li, lbsize;
        struct commit_node *n;
        char *lbuf;

        clear_list(gdd);
        tlines = gdd->lh / 2;
        plines = (gdd->lsel - 1) / 2;
        n = gdd->csel;
        lbsize = gdd->lw + 1;
        lbuf = (char*)malloc(lbsize);
        lcount = traverse_back(&n, plines);
        gdd->lsel -= (plines - lcount)*2;
        li = 1;
        for (lcount = 0; lcount < tlines && n; lcount++) {
                memset(lbuf, '\0', lbsize);
                snprintf(lbuf, lbsize, "%s | %s", n->date, n->author);
                mvwaddnstr(gdd->lwin, li++, 1, lbuf, gdd->lw); 
                mvwaddnstr(gdd->lwin, li++, 5, n->comment, gdd->lw - 4);
                decorate_list_entry(gdd, li-2, n);
                n = n->next;
        }
        gdd->lref = 1;

        free(lbuf);        
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
        int perc;

        werase(gdd->statwin);
        sprintf(sbuf, "%d commits", gdd->ccount);
        waddstr(gdd->statwin, sbuf);
        perc = 100 * (gdd->csel->ind + 1) / gdd->ccount;
        if (gdd->csel->ind == 0) 
                strcpy(sbuf, " TOP");
        else if (gdd->csel->ind == gdd->ccount - 1) 
                strcpy(sbuf, " BOT");
        else
                sprintf(sbuf, "%3d%%", perc);
        mvwaddstr(gdd->statwin, 0, gdd->lw-4, sbuf);
        gdd->sref = 1;
}


int max_list_ind(struct gd_data *gdd)
{
        return (gdd->lh - ((gdd->lh % 2) ? 2 : 1)); 
}


int change_selection(struct gd_data *gdd, int diff)
{
        int d, maxpos, prevsel;
        struct commit_node *prevn;

        prevsel = gdd->lsel;
        prevn = gdd->csel;
        if (diff > 0) {
                d = traverse_forward(&(gdd->csel), diff);
                gdd->lsel += d*2;
                maxpos = max_list_ind(gdd);
                if (gdd->lsel > maxpos) {
                        gdd->lsel = maxpos;
                        draw_list(gdd);
                } 
        } else if (diff < 0) {
                d = traverse_back(&(gdd->csel), -diff);
                gdd->lsel -= d*2;
                if (gdd->lsel < 1) {
                        gdd->lsel = 1;
                        draw_list(gdd);
                }
        }
        decorate_list_entry(gdd, prevsel, prevn);
        decorate_list_entry(gdd, gdd->lsel, gdd->csel);
        gdd->lref = 1;
        return d;
}


void ev_loop(struct gd_data *gdd, struct keybindings *kb)
{
        int ch;
        struct command *cmd;

        refresh_windows(gdd);

        while ((ch = getch()) != 'q') {
                switch (ch) {
                case '\n':
                case KEY_ENTER:
                        start_diff_tool(gdd);
                        break;
                case KEY_RESIZE:
                        resize_windows(gdd);
                default:
                        run_command((cmd = get_command(kb, ch)), gdd);
                        if (cmd)
                                draw_statbar(gdd);
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



void resize_windows(struct gd_data *gdd)
{
        endwin();
        init_windows(gdd);
        init_list(gdd);
        draw_list(gdd);
        draw_statbar(gdd);
        draw_fromwin(gdd);
        draw_towin(gdd);
        gdd->lref = gdd->tref = gdd->fref = gdd->sref = 1;
        refresh();
}


void end_curses()
{
        if (CURSES_SCREEN) {
                endwin();
                CURSES_SCREEN = 0;
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


void run_command(struct command *cmd, struct gd_data *gdd)
{
        if (cmd && cmd->f)
                cmd->f(gdd, cmd->arg);
}


/* Commands */

void scrollup(struct gd_data *gdd, char *arg)
{
        change_selection(gdd, -1);
}


void scrolldown(struct gd_data *gdd, char *arg)
{
        change_selection(gdd, 1);
}


void scrolltotop(struct gd_data *gdd, char *arg)
{
        gdd->csel = gdd->cl;
        gdd->lsel = 1;
        draw_list(gdd);
}

        
void scrolltobottom(struct gd_data *gdd, char *arg)
{
        traverse_forward(&(gdd->csel), -1);
        gdd->lsel = max_list_ind(gdd);
        draw_list(gdd);
        gdd->lref = 1;
}


void pagedown(struct gd_data *gdd, char *arg)
{
        change_selection(gdd, max_list_ind(gdd));
}


void pageup(struct gd_data *gdd, char *arg)
{
        change_selection(gdd, -max_list_ind(gdd));
}


void perc(struct gd_data *gdd, char *arg)
{
}


void selto(struct gd_data *gdd, char *arg)
{
        gdd->cto = gdd->csel;
        draw_list(gdd);
        gdd->tref = 1;
        gdd->lref = 1;
}


void selfrom(struct gd_data *gdd, char *arg)
{
        gdd->cfrom = gdd->csel;
        draw_list(gdd);
        gdd->fref = 1;
        gdd->lref = 1;
}


void find(struct gd_data *gdd, char *arg)
{
}


void selnext(struct gd_data *gdd, char *arg)
{
}


void selprev(struct gd_data *gdd, char *arg)
{
}
