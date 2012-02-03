/* commitlist.c - Parsing git log's output into a data structure
 * Blake Mitchell, 2012
 */

#include <string.h>
#include <stdlib.h>
#include "commitlist.h"

#define MAX_LBUF_SIZE           512
#define MAX_COMMENT_SIZE        2046


static char *COMMIT_TOKEN = "commit ";
static char *AUTHOR_TOKEN = "Author: ";
static char *DATE_TOKEN =   "Date:   ";
static char *COMMENT_TOKEN = "    ";



struct commit_node* new_commit_node(struct commit_node *prev, 
                                    struct commit_node *next)
{
      struct commit_node *n;

      n = (struct commit_node*)malloc(sizeof(struct commit_node));
      memset(n->hash, '\0', COMMIT_HASH_SIZE);
      n->author = NULL;
      n->date = NULL;
      n->comment = NULL;
      n->prev = prev;
      n->next = next;

      return n;
}


int begins_with(char *s, char *tkn) 
{
       if (!(s && tkn)) 
               return 0;
       return (!strncmp(s, tkn, strlen(tkn)));
}


char *new_str_after_token(char *s, char *tkn) 
{
        char *ret, *p;

        ret = NULL;
        if (begins_with(s, tkn)) {
                /* Don't want spaces at the beginning */
                for (p = (s + strlen(tkn)); *p == ' '; p++);
                ret = (char*)malloc(strlen(p) + 1);
                memset(ret, '\0', strlen(p) + 1);
                strcpy(ret, p);
                /* Don't want newlines at the end */
                for (p = (ret + strlen(ret) - 1); *p == '\n' && p > ret; p--)
                        *p = '\0';
        }

        return ret;
}


void remove_newlines_and_tabs(char *s) 
{
        int i;

        if (!s)
                return;

        for (i = 0; i < strlen(s); i++) 
                if (s[i] == '\n' || s[i] == '\t')
                        s[i] = ' ';
}


int parse_comment(struct commit_node *cn, char *lbuf, FILE *f) 
{
        char cbuf[MAX_COMMENT_SIZE];
        char *cp, *lp;

        memset(cbuf, '\0', sizeof(cbuf));
        cp = cbuf;
        while (begins_with(fgets(lbuf, MAX_LBUF_SIZE, f), COMMENT_TOKEN))
        {
                lp = lbuf + strlen(COMMENT_TOKEN);
                if (cp - cbuf + strlen(lp) < MAX_COMMENT_SIZE) {
                        memcpy(cp, lp, strlen(lp));
                        cp += strlen(lp);
                }
        }
        cn->comment = new_str_after_token(cbuf, "");  
        remove_newlines_and_tabs(cn->comment); 

        return 0;
}


int parse_commit(struct commit_node* n, FILE *f)
{
        char lbuf[MAX_LBUF_SIZE];

        fgets(lbuf, MAX_LBUF_SIZE, f);
        if (begins_with(lbuf, COMMIT_TOKEN)) 
               memcpy(n->hash, lbuf + strlen(COMMIT_TOKEN), COMMIT_HASH_SIZE);
        else
               return 0;
        while (strcmp(fgets(lbuf, MAX_LBUF_SIZE, f), "")
               &&  strcmp(lbuf, "\n")) 
        {
                if (begins_with(lbuf, AUTHOR_TOKEN))
                        n->author = new_str_after_token(lbuf, AUTHOR_TOKEN);
                if (begins_with(lbuf, DATE_TOKEN)) 
                        n->date = new_str_after_token(lbuf, DATE_TOKEN);
               
        }
        parse_comment(n, lbuf, f);

        return 1;
}


commit_list parse_commit_list(FILE *f)
{
        struct commit_node *root, *n;

        n = root = new_commit_node(NULL, NULL);
        while (parse_commit(n, f) == 1) {
                if (n->prev)
                        n->prev->next = n;
                n = new_commit_node(n, NULL);
        }
        if (n->prev)
                n->prev->next = NULL;
        free_commit_list(&n); 

        return root;
}


void free_commit_list(commit_list* cl)
{
        struct commit_node *n, *tmp;

        n = *cl;
        while (n) {
                if (n->author)
                        free(n->author);
                if (n->date)
                        free(n->date);
                if (n->comment)
                        free(n->comment);
                tmp = n->next;
                free(n);
                n = tmp;
        }
        *cl = NULL; 
}


int commit_list_count(commit_list cl)
{
        struct commit_node *n;
        int c;

        for (n = cl, c = 0; n; n = n->next, c++)
                ;

        return c;
}


int traverse_back(commit_list *cl, int max)
{
        struct commit_node*ln;
        int i;

        ln = *cl; 
        for (i = 0; *cl && i < max; i++) {
                ln = *cl;
                *cl = (*cl)->prev;
        }
        if (!*cl) {
                *cl = ln;
                i--;
        }

        return i;
}


int traverse_forward(commit_list *cl, int max)
{
        struct commit_node*ln;
        int i;

        ln = *cl; 
        for (i = 0; *cl && i < max; i++) {
                ln = *cl;
                *cl = (*cl)->next;
        }
        if (!*cl) {
                *cl = ln;
                i--;
        }

        return i;
}
