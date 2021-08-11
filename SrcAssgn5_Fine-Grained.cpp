#include <iostream>
#include <thread>
#include <fstream>
#include <random>
#include <mutex>
using namespace std;

struct node {   //each node of graph 
    int vertex;
    int type;   //0 for internal vertex, 1 for external vertex
    int partitionIndex; //means this vertex lies in which partition
    node *next;     //pointer to next node of class
};

struct list{    //used for partition lists
    struct node *vertex; 
    list *next;
};

class Info {
    public:
        struct node **AdjList;  //pointer to array AdjList[n+1]
        struct list **Partition;    //pointer to array Partition[k+1]
        int *color; //pointer to array color[n+1]
        bool *available;    //pointer to array available[k+1]
};

int n, k;
mutex *mtx;
list *ExternalVerticesList;     //linked list of all external vertices
ofstream output("output_Fine-Grained.txt");

node* CreateListNode(int vertex) {  //creates node of graph
    node *temp = new node;
    temp->vertex = vertex;
    temp->next = NULL;
    return temp;
}

node* InsertEdge(node *head, int data) {    //inserting into adjacency list
    if (head == NULL)   
    	head = CreateListNode(data);
    else {
        if(data >= head->vertex)
            head->next = InsertEdge(head->next, data);		//recursive call
        else {
            node *temp = CreateListNode(data);
            temp->next = head;
            head = temp;
        }
    }
    return head;
}

list *insert(list *head, node *AdjList) {   //insert AdjList node into head 
    if(head == NULL) {
        head = new list;    //memory allocation
        head->vertex = AdjList;
        head->next = NULL;
    }
    else    
        head->next = insert(head->next, AdjList);   //recursive call
    return head;    //returning head
}

list *getExternalVertices(node *AdjList[], int n) { //getting all external vertices of graph
    list *head = NULL;
    for(int i=1; i<n+1; i++) {
        node *temp = AdjList[i]->next;
        while(temp!=NULL) {
            if(AdjList[i]->partitionIndex != AdjList[temp->vertex]->partitionIndex) {   //if both don't have common partitionIndex, it implies that both lies in differnet partitions
                AdjList[i]->type = 1;   //so setting its type to 1, means it's an external vertex
                break;
            }
            else
                AdjList[i]->type = 0;   //means it's an internal vertex

            temp = temp->next;
        }
    }

    for(int i=1; i<n+1; i++) {
        if(AdjList[i]->type)
            head = insert(head, AdjList[i]);    //inserting it all external vertices into linked list
    }
    return head;    //returning list
}

node* InsertNode(node *head, int vertex, int type, int partitionIndex) {    //inserting into adjacency list
    if (head == NULL) {
        head = new node;
        head->vertex = vertex;
        head->type = type;
        head->partitionIndex = partitionIndex;
        head->next = NULL;
    }
    	
    else {
        if(vertex >= head->vertex)
            head->next = InsertNode(head->next, vertex, type, partitionIndex);		//recursive call
        else {
            node *temp = new node;
            temp->vertex = vertex;
            temp->type = type;
            temp->partitionIndex = partitionIndex;
            temp->next = head;
            head = temp;
        }
    }
    return head;
}

int sizeOf(node *head) {
    int ctr=0;
    while(head!=NULL) {
        ctr++;
        head = head->next;
    }
    return ctr;
}

void fineGrained(Info object, int i) {    //using coarse grained mechanism
    list *Trav1 = object.Partition[i];  //for traversing through Partitions(in case of internal vertices)
    list *Trav2 = object.Partition[i];  //for traversing through Partitions(in case of external vertices)

    while(Trav1 != NULL) {
        node *v = Trav1->vertex;
        if(!v->type) {  //means it's an internal vertex
            node *temp = v->next;   //pointer to adj list, to which v is pointing
            while(temp!=NULL) {
                if(object.color[temp->vertex] != -1) 
                    object.available[object.color[temp->vertex]] = false;
                temp = temp->next;
            }

            for(int j=1; j<n+1; j++) {  //finding first available color
                if(object.available[j] == true) {
                    object.color[v->vertex] = j;    //assigning color
                    break;
                }    
            }
            
            temp = v->next; 
            while(temp!=NULL) {     //resetting values back to true(i.e. availablity is true)
                if(object.color[temp->vertex] != -1)
                    object.available[object.color[temp->vertex]] = true;
                temp = temp->next;
            }
        }
        Trav1 = Trav1->next;
    }

    while(Trav2 != NULL) {
        node *v = Trav2->vertex;
        if(v->type == 1) {   //means it's an external vertex
            node *ExternalVertices = NULL;  //storing all external/boundary vertices near to v
            node *CurrentlyLockedVertices = NULL;   //storing currently locked vertices
            node *temp = v->next;   

            while(temp != NULL) {
                if(object.AdjList[temp->vertex]->type == 1) {
                    ExternalVertices = InsertNode(ExternalVertices, temp->vertex, temp->type, temp->partitionIndex);    //inserting into externa; vertices linked list
                }
                temp = temp->next;
            }
            ExternalVertices = InsertNode(ExternalVertices, v->vertex, v->type, v->partitionIndex);            
            int locksCTR = 0;

            while (1){//getting all locks for neighbours and vertex and if the vertex gets all locks then only continue, otherwise try to het all locks
                node *temp2 = ExternalVertices;
                while(temp2 != NULL) {
                    if(mtx[temp2->vertex].try_lock()) {
                        locksCTR++;
                        CurrentlyLockedVertices = InsertNode(CurrentlyLockedVertices, temp2->vertex, temp2->type, temp2->partitionIndex);
                    }

                    else {
                        locksCTR = 0;
                        while(CurrentlyLockedVertices != NULL) {    //releasing all locks to avoid deadlock situation
                            mtx[CurrentlyLockedVertices->vertex].unlock();
                            CurrentlyLockedVertices = CurrentlyLockedVertices->next;
                        }
                        break;
                    }
                    temp2 = temp2->next;
                }
                if(locksCTR == sizeOf(ExternalVertices)) 
                    break;
            }

            temp = v->next;   //pointer to adj list, to which v is pointing
            while(temp!=NULL) {
                if(object.color[temp->vertex] != -1) 
                    object.available[object.color[temp->vertex]] = false;
                temp = temp->next;
            }

            for(int j=1; j<n+1; j++) {  //finding first available color
                if(object.available[j] == true) {
                    object.color[v->vertex] = j;    //assigning color
                    break;
                }    
            }
            
            temp = v->next; 
            while(temp!=NULL) {     //resetting values back to true(i.e. availablity is true)
                if(object.color[temp->vertex] != -1)
                    object.available[object.color[temp->vertex]] = true;
                temp = temp->next;
            }
            
            while(CurrentlyLockedVertices != NULL) {
                mtx[CurrentlyLockedVertices->vertex].unlock();
                CurrentlyLockedVertices = CurrentlyLockedVertices->next;
            }
            CurrentlyLockedVertices = NULL;
        }
        Trav2 = Trav2->next;
    }
}

