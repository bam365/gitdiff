/* gitdiff.h - Git commit diff helper
 * Blake Mitchell, 2012
 */

#ifndef GITDIFF_H
#define GITDIFF_H

#include "commitlist.h"
#include <curses.h>

#define ARRYSIZE(x)     (sizeof(x)/sizeof(x[0]))
#define NUMKEYS         (1<<8)


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


struct command {
        void (*f)(struct gd_data *gdd, char *arg);
        char *arg;
};



/* Commands */

void scrollup(struct gd_data *gdd, char *arg);
void scrolldown(struct gd_data *gdd, char *arg);
void scrolltotop(struct gd_data *gdd, char *arg);
void scrolltobottom(struct gd_data *gdd, char *arg);
void pagedown(struct gd_data *gdd, char *arg);
void pageup(struct gd_data *gdd, char *arg);
void perc(struct gd_data *gdd, char *arg);
void selto(struct gd_data *gdd, char *arg);
void selfrom(struct gd_data *gdd, char *arg);
void find(struct gd_data *gdd, char *arg);
void selnext(struct gd_data *gdd, char *arg);
void selnext(struct gd_data *gdd, char *arg);

#endif
