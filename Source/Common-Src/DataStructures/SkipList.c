#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "SkipList.h"
#include "Utilities.h"
#include "Person.h"

#define PROBABILITY 0.6 // Probility of a node to have one more level in the skip list (has to be between [0,1])

// Each node of the skip-list has a tuple value (A person and (possibly) a vaccination date)
// and a (dynamically allocated) arrey of pointers to the next node of each level (NULL if there is no next)
typedef struct listNode{
	Person *person;
	char *date;
	struct listNode **nextArrey;
	int nextArreyHeight; // The size of the above arrey (or max height of this node)
} listNode;

typedef struct SkipList{ //The skip-list data structure has:
	struct listNode **levelArrey; // An arrey which points to the first node of each level
	int height; // The maximum height of the above arrey
	int (*cmpfun)(void *, void *); // And a function that compares the item of the nodes (a person in our case)
} SkipList;

// Frees the memory a node of the skip-list is occupying
void freeSkipNode(listNode* node){
	free(node->nextArrey);
	if(node->date!=NULL) free(node->date);
	// The persons are saved on a hashtable which frees the memory each person has allocated
	// deletePerson(node->person); So we should not also free the person memory here
	free(node);
}

// Simply "flips a coin" and returns heads (0) or tails (1) based on the defined PROBABILITY
int flipCoin(){
	static char srandInit=0;
	// Due to the static nature of the srandInit variable the srand will be initialized only once
	if(srandInit==0) {srandInit++; srand(time(NULL));}

	return !( ( (float)rand()/(float)RAND_MAX ) > PROBABILITY );
}

// Function that cretes a skip-list data structure and returns a pointer to it
SkipList* SkipListInitialize(unsigned int height,int (*cmpfun)(void *, void *)){
	SkipList* listS = malloc(sizeof(SkipList));// Allocating memory for the skip-list structure

	// Creating the table which will hold the first node of each level
	listS->levelArrey = malloc(sizeof(listNode*)*height);
	listS->height = height; // Note that the skip-list can have at most "height" level (acceptable implementation)

	for(int i=0; i<listS->height; i++){// An empty list has no nodes, and thus all levels will not point anywhere
		listS->levelArrey[i] = NULL;
	}

	listS->cmpfun=cmpfun; // Initializing the compare function

	return listS;
}

// Checks if a given skip-list has no elements
int CheckEmpty(SkipList* listS){
	return listS->levelArrey[0]!=NULL? 0 : 1; // A skip-list is empty when there are no nodes at the first level
}