int main(void) {
    ifstream input;
    input.open("input_params.txt");
    input>> k>> n;  //k threads/partition , n vertices
    
    /* below i have declared as n+1 and k+1 moslty for easiness, like vertices from 1 to n instead of 0 to n-1, 
    also threads and partition from 1 to k instead of 0 to k-1 */
    node* AdjList[n+1]; //n+1 pointers of type node(gonna use only 1 to n), each pointer points to the adjacency list associated with that index
    list* Partition[k+1]; //k+1 partitions(gonna use only 1 to k), each partition list's vertex pointer points to the linked list of vertices lying in that partition
    int color[n+1];   //n+1 colors(gonna use only 1 to n), where color[i] means color of ith vertex from graph(there also vertex numbers are from 1 to n)
    bool available[n+1];      //n+1 availability checks for n+1 colors... here also i'm just using 1 to n
    mtx = new mutex[n+1];   //mutex locks
    chrono::time_point<chrono::system_clock> startTime,endTime;

    for(int i=1; i<n+1; i++)    
        AdjList[i] = CreateListNode(i); //assigning each AdjList
    for(int i=1; i<k+1; i++)
        Partition[i] = NULL;    //initialising it to NULL

    Info object;    //will pass into thread function namely coarseGrained
    object.AdjList = AdjList;
    object.Partition = Partition;
    object.color = color;
    object.available = available;

    int i=1, j=0;
    while(!input.eof()) {   //taking input from the graph
        int data;
        input>> data;
        j++;
        if(data==1)
            AdjList[i]->next = InsertEdge(AdjList[i]->next, j);
        
        if(j==n) {
            j=0;
            i++;
        }
    }

    srand(time(NULL));
    for(int i=1; i<n+1; i++) {
        int r = rand()%(k) + 1; //random number between 1 to k
        //cout<< "r: "<< r<< " i: "<< i<< endl;
        Partition[r] = insert(Partition[r], AdjList[i]);    //inserting a vertex into randomly generated partition
        AdjList[i]->partitionIndex = r;     //set partitionIndex equal to r, will use it in future
    }

    ExternalVerticesList = getExternalVertices(AdjList, n);     //linked list off all external vertices of graph
    for(int i=0; i<n+1; i++) {
        color[i] = -1;      //setting color initially to -1 (means no color)
        available[i] = true;    //setting avaible to true means color[i] is available and you can use it 
    }

    startTime = chrono::system_clock::now();    //noting start time
    vector<thread> Thread;  //vector of threads
    for(int i=0; i<k; i++) 
        Thread.push_back(thread(fineGrained, object, i+1));   //pusing info object which has complete info

    for(int i=0; i<k; i++) 
        Thread[i].join();   //joining all threads
    endTime = chrono::system_clock::now();      //noting end time

    output<< "Fine Lock"<< endl;
    int maxColor=0;
    for(int i=1; i<n+1; i++) {  //calculating numver of colors used
        if(maxColor < color[i])
            maxColor = color[i];
    }
    output<< "No. of colors used: "<< maxColor<< endl;
    output<< "Time taken by algorithm using fine grained lock is: "<< chrono::duration_cast <chrono::microseconds> (endTime-startTime).count()<< " microseconds"<< endl;
    output<< endl<< "Colors: "<< endl;
    for(int i=1; i<n+1; i++)
        output<< "V"<< i<< "-"<< color[i]<<", ";
    output<< endl;
}