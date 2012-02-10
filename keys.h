/* keys.h - gitdiff key bindings data structure
 * Blake Mitchell, 2012
 *
 * NOTE: This program uses a simple linked list to store keybindings. Is it the
 * most efficient structure for this purpose? No. But it is by far the simplest
 * and is still plenty efficient for what we're doing here. In addition, I do
 * some things like keep track of the final node in the list as well as the
 * last command executed that eliminate the need to iterate through the list in
 * the two most common cases that it will be used. Besides, we're really
 * not going to have more than about 20 keybindings anyway.
 */

#ifndef GITDIFF_KEYS_H
#define GITDIFF_KEYS_H

#include "gitdiff.h"


struct kb_node {
        int key;
        struct command cmd;
        struct kb_node *next;
};


struct keybindings {
        struct kb_node *kbl;
        struct kb_node *lastcmd;
        struct kb_node *end;
};


struct keybindings *new_keybindings();
void free_keybindings(struct keybindings *kb);
void add_keybinding(struct keybindings *kb, int key, struct command *cmd);
struct command *get_command(struct keybindings *kb, int key);




#endif
