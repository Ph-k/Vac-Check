/* Code from https://github.com/Ph-k/Vac-Check. Philippos Koumparos (github.com/Ph-k)*/
typedef struct SkipList SkipList;
typedef struct Person Person;

typedef struct PersonVaccination{//Used anywhere?
    Person* person;
    char *date;
}PersonVaccination;

typedef struct Abacus Abacus;//fd

// Function that cretes a skip-list data structure and returns a pointer to it
SkipList* SkipListInitialize(unsigned int height,int (*cmpfun)(void *, void *));

// Searches for the tuple values of a node (person and date), given it's id and the skiplist
int SkipListSearch(SkipList* listS,char *IdKey,Person** person, char** date);

// Creates a new listNode and inserts to a the right position, while also creating a radrom height for the node
int SkipListInsert(SkipList* listS,Person *person,char* date);

// Deletes the node with the given Id (if it exists), while also freeing all of the nodes memory
int SkipListDelete(SkipList* listS,char *IdKey);

// Traverses the skip-list data structure and calls fun for each node it comes across
void SkipListTraverse(SkipList* listS, void (*fun)(void* person));

// This function prints the skip list structure along with the id value of each node (used for debugging purposes)
void SkipListPrint(SkipList *SL);

// Frees all the memory the data structure used
int SkipListDestroy(SkipList* listS);
