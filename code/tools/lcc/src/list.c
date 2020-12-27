#include "c.h"


static List freenodes;		/* free list nodes */

/* append - append x to list, return new list */
List append(void *x, List list) {
	List new;

	if ((new = freenodes) != NULL)
		freenodes = freenodes->link;
	else
		NEW(new, PERM);
	if (list) {
		new->link = list->link;
		list->link = new;
	} else
		new->link = new;
	new->x = x;
	return new;
}

/* length - # elements in list */
int length(List list) {
	int n = 0;

	if (list) {
		List lp = list;
		do
			n++;
		while ((lp = lp->link) != list);
	}
	return n;
}

/* ltov - convert list to a NULL-terminated vector allocated in arena */
void *ltov(List *list, unsigned arena) {
	int i = 0;
	void **array = newarray(length(*list) + 1, sizeof array[0], arena);

	if (*list) {
		List lp = *list;
		do {
			lp = lp->link;
			array[i++] = lp->x;
		} while (lp != *list);
#ifndef PURIFY
		lp = (*list)->link;
		(*list)->link = freenodes;
		freenodes = lp;
#endif
	}
	*list = NULL;
	array[i] = NULL;
	return array;
}
