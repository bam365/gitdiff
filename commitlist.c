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


int begins_with(char *s, char *tkn) {
       if (!(s && tkn)) 
               return 0;
       return (!strncmp(s, tkn, strlen(tkn)));
}


/* Checks that s begins with tkn. If it does, returns a malloc'ed, trimmed
 * string copy of s of size strlen(s)+1. If s does not begin with tkn, returns
 * NULL
 */
char *new_str_after_token(char *s, char *tkn) 
{
        char *ret, *p;

        ret = NULL;
        if (begins_with(s, tkn)) {
                for (p = (s+strlen(tkn)); *p == ' '; p++);
                ret = (char*)malloc(strlen(p)+1);
                strcpy(ret, p);
                for (p = (ret + strlen(ret)-1); *p == '\n'; p--)
                        *p = '\0';
        }

        return ret;
}


int parse_comment(struct commit_node *cn, char *lbuf, FILE *f) {
        char cbuf[MAX_COMMENT_SIZE];
        char *cp, *lp;

        memset(cbuf, '\0', sizeof(cbuf));
        cp = cbuf;
        while (begins_with(fgets(lbuf, MAX_LBUF_SIZE, f), COMMENT_TOKEN))
        {
                lp = lbuf+strlen(COMMENT_TOKEN);
                if (cp-cbuf+strlen(lp) < MAX_COMMENT_SIZE) {
                        memcpy(cp, lp, strlen(lp));
                        cp += strlen(lp);
                }
        }
        cn->comment = new_str_after_token(cbuf, "");  
       
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
