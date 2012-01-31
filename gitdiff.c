/* gitdiff.c - Git commit diff helper
 * Blake Mitchell, 2012
 */

#include <stdio.h>
#include <curses.h>

#define ARRYSIZE(x) (sizeof(x)/sizeof(x[0]))

void print_strings(char *strings[], int size);
void ev_loop(int size);

main()
{
        char *strings[] = {"Hello", "There", "World"};

        initscr();
        cbreak();
        noecho();
        print_strings(strings, ARRYSIZE(strings));
        refresh();
        
        ev_loop(ARRYSIZE(strings));

        endwin();
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
