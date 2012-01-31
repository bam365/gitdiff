/* commitlist.h - Parsing git log's output into a data structure
 * Blake Mitchell, 2012
 */

#ifndef COMMITLIST_H
#define COMMITLIST_H

#include <stdio.h>


#define COMMIT_HASH_SIZE  40


struct commit_node {
        char hash[COMMIT_HASH_SIZE];
        char *date;
        char *author;
        char *comment;
        struct commit_node *prev;
        struct commit_node *next;
};

typedef struct commit_node* commit_list;


commit_list parse_commit_list(FILE *f);
void free_commit_list(commit_list *cl);



#endif