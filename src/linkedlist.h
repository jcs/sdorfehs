/* This file was taken from the Linux kernel and is
 * Copyright (C) 2003 Linus Torvalds
 *
 * Modified by Shawn Betts. Portions created by Shawn Betts are
 * Copyright (C) 2003, 2004 Shawn Betts
 *
 * This file is part of ratpoison.
 *
 * ratpoison is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * ratpoison is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 */

#ifndef _RATPOISON_LINKLIST_H
#define _RATPOISON_LINKLIST_H

#include <string.h>

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head {
        struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
        struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/* Prototypes of C functions. */
int list_size (struct list_head *list);
void list_splice_init(struct list_head *list,
                      struct list_head *head);

void list_splice_init(struct list_head *list,
                      struct list_head *head);

void list_splice(struct list_head *list, struct list_head *head);

void __list_splice(struct list_head *list,
                   struct list_head *head);

int list_empty(struct list_head *head);

void list_move_tail(struct list_head *list,
                    struct list_head *head);

void list_move(struct list_head *list, struct list_head *head);

void list_del_init(struct list_head *entry);
void list_del(struct list_head *entry);
void __list_del(struct list_head * prev, struct list_head * next);
void list_add_tail(struct list_head *new, struct list_head *head);
void list_add(struct list_head *new, struct list_head *head);
void __list_add(struct list_head *new,
                struct list_head *prev,
                struct list_head *next);

#ifdef HAVE___BUILTIN_PREFETCH
#define prefetch(x) __builtin_prefetch(x)
#else
#define prefetch(x) ((void)(x))
#endif

/* Return the last element in the list. */
#define list_last(last, head, member)                           \
{                                                               \
  last = list_entry((head)->prev, typeof(*last), member);       \
  if (&last->member == (head))                                  \
    last = NULL;                                                \
}


/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

/**
 * list_entry - get the struct for this entry
 * @ptr:        the &struct list_head pointer.
 * @type:       the type of the struct this is embedded in.
 * @member:     the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)


/**
 * __list_for_each      -       iterate over a list
 * @pos:        the &struct list_head to use as a loop counter.
 * @head:       the head for your list.
 *
 * This variant differs from list_for_each() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define list_for_each(pos, head) \
        for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev   -       iterate over a list backwards
 * @pos:        the &struct list_head to use as a loop counter.
 * @head:       the head for your list.
 */
#define list_for_each_prev(pos, head) \
        for (pos = (head)->prev, prefetch(pos->prev); pos != (head); \
        pos = pos->prev, prefetch(pos->prev))

/**
 * list_for_each_safe   -       iterate over a list safe against removal of list entry
 * @pos:        the &struct list_head to use as a loop counter.
 * @n:          another &struct list_head to use as temporary storage
 * @head:       the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)

#define list_for_each_safe_entry(item, pos, n, head, member) \
        for (pos = (head)->next,  \
             item = list_entry(pos, typeof(*item), member), \
             n = pos->next  \
                     ; \
             pos != (head) \
                     ; \
             pos = n,  \
             item = list_entry(pos, typeof(*item), member), \
             n = pos->next) \

/**
 * list_for_each_entry  -       iterate over list of given type
 * @pos:        the type * to use as a loop counter.
 * @head:       the head for your list.
 * @member:     the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)                          \
        for (pos = list_entry((head)->next, typeof(*pos), member),      \
                     prefetch(pos->member.next);                        \
             &pos->member != (head);                                    \
             pos = list_entry(pos->member.next, typeof(*pos), member),  \
                     prefetch(pos->member.next))

#define list_for_each_entry_safe(pos, n, head, member)                  \
        for (pos = list_entry((head)->next, typeof(*pos), member),      \
                n = list_entry(pos->member.next, typeof(*pos), member); \
             &pos->member != (head);                                    \
             pos = n,                                                   \
                n = list_entry(pos->member.next, typeof(*pos), member))

#define list_direction_entry(pos, head, member, direction) \
({ \
        typeof(pos) ret = NULL;  \
        struct list_head *a_head = head;  \
        if (pos->member.direction == a_head) { \
                        ret = list_entry(a_head->direction,  \
                                         typeof(*pos), member); \
        } else { \
                ret = list_entry(pos->member.direction,  \
                                 typeof(*pos), member); \
        } \
        ret; \
})

#define list_next_entry(pos, head, member) \
        list_direction_entry(pos, head, member, next)

#define list_prev_entry(pos, head, member) \
        list_direction_entry(pos, head, member, prev)

#define list_for_each_entry_prev(pos, head, member)                     \
        for (pos = list_entry((head)->prev, typeof(*pos), member),      \
                     prefetch(pos->member.prev);                        \
             &pos->member != (head);                                    \
             pos = list_entry(pos->member.prev, typeof(*pos), member),  \
                     prefetch(pos->member.prev))

#endif


/* Return the first element in the list. */
#define list_first(first, head, member)                         \
{                                                               \
  first = list_entry((head)->next, typeof(*first), member);     \
  if (&first->member == (head))                                 \
    first = NULL;                                               \
}

void list_sort(void *priv, struct list_head *head,
               int (*cmp)(void *priv, struct list_head *a,
                          struct list_head *b));