// This function searches for an node with the given key, while it save the path of nodes it visited on an arrey
int searchNodeWithPath(SkipList* listS,char *IdKey,listNode **result,listNode **levelPath,int *levelPathIndex,int stopIfFound){
	int cmp,level=listS->height - 1;
	listNode *curIndex=NULL;

	if(levelPath!=NULL){// If an arrey was given to save the nodes which were visited
		for(int i=0; i<listS->height; i++)// It is emptied, since it will be filled in the traversing procces
			levelPath[i]=NULL;
	}

	//Getting the most north west elememnt, which also is the starting point of the search
	for(level = listS->height - 1; level>=0; level--){
		if( listS->levelArrey[level]!=NULL ){
			// The most north west element from which we will start searching
			// is the highest node smaller or equal to the node we are searching
			if(listS->cmpfun(listS->levelArrey[level]->person->id,IdKey) <= 0 || level==0){
				curIndex = listS->levelArrey[level];
				break;// Breaking from the loop, most north west node found 
			}
		}
	}

	*result=NULL;
	if(curIndex==NULL) {// If there was no most north west elememnt even at level 0
		return -1;// The list is empty
	}else if( listS->cmpfun(curIndex->person->id,IdKey) == 0 ){// Otherwise if the most north west elememnt happens to be the one we search
		*result = curIndex; // We save the memory location of the node
		if(stopIfFound==1) return 0; // And we keep searching only if it is requested (usually in order to fill the levelPathIndex)
		if(curIndex==listS->levelArrey[level] && level>0) curIndex=listS->levelArrey[level-1];// Going a level lower if needed
		if(levelPath!=NULL) levelPath[level]=curIndex;// Saving the just traversed node
		level--;
	}

	while(level>=0){// Searching in all the posible levels
		if(curIndex->nextArrey[level]!=NULL){// If there are nodes left in this level after the current node
			cmp = listS->cmpfun(curIndex->nextArrey[level]->person->id,IdKey);
			if(cmp < 0){// If this node is smaller than the one we search
			
				curIndex = curIndex->nextArrey[level];// We are traversing the arrey at the same level

			}else if(cmp > 0){// If the node is bigger than the one we search

				if(levelPath!=NULL) levelPath[level]=curIndex;// Then we have to save the node to the path
				// And then go one level lower
				if(stopIfFound==0 && curIndex==listS->levelArrey[level] && level>0) curIndex=listS->levelArrey[level-1];
				level--;

			}else{// If we just found the node we were searching for

				*result = curIndex->nextArrey[level];// It's memory location is saved
				if(stopIfFound==1) return 0;// And we stop now that we found the node if it was requested to do so
				if(levelPath!=NULL) levelPath[level]=curIndex; // Otherwise we save the current node to the path and keep traversing
				level--;

			}

		}else{//If there are no next nodes after the current one, we have to check the current one once again

			//Evrything is done as above
			cmp = listS->cmpfun(curIndex->person->id,IdKey);
			if(cmp < 0){
				if(levelPath!=NULL) levelPath[level]=curIndex;
				level--;
			}else if(cmp > 0){
				level--;
			}else{
				*result = curIndex->nextArrey[level];
				if(stopIfFound==1) return 0;
				if(curIndex==listS->levelArrey[level] && level>0) curIndex=listS->levelArrey[level-1];
				if(levelPath!=NULL) levelPath[level]=curIndex;
				level--;
			}

		}
	}

	if(stopIfFound==1){// If we traversed the whole skip list, and only had to find the node
	// We simply did not find it and we return the last node along with an error code
		*result = curIndex;
		return 1;
	}else{// Otherwise the important part is to have filled the levelPath[]
	// So we simply return the first node of the skip list as the result
		if(*result==NULL) *result=listS->levelArrey[0];
		return listS->cmpfun((*result)->person->id,IdKey)!=0;
	}

}

// Searches for the tuple values of a node (person and date), given it's id and the skiplist
// It simply utilizes the above function in an efficient way, 
// while preseting this functionality of the skip-list in a simply way for the "outside world"
int SkipListSearch(SkipList* listS,char *IdKey,Person** person, char** date){
	if(listS==NULL) return -1;
	listNode *vaccination;
	// The first 3 arguments are trivial (the SL, the search key, and a pointer to the possible founded node),
	// The 4th and 5th signify that we don't want the path of the travesal to be saved (since it's currently useless)
	// The 5th argument makes sure that the searching will stop once the node is found
	switch (searchNodeWithPath(listS,IdKey,&vaccination,NULL,NULL,1)){
		case 0:// The node with the given id was found
			// So the tuple values are places to the callers variables
			*person = vaccination->person;
			*date = vaccination->date;
			return 1; // Found return code
		default:// The node was not found for whatever reason
			return 0; // Not found return code
	}
}

