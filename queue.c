#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

static bool cmp_function(struct list_head *lhs,
                         struct list_head *rhs,
                         bool descend);
static int q_scend(struct list_head *head, bool descend);
int q_merge_two(struct list_head *head1, struct list_head *head2, bool descend);


/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (head)
        INIT_LIST_HEAD(head);

    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;

    element_t *element, *safe;
    list_for_each_entry_safe (element, safe, head, list) {
        list_del(&element->list);
        q_release_element(element);
    }
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *element = malloc(sizeof(element_t));
    if (!element)
        return false;

    size_t len = strlen(s);
    element->value = malloc(len + 1);
    if (!element->value) {
        free(element);
        return false;
    }
    strncpy(element->value, s, len);
    (element->value)[len] = '\0';

    list_add(&element->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    return q_insert_head(head->prev, s);
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head)
        return NULL;
    else if (list_empty(head))
        return NULL;

    element_t *element = list_first_entry(head, element_t, list);
    list_del(&element->list);

    if (sp) {
        size_t len = strlen(element->value);
        if (len > bufsize - 1)
            len = bufsize - 1;
        strncpy(sp, element->value, len);
        sp[len] = '\0';
    }

    return element;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head)
        return NULL;

    return q_remove_head(head->prev->prev, sp, bufsize);
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int size = 0;
    struct list_head *node;
    list_for_each (node, head)
        size++;

    return size;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head)
        return false;
    else if (list_empty(head))
        return false;

    struct list_head *fast = head->next;
    struct list_head *slow = head->next;
    while (fast != head && fast->next != head) {
        fast = fast->next->next;
        slow = slow->next;
    }

    list_del(slow);
    q_release_element(list_entry(slow, element_t, list));
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head)
        return false;

    struct list_head *front = head->next;
    struct list_head *back = front->next;
    while (front != head) {
        element_t *felement = list_entry(front, element_t, list);
        /* Find same-value elements from [front ... back) */
        while (back != head) {
            element_t *belement = list_entry(back, element_t, list);
            if (strcmp(felement->value, belement->value) == 0)
                back = back->next;
            else
                break;
        }

        /* Delete same-value elements if duplicating */
        if (back->prev != front) {
            struct list_head *root = front->prev;
            while (root->next != back) {
                struct list_head *temp = root->next;
                element_t *telement = list_entry(temp, element_t, list);
                list_del(temp);
                q_release_element(telement);
            }
        }

        front = back;
    }

    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head)
        return;
    else if (list_empty(head))
        return;

    struct list_head *node = head->next;
    while (node != head && node->next != head) {
        node = node->next->next;
        struct list_head *first = node->prev->prev;
        struct list_head *second = node->prev;
        list_move(first, second);
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head)
        return;
    else if (list_empty(head))
        return;

    struct list_head *node, *safe;
    list_for_each_safe (node, safe, head) {
        list_move(node, head);
    }
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
    if (!head)
        return;
    else if (list_empty(head))
        return;

    struct list_head *pseudo_head = head;
    struct list_head *node, *safe;
    int count = 0;
    list_for_each_safe (node, safe, head) {
        count = (count + 1) % k;
        if (count == 0) {
            struct list_head *curr = pseudo_head->next;
            struct list_head *next = curr->next;
            for (int i = 0; i < k; ++i) {
                list_move(curr, pseudo_head);
                curr = next;
                next = next->next;
            }
            pseudo_head = safe->prev;
        }
    }
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head)
        return;
    else if (list_empty(head))
        return;
    else if (list_is_singular(head))
        return;

    /* Step 1: Find middle node */
    struct list_head *fast = head->next;
    struct list_head *middle = head->next;
    while (fast != head && fast->next != head) {
        fast = fast->next->next;
        middle = middle->next;
    }

    /* Step 2: Divide head into __head and head. It is important to notice that
     * we have to cut at middle->prev */
    struct list_head __head;
    list_cut_position(&__head, head, middle->prev);

    /* Step 3: Sort __head and head separately */
    q_sort(&__head, descend);
    q_sort(head, descend);

    /* Step 4: Merge __head and head back into head */
    q_merge_two(head, &__head, descend);
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return q_scend(head, false);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return q_scend(head, true);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    if (!head)
        return 0;
    else if (list_empty(head))
        return 0;

    struct list_head trash;
    INIT_LIST_HEAD(&trash);
    while (!list_is_singular(head)) {
        struct list_head *trace = head->next;
        while (trace != head && trace->next != head) {
            trace = trace->next->next;
            queue_contex_t *first =
                list_entry(trace->prev->prev, queue_contex_t, chain);
            queue_contex_t *second =
                list_entry(trace->prev, queue_contex_t, chain);
            first->size = q_merge_two(first->q, second->q, descend);

            second->size = 0;
            list_move(&second->chain, &trash);
        }
    }
    list_splice_tail(&trash, head);
    return list_first_entry(head, queue_contex_t, chain)->size;
}

/* Compare two strings to check if they are in order */
static bool cmp_function(struct list_head *lhs,
                         struct list_head *rhs,
                         bool descend)
{
    element_t *l_element = list_entry(lhs, element_t, list);
    element_t *r_element = list_entry(rhs, element_t, list);

    int result = strcmp(l_element->value, r_element->value);
    if (descend)
        return result >= 0;
    else
        return result <= 0;
}

/* Common function for q_ascend() and q_descend() */
static int q_scend(struct list_head *head, bool descend)
{
    if (!head)
        return 0;
    else if (list_empty(head))
        return 0;

    int count = 0;
    struct list_head *node, *safe;
    list_for_each_safe (node, safe, head) {
        count++;
        while (node->prev != head && !cmp_function(node->prev, node, descend)) {
            struct list_head *target = node->prev;
            list_del(target);
            q_release_element(list_entry(target, element_t, list));
            count--;
        }
    }
    return count;
}

/* Merge two queues into one sorted queue, which is in ascending/descending
 * order. Merged list will be saved in head1, and head2 will be set to NULL.
 * Return: merged list size
 */
int q_merge_two(struct list_head *head1, struct list_head *head2, bool descend)
{
    if (!head1 && !head2)
        return 0;

    struct list_head mhead;
    INIT_LIST_HEAD(&mhead);
    while (!list_empty(head1) && !list_empty(head2)) {
        if (cmp_function(head1->next, head2->next, descend)) {
            list_move_tail(head1->next, &mhead);
        } else {
            list_move_tail(head2->next, &mhead);
        }
    }

    if (list_empty(head1))
        list_splice_tail_init(head2, &mhead);
    else
        list_splice_tail_init(head1, &mhead);

    list_splice_tail_init(&mhead, head1);
    return q_size(head1);
}
