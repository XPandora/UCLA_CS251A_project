#ifndef _INCLUDED_SLIST
#define _INCLUDED_SLIST
#include <stdint.h>

typedef struct _node_ Node;
typedef struct _slist_ Slist;

struct _node_
{
    int32_t data;
    Node *next;
};

struct _slist_
{
    Node *head;
    Node *tail;
    uint32_t length;
};

/* public interface for user*/

/*constructor */
Slist slist_new();

/* return list length*/
uint32_t slist_length(Slist *list);

/*search for node with key data and return node ptr if found, NULL if not*/
Node *slist_lookup(Slist *list, int32_t key);
/*search for a node and delete it from list*/
Slist *slist_look_del(Slist *list, int32_t key);

/*add head and tail*/
Slist *slist_add_head(Slist *list, int32_t element);
Slist *slist_add_tail(Slist *list, int32_t element);
/*delete head and tail*/
Slist *slist_delete_head(Slist *list);
Slist *slist_delete_tail(Slist *list);

/*take Node and put make it the new head*/
Slist *slist_repl_head(Slist *list, Node *repl);
// int32_t slist_min(Slist *list);
// int32_t slist_max(Slist *list);
// Slist* slist_spec_ele(Slist *list, int32_t element,int32_t spec_ele);
// Slist* slist_spec_ele_delete(Slist *list, int32_t spec_ele);
// uint32_t list_compare(Slist *list1,Slist *list2);
// Slist* reverse_list(Slist *list);
// Slist* union_twolist(Slist *union_list,Slist *list1,Slist *list2);
// Slist* intersection_twolist(Slist *intersection_list,Slist *list1,Slist *list2);
// Slist* unique_slist(Slist* list,uint32_t element);
// Slist* slist_display(Slist *list);
#endif
