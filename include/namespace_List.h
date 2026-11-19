// complete struct
struct Item
{
	struct Item *next;
	struct Item *prev;
};

// complete struct
struct LinkedList
{
	struct Item *first;
	struct Item *last;
	int count;
};

#if !defined(__LP64__)
_Static_assert(sizeof(struct LinkedList) == 0xC);
#endif
