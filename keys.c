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

#include "keys.h"
#include <stdlib.h>
#include <string.h>


struct keybindings *new_keybindings()
{
        struct keybindings *newkb;

        newkb = (struct keybindings*)malloc(sizeof(struct keybindings));
        newkb->kbl = NULL;
        newkb->lastcmd = NULL;
        newkb->end = NULL;
}


void free_keybindings(struct keybindings *kb)
{
        if (!kb)
                return;

        struct kb_node *n, *tmp;

        n = kb->kbl;
        while (n) {
                tmp = n->next;
                free(n);
                n = n->next;
        }
        free(kb);
}


void add_keybinding(struct keybindings *kb, int key, struct command *cmd)
{
        struct kb_node *n;

        n = (struct kb_node*)malloc(sizeof(struct kb_node));
        n->key = key;
        memcpy(&(n->cmd), cmd, sizeof(n->cmd));
        n->next = NULL;
        if (!kb->kbl) 
                kb->kbl = n;
        else
                kb->end->next = n;
        kb->end = n;
}


struct kb_node *find_keybinding(struct keybindings *kb, int key) 
{
        struct kb_node *n, *found;

        found = NULL;
        for (n = kb->kbl; n && !found; n = n->next)
                if (n->key == key)
                        found = n;
        
        return found;
}

                
struct command *get_command(struct keybindings *kb, int key)
{
        struct kb_node *n;

        if (kb->lastcmd && kb->lastcmd->key == key) 
                n = kb->lastcmd;
        else 
                n = find_keybinding(kb, key);
        if (n) {
                kb->lastcmd = n;
                return &(n->cmd);
        } else {
                return NULL;
        }
}



