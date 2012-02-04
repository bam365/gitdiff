/* gitdiff.h - Git commit diff helper
 * Blake Mitchell, 2012
 */

#ifndef GITDIFF_H
#define GITDIFF_H

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


#endif
