#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CACHE_SIZE 5242880

//노드구조체
typedef struct _node{
	int size;
	struct _node* prev;
	struct _node* next;
	char* data;
	char* url;
} node;

//큐 구조체
typedef struct _lruQueue{
	node* header;
	node* trailer;
	int node_count;
	int object_size;
} lruQueue;


node* node_alloc();//node를 하나생성하고 반환한다.
void free_node(node* freeNode);//노드를 free해준다
lruQueue* init_queue();//node header와 trailer를 생성한후 NULL을 담은 큐를 생성한다.
int isEmpty(lruQueue* q);//q가 비었는지 확인하고 1이나 0을 반환한다.

void addFirst(lruQueue* q, node* newNode);//큐의 앞에 노드를 추가한다.
void addLast(lruQueue* q, node* newNode);//큐의 뒤에 노드를 추가한다
node* removeFirst(lruQueue* q);//큐의 앞 노드를 삭제하고 그 노드를 반환한다.
node* removeLast(lruQueue* q);//큐의 뒷 노드를 삭제하고 그 노드를 반환한다.
node* removeNode(lruQueue* q, node* remove_node);//큐의 해당 노드를 삭제하고 그 노드를 반환한다.
node* search(lruQueue* q, char* url);//해당 url을 가지고있는 노드를 확인하고 있으면 lru알고리즘으로 정렬하고, 그 노드를 반환해준다. 없으면 NULL을 반환한다.
node* getFirst(lruQueue* q);//첫번째 노드를 반환한다
void print(lruQueue* q);//큐의 전체를 출력한다

node* node_alloc(){
	node* newNode = (node*)malloc(sizeof(node));
	(newNode->size) = 0;
	(newNode->prev) = (newNode->next) = NULL;
	(newNode->data) = (newNode->url) = NULL;
	return newNode;
}

void free_node(node* freeNode) {
	// url free
	if (freeNode->url != NULL) {
		free(freeNode->url);
	}

	// object free
	if (freeNode->data != NULL) {
		free(freeNode->data);
	}

	free(freeNode);
}

lruQueue* init_queue(){
	lruQueue* newQueue = (lruQueue*)malloc(sizeof(lruQueue));
	(newQueue->header) = node_alloc();
	(newQueue->trailer) = node_alloc();
	(newQueue->header->next) = (newQueue->trailer);
	(newQueue->trailer->prev) = (newQueue->header);
	(newQueue->node_count) = 0;
	(newQueue->object_size) = 0;

	return newQueue;
}

int isEmpty(lruQueue* q){
	return (q->node_count)==0?1:0;
}

void addFirst(lruQueue* q, node* newNode){
	node* tmp  = (q->header->next);
	(tmp->prev) = newNode;
	(newNode->next) = tmp;
	(newNode->prev) = (q->header);
	(q->header->next) = newNode;

	(q->node_count) += 1;
	(q->object_size) += (newNode->size);
}
void addLast(lruQueue* q, node* newNode){
	node* tmp = (q->trailer->prev);
	(tmp->next) = newNode;
	(newNode->prev) = tmp;
	(newNode->next) = (q->trailer);
	(q->trailer->prev) = newNode;

	//큐의 노드의 갯수 증가와 데이터 크기 증가
	(q->node_count) += 1;
	(q->object_size) += (newNode->size);
}

node* removeFirst(lruQueue* q){
	if(isEmpty(q)) return NULL;

	node* remove_node = (q->header->next);
	(q->header->next) = (remove_node->next);
	(remove_node->next->prev) = (q->header);

	(remove_node->next) = (remove_node->prev) = NULL;

	(q->node_count) -= 1;
	(q->object_size) -= (remove_node->size);

	return remove_node;
}
node* removeLast(lruQueue* q){
	if(isEmpty(q)) return NULL;

	node* remove_node = (q->trailer->prev);
	(q->trailer->prev) = (remove_node->prev);
	(remove_node->prev->next) = (q->trailer);

	(remove_node->next) = (remove_node->prev) = NULL;

	(q->node_count) -= 1;
	(q->object_size) -= (remove_node->size);

	return remove_node;
}

node* removeNode(lruQueue* q, node* remove_node){
	(remove_node->prev->next) = (remove_node->next);
	(remove_node->next->prev) = (remove_node->prev);

	(remove_node->next) = (remove_node->prev) = NULL;

	(q->node_count) -= 1;
	(q->object_size) -= (remove_node->size);

	return remove_node;
}


node* search(lruQueue* q, char* url){
	node* tmp = (q->header->next);

	while(tmp != (q->trailer)){
		if(strcmp((tmp->url), url) == 0){
			tmp = removeNode(q, tmp);
			addFirst(q, tmp);
			return tmp;
		}
		tmp = (tmp->next);
	}
	return NULL;
}
node* getFirst(lruQueue* q){
	return q->header->next;
}

void print(lruQueue* q){
	printf("%s%s%s",
		"\n************************************************\n",
		"|              < Web Cache Status >            |\n",
		"------------------------------------------------\n\n");

	node* tmp = (q->header->next);
	
	int i = 0;

	while(tmp != (q->trailer)){
		printf("|[%2d]URL:%s\n|    (%7d bytes)\n\n", i, (tmp->url), (tmp->size));
		tmp = (tmp->next);
		i++;
	}

	printf("\n------------------------------------------------\n|%20s / %-23s|\n|%20d / %-23d|\n************************************************\n\n", "Current Size", "MAX SIZE", (q->object_size), MAX_CACHE_SIZE);
}