// Creates a new listNode and inserts to a the right position, while also creating a radrom height for the node
int SkipListInsert(SkipList* listS,Person *person,char *date){
	if(listS==NULL) return -2;
	listNode *curIndex,*newNode,**levelPath=malloc(sizeof(listNode *)*(listS->height));//The leval path takes exactly height values
	int cmp,search,levelPathIndex=0,newNodeHeight=0,i=0;

	// The searchNodeWithPath finds the sequence of nodes which next pointers will have to be changed to insert the new node
	search = searchNodeWithPath(listS,person->id,&curIndex,levelPath,&levelPathIndex,1);
	if(search==0){// A return value of 0 means that that a node with the same id was found in the skip list
		free(levelPath);
		return -1; // That is a duplicate and cannot be inserted to the skiplist (since every id is unique)
	}
	
	// Flipping coins to probabilistically calculate the hight of the new node, which cannot exceed the maximum hight of the SL
	while(flipCoin()==1 && newNodeHeight<listS->height-1){
		newNodeHeight++;
	}

	// Creating the new node and copying person and date values
	newNode = malloc(sizeof(listNode));
	newNode->person=person;
	if(date != NULL){
		newNode->date=strdup(date);
	}else{
		newNode->date=NULL;
	}
	// Allocating an arrey of next nodes just enough for the hight of the new node
	newNode->nextArrey=malloc(sizeof(listNode*)*(newNodeHeight+1));
	newNode->nextArreyHeight=newNodeHeight;
	// Initializing all the next pointers to NULL
	for(i=0; i<=newNodeHeight; i++){
		newNode->nextArrey[i] = NULL;
	}

	for(i=0; i<=newNodeHeight; i++){ // From level 0, to the max hight of the new node
		if(levelPath[i] != NULL){ // If there are also other nodes in this level

			if(levelPath[i]!=listS->levelArrey[i]){// If the node has not to be inserted first
				// The new node is inserted after the node which was in the travesal path for this level
				newNode->nextArrey[i] = levelPath[i]->nextArrey[i];
				levelPath[i]->nextArrey[i] = newNode;
			}else{// If the new node has to be inserted first
				cmp = listS->cmpfun(listS->levelArrey[i]->person->id,newNode->person->id);
				// It is checked whether the new node has to be inserted first for this level, or after the first node of this level
				if(cmp < 0){
					newNode->nextArrey[i] = listS->levelArrey[i]->nextArrey[i];
					listS->levelArrey[i]->nextArrey[i] = newNode;
				}else if(cmp > 0){
					newNode->nextArrey[i] = listS->levelArrey[i];
					listS->levelArrey[i] = newNode;
				}else{
					printf("skip list fatal error!!\n"); //No duplicates should exist this far, otherwise something went teribly wrong
				}
			}
		}else{// If there are no nodes at this level the node is simply inserted first
			newNode->nextArrey[i] = listS->levelArrey[i]; //listS->levelArrey[i] = NULL anyways
			listS->levelArrey[i] = newNode;
		}
	}

	free(levelPath);// Freeing the dynamically allocated path arrey
	return 0;
}

// Deletes the node with the given Id (if it exists), while also freeing all of the nodes memory
int SkipListDelete(SkipList* listS,char *IdKey){
	if(listS==NULL) return -3;
	listNode *result,**prevNodes=malloc(sizeof(listNode *)*(listS->height));//The prev nodes takes exactly height values
	int cmp,level,prevNodesIndex=0;

	// Utilising the searchNodeWithPath in order to finds the sequence of nodes which next pointers will have to be changed
	if((cmp=searchNodeWithPath(listS,IdKey,&result,prevNodes,&prevNodesIndex,0))!=0){
		// If the node was not found, then there is nothing to be deleted
		free(prevNodes);
		return cmp; // A number signifying why the node was not found is returned
	}

	for(level=0; level<=result->nextArreyHeight; level++){//For all the levels
		//The pointers structure is changes in such a way that the deleted node won't be a part of it, but the SL will still be sorted
		if(listS->levelArrey[level]!=result)
			prevNodes[level]->nextArrey[level]=result->nextArrey[level]; // The deleted node was not the first at this level case
		else
			listS->levelArrey[level]=result->nextArrey[level]; // The deleted node was the first at this level case
	}
	freeSkipNode(result);// Freeing the memory the node has allocated

	free(prevNodes);// Freeing the dynamically allocated path arrey
	return 0;
}

// Traverses the skip-list data structure and calls fun for each node it comes across
void SkipListTraverse(SkipList* listS, void (*fun)(void* person)){
	listNode *index;
	// The traversal of all the nodes of the skip-list can be done by traveresing only level[0]
	index = listS->levelArrey[0];
	while(index!=NULL){
		fun(index->person);// Calling fun for each node
		index = index->nextArrey[0];
	}
}

// This function prints the skip list structure along with the id value of each node (used for debugging purposes)
void SkipListPrint(SkipList *SL){
	listNode *index;
	for(long i=SL->height-1; i>=0; i--){
		printf("level %ld: ",i);
		index = SL->levelArrey[i];
		while(index!=NULL){
			printf("[%s]->",index->person->id);
			index = index->nextArrey[i];
		}
		printf("\n");
	}
	printf("\n");
}

// Frees all the memory the data structure used
int SkipListDestroy(SkipList *SL){
	listNode *index,*indexToDelete;
	index = SL->levelArrey[0];
	//In order to free all the memory allocated from the skip-list
	while(index!=NULL){
		// We first start by freeing the memory of the nodes
		indexToDelete = index;
		index = index->nextArrey[0];
		freeSkipNode(indexToDelete);
	}
	free(SL->levelArrey); // Then the memory of the first nodes at each level
	free(SL); // And at last tThe memory the struct of the structure was using
	return 0;
}
