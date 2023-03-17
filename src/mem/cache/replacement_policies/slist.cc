#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "slist.hh"
#include <stdio.h>
Slist slist_new()
{
    // Slist s = {NULL, NULL, 0};
    Slist s = _slist_();
    return s;
}

static Node *slist_new_node(BlockPA element)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->data = element;
    new_node->next = NULL;
    return new_node;
}

Slist *slist_add_head(Slist *list, BlockPA element)
{
    assert(list != NULL);
    Node *new_node = slist_new_node(element);
    new_node->next = list->head;
    list->head = new_node;
    if (list->tail == NULL)
    {
        list->tail = new_node;
    }
    ++list->length;
    assert((list->length == 1 && list->head == list->tail) || (list->length > 0 && list->head != list->tail));
    return list;
}

uint32_t slist_length(Slist *list)
{

    assert(list != NULL);
    return list->length;
}

Node *slist_lookup(Slist *list, BlockPA key)
{

    assert(list != NULL);
    Node *cur;
    for (cur = list->head; cur != NULL; cur = cur->next)
    {
        if (cur->data == key)
            return (cur);
    }

    return (NULL);
}

Slist *slist_look_del(Slist *list, BlockPA key)
{
    assert(list != NULL);
    if (list->length == 0)
        return list;

    Node *temp, *prev;
    prev = NULL;
    temp = list->head;
    assert(temp != NULL);
    if (list->length == 1)
    {
        if (key == temp->data)
        {
            list->head = NULL;
            list->tail = NULL;
            list->length = 0;
            free(temp);
        }
    }
    else if (temp->data == key)
    {
        list->head = temp->next;
        free(temp);
        --list->length;
    }
    else
    {
        for (temp = list->head; temp != NULL; temp = temp->next)
        {
            if (temp->data == key)
            {
                assert(prev != NULL);
                prev->next = temp->next;
                if(temp == list->tail)
                    list->tail = prev;

                break;
            }
            prev = temp;
        }
        free(temp);
        --list->length;
    }

    return list;
}

Slist *slist_delete_head(Slist *list)
{
    Node *temp;
    if (list->head != NULL)
    {
        temp = list->head;
        list->head = list->head->next;
        if (list->head == NULL)
        {
            list->tail = NULL;
        }

        free(temp);
        --list->length;
    }
    return list;
}

Slist *slist_add_tail(Slist *list, BlockPA element)
{
    assert(list != NULL);
    Node *new_node = slist_new_node(element);
    if (list->tail != NULL)
    {
        list->tail->next = new_node;
        list->tail = new_node;
    }
    else
    {
        list->tail = list->head = new_node;
    }
    ++list->length;
    return list;
}

Slist *slist_delete_tail(Slist *list)
{
    assert(list != NULL);
    Node *cur, *temp;
    if (list->tail != NULL)
    {
        assert(list->head != NULL);
        temp = list->tail;
        if (list->head == list->tail)
        {
            list->head = NULL;
            list->tail = NULL;
            assert(list->length == 1);
        }
        else
        {
            for (cur = list->head; cur->next != temp; cur = cur->next)
                ;
            list->tail = cur;
            cur->next = NULL;
        }
        free(temp);
        --list->length;
    }
    return list;
}

Slist *slist_repl_head(Slist *list, Node *repl)
{
    assert(list != NULL);
    assert(repl != NULL);
    assert(list->head != NULL);

    Node *temp = list->head;

    if (temp != NULL && temp == repl)
    {
        // pass, already head
        return list;
    }
    else
    {
        for (temp = list->head; temp != NULL; temp = temp->next)
        {
            if (temp->next == repl)
            {
                if (repl == list->tail)
                {
                    list->tail = temp;
                    temp->next = NULL;
                    assert(repl->next == NULL);
                }
                else
                    temp->next = repl->next;

                repl->next = list->head;
                list->head = repl;

                break;
            }
        }
    }

    return list;
}
