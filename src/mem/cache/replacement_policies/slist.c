#include<stdlib.h>
#include<stddef.h>
#include<assert.h>
#include "slist.h"
#include<stdio.h>
Slist slist_new()
{
    Slist s={NULL,NULL,0};
    return s;
}



static Node* slist_new_node(int32_t element)
{
    Node *new_node=(Node *)malloc(sizeof(Node));
    new_node -> data=element;
    new_node->next=NULL;
    return new_node;
}


Slist* slist_add_head(Slist *list,int32_t element)
{
assert(list!=NULL);
Node *new_node=slist_new_node(element);
new_node->next=list->head;
list->head=new_node;
if(list->tail ==NULL)
    {
        list->tail=new_node;
    }
++list->length;
assert((list->length==1 && list->head==list->tail) || (list->length>0 && list->head!=list->tail));
return list;
}


uint32_t slist_length(Slist *list)
{

        assert(list!=NULL);
        return list->length;

}


Node* slist_lookup(Slist *list,int32_t key)
{

        assert(list!=NULL);
        Node *cur;
        for(cur=list->head;cur!=NULL;cur=cur->next)
        { if(cur->data==key){
          break;

         }
        }
        return (cur);
}


Slist* slist_delete_head(Slist *list)
{
  Node* temp;
  if(list->head!=NULL){
        temp=list->head;
        list->head=list->head->next;
        if(list->head==NULL){
                list->tail=NULL;
        }
        --list->length;
  }free(temp);
  return list;
}


Slist* slist_add_tail(Slist *list,int32_t element)
{
        assert(list!=NULL);
        Node *new_node=slist_new_node(element);
        if(list->tail!=NULL){
                list->tail->next=new_node;
                list->tail=new_node;
        }
        else{
                list->tail=list->head=new_node;
        }
        ++list->length;
        return list;
}


Slist* slist_delete_tail(Slist *list)
{
        assert(list!=NULL);
        Node *cur,*temp;
        if(list->tail!=NULL)
        {
            temp=list->tail;
            for(cur=list->head;cur->next!=temp;cur=cur->next);
            list->tail=cur;
            cur->next=NULL;
            --list->length;
        }
        return list;
}


Slist* slist_repl_head(Slist *list, Node* repl)
{
    assert(list != NULL);
    assert(repl != NULL);

    Node *temp = list->head;

    if (temp != NULL && temp == repl)
    {
        //pass, already head
    }
    else{
    for(temp=list->head;temp!=NULL;temp=temp->next){


        if(temp->next == repl)
        {
            temp->next=repl->next;
	    repl->next = list->head;
	    list->head = repl;
        }
    }
    free(temp);

   }

    return list;
}






