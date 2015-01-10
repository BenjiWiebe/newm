#ifndef __USERLIST_H_INC_
#define __USERLIST_H_INC_

struct userlist {
	size_t length;
	char **array;
};

struct userlist* ul_create(int initial_length);
void ul_populate(struct userlist *l);
size_t ul_count(struct userlist *l);
char* ul_subtract(struct userlist *bigger, struct userlist *smaller);
void ul_sort(struct userlist *l);
void ul_free(struct userlist *l);

#endif

