/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE.txt for details.
 */

/* This program tries to find a thrackle embedding for the input graph.   
 * 
 * 
 * Compile with:
 *     
 *     cc -o thrackler -O4 thrackler.c
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <time.h>

//debug macros
#ifdef DEBUG
#define WHERESTR  "[file %s, line %d]: "
#define WHEREARG  __FILE__, __LINE__
#define DEBUGPRINT2(...)       fprintf(stderr, __VA_ARGS__)
#define DEBUGPRINT(_fmt, ...)  DEBUGPRINT2(WHERESTR _fmt, WHEREARG, __VA_ARGS__)
#define DEBUGCALL(call) call
#else
#define DEBUGPRINT(_fmt, ...)
#define DEBUGCALL(call)
#endif
#include "shared/multicode_base.h"
#include "shared/multicode_input.h"

#define MAXE (MAXN*MAXVAL)/2
#define MAXI (MAXE*(MAXE-1))/2 /* the maximum number of intersections */
#define MAXCN MAXN + MAXI      /* the maximum number of vertices in the cross graph */
#define MAXCE (6*MAXCN-12)     /* the maximum number of oriented edges in the cross graph */

#define INFI (MAXN + 1)

typedef int boolean;

#define FALSE 0
#define TRUE  1

typedef int VERTEXTYPE;

#define VERTEX 0
#define EDGEINTERSECTION 1
#define IS_VERTEX(v) !(v)
#define IS_INTERSECTION(v) (v)

typedef struct e /* The data type used for edges */ {
    int start; /* vertex where the edge starts */
    VERTEXTYPE startType;
    int end; /* vertex where the edge ends */
    VERTEXTYPE endType;
    
    int edgeNumber; /* the number of the edge in the original graph */

    struct e *prev; /* previous edge in clockwise direction */
    struct e *next; /* next edge in clockwise direction */
    struct e *inverse; /* the edge that is inverse to this one */
    int mark, index; /* two ints for temporary use;
                          Only access mark via the MARK macros. */
} EDGE;

EDGE *firstedge[MAXCN]; /* pointer to arbitrary edge out of vertex i. */
int degree[MAXCN];

EDGE edges[MAXCE];

static int markvalue = 30000;
#define RESETMARKS {int mki; if ((markvalue += 2) > 30000) \
       { markvalue = 2; for (mki=0;mki<MAXCE;++mki) edges[mki].mark=0;}}
#define MARK(e) (e)->mark = markvalue
#define MARKLO(e) (e)->mark = markvalue
#define MARKHI(e) (e)->mark = markvalue+1
#define UNMARK(e) (e)->mark = markvalue-1
#define ISMARKED(e) ((e)->mark >= markvalue)
#define ISMARKEDLO(e) ((e)->mark == markvalue)
#define ISMARKEDHI(e) ((e)->mark > markvalue)

int nv; //number of vertices
int ni; //number of intersections
int ne; //number of (undirected) edges in the cross graph

int numberedEdges[MAXE][2];
int edgeCount;

int intersectionCount;

int edgeCounter;
int crossGraphEdgeCounter;
int intersectionCounter;

unsigned long long int numberOfThrackles = 0;

//bit vectors

typedef unsigned long long int bitset;

#define ZERO 0ULL
#define ONE 1ULL
#define EMPTY_SET 0ULL
#define SINGLETON(el) (ONE << (el))
#define IS_SINGLETON(s) ((s) && (!((s) & ((s)-1))))
#define HAS_MORE_THAN_ONE_ELEMENT(s) ((s) & ((s)-1))
#define IS_NOT_EMPTY(s) (s)
#define IS_EMPTY(s) (!(s))
#define CONTAINS(s, el) ((s) & SINGLETON(el))
#define CONTAINS_ALL(s, elements) (((s) & (elements)) == (elements))
#define ADD(s, el) ((s) |= SINGLETON(el))
#define ADD_ALL(s, elements) ((s) |= (elements))
#define UNION(s1, s2) ((s1) | (s2))
#define INTERSECTION(s1, s2) ((s1) & (s2))
//these will only work if the element is actually in the set
#define REMOVE(s, el) ((s) ^= SINGLETON(el))
#define REMOVE_ALL(s, elements) ((s) ^= (elements))
#define MINUS(s, el) ((s) ^ SINGLETON(el))
#define MINUS_ALL(s, elements) ((s) ^ (elements))
//the following macros perform an extra step, but will work even if the element is not in the set
#define SAFE_REMOVE(s, el) ADD(s, el); REMOVE(s, el)
#define SAFE_REMOVE_ALL(s, elements) ADD_ALL(s, elements); REMOVE_ALL(s, elements)
#define ALL_UP_TO(el) (SINGLETON((el)+1)-1)
#define TOGGLE(s, el) ((s) ^= SINGLETON(el))
#define TOGGLE_ALL(s, elements) ((s) ^= (elements))

//////////////////////////////////////////////////////////////////////////////

void writeThrackleCode();
void doNextEdge();

//////////////////////////////////////////////////////////////////////////////

//debugging methods

void printThrackle(){
    int i;
    EDGE *e, *elast;
    
    ni = intersectionCount;
    
    fprintf(stderr, "Vertices: %d\nIntersections: %d\n", nv, ni);
    
    for(i=0; i<nv + ni; i++){
        if(degree[i] == 0) {
            fprintf(stderr, "%d)\n", i+1);
            continue;
        }
        e = elast = firstedge[i];
        fprintf(stderr, "%d) ", i+1);
        do {
            fprintf(stderr, "%d(%d) ", (e->end)+1, (e->edgeNumber)+1);
            e = e->next;
        } while (e != elast);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

void printEdgeNumbering(){
    int i;
    
    for(i = 0; i < edgeCount; i++){
        fprintf(stderr, "%d) %2d - %2d\n", i+1, numberedEdges[i][0]+1, numberedEdges[i][1]+1);
    }
}

//////////////////////////////////////////////////////////////////////////////

void handleThrackle(){
    numberOfThrackles++;
    ni = intersectionCounter;
    writeThrackleCode();
}

void intersectNextEdge(EDGE *neighbouringEdge,
        bitset nonIntersectedEdges, int currentEdge, int targetVertex){
    if(IS_NOT_EMPTY(nonIntersectedEdges)){
        //we still need to intersect some edges
        
        EDGE *e, *elast;
        e = elast = neighbouringEdge;
        do {
            if(CONTAINS(nonIntersectedEdges, e->edgeNumber)){
                DEBUGPRINT("Current edge: %d -- intersecting %d\n", currentEdge + 1, e->edgeNumber + 1);
                //we still need to intersect this edge, so let us try it
                EDGE *neighbouringEdgeNext = neighbouringEdge->next;
                EDGE *eInverse = e->inverse;
                //the new edges crossing the face
                EDGE *newCrossingEdge = edges + crossGraphEdgeCounter++;
                EDGE *newCrossingEdgeInverse = edges + crossGraphEdgeCounter++;
                //the other new edges created by the intersection
                EDGE *newEdgeAtE = edges + crossGraphEdgeCounter++;
                EDGE *newEdgeAtEInverse = edges + crossGraphEdgeCounter++;
                
                int newVertex = nv + intersectionCounter++;
                
                newCrossingEdge->start = neighbouringEdge->start;
                newCrossingEdge->startType = neighbouringEdge->startType;
                newCrossingEdge->end = newVertex;
                newCrossingEdge->endType = EDGEINTERSECTION;
                newCrossingEdge->edgeNumber = currentEdge;
                newCrossingEdge->inverse = newCrossingEdgeInverse;
                newCrossingEdge->prev = neighbouringEdge;
                newCrossingEdge->next = neighbouringEdgeNext;
                
                newCrossingEdgeInverse->start = newVertex;
                newCrossingEdgeInverse->startType = EDGEINTERSECTION;
                newCrossingEdgeInverse->end = neighbouringEdge->start;
                newCrossingEdgeInverse->endType = neighbouringEdge->startType;
                newCrossingEdgeInverse->edgeNumber = currentEdge;
                newCrossingEdgeInverse->inverse = newCrossingEdge;
                newCrossingEdgeInverse->prev = newEdgeAtEInverse;
                newCrossingEdgeInverse->next = newEdgeAtE;
                
                newEdgeAtE->start = newVertex;
                newEdgeAtE->startType = EDGEINTERSECTION;
                newEdgeAtE->end = e->start;
                newEdgeAtE->endType = e->startType;
                newEdgeAtE->edgeNumber = e->edgeNumber;
                newEdgeAtE->inverse = e;
                newEdgeAtE->prev = newCrossingEdgeInverse;
                newEdgeAtE->next = newEdgeAtEInverse;
                
                newEdgeAtEInverse->start = newVertex;
                newEdgeAtEInverse->startType = EDGEINTERSECTION;
                newEdgeAtEInverse->end = eInverse->start;
                newEdgeAtEInverse->endType = eInverse->startType;
                newEdgeAtEInverse->edgeNumber = eInverse->edgeNumber;
                newEdgeAtEInverse->inverse = eInverse;
                newEdgeAtEInverse->prev = newEdgeAtE;
                newEdgeAtEInverse->next = newCrossingEdgeInverse;
                
                e->inverse = newEdgeAtE;
                eInverse->inverse = newEdgeAtEInverse;
                neighbouringEdge->next = newCrossingEdge;
                neighbouringEdgeNext->prev = newCrossingEdge;
                e->end = newVertex;
                e->endType = EDGEINTERSECTION;
                eInverse->end = newVertex;
                eInverse->endType = EDGEINTERSECTION;
                
                firstedge[newVertex] = newCrossingEdgeInverse;
                degree[newVertex] = 3;
                degree[neighbouringEdge->start]++;
                DEBUGCALL(printThrackle());
                
                //go to next intersection
                intersectNextEdge(newEdgeAtE, MINUS(nonIntersectedEdges, e->edgeNumber),
                        currentEdge, targetVertex);
                
                //backtracking
                DEBUGPRINT("Backtracking with edge %d\n", currentEdge + 1);
                intersectionCounter--;
                crossGraphEdgeCounter-=4;
                degree[neighbouringEdge->start]--;
                e->inverse = eInverse;
                eInverse->inverse = e;
                neighbouringEdge->next = neighbouringEdgeNext;
                neighbouringEdgeNext->prev = neighbouringEdge;
                e->end = eInverse->start;
                e->endType = eInverse->startType;
                eInverse->end = e->start;
                eInverse->endType = e->startType;
            }
            e = e->inverse->prev;
        } while (e != elast);
    } else {
        //we have intersected all edges: check that target vertex is in the current face
        
        if(degree[targetVertex]==0){
            //vertex is not yet in the graph
            EDGE* newEdge = edges + crossGraphEdgeCounter++;
            EDGE* newEdgeInverse = edges + crossGraphEdgeCounter++;
            
            int startVertex = neighbouringEdge->start;
            VERTEXTYPE startVertexType = neighbouringEdge->startType;
            
            newEdge->start = startVertex;
            newEdge->startType = startVertexType;
            newEdge->end = targetVertex;
            newEdge->endType = VERTEX;
            newEdge->edgeNumber = currentEdge;
            
            EDGE *nextEdge = neighbouringEdge->next;
            neighbouringEdge->next = newEdge;
            newEdge->prev = neighbouringEdge;
            nextEdge->prev = newEdge;
            newEdge->next = nextEdge;

            newEdgeInverse->start = targetVertex;
            newEdgeInverse->startType = VERTEX;
            newEdgeInverse->end = startVertex;
            newEdgeInverse->endType = startVertexType;
            newEdgeInverse->next = newEdgeInverse->prev = newEdgeInverse;
            newEdgeInverse->edgeNumber = currentEdge;

            newEdge->inverse = newEdgeInverse;
            newEdgeInverse->inverse = newEdge;
            
            degree[startVertex]++;
            degree[targetVertex] = 1;
            firstedge[targetVertex] = newEdgeInverse;
            
            //go to next edge
            doNextEdge();
            
            //backtracking
            degree[startVertex]--;
            degree[targetVertex] = 0;
            crossGraphEdgeCounter -= 2;
            nextEdge->prev = neighbouringEdge;
            neighbouringEdge->next = nextEdge;
        } else {
            EDGE *e, *elast;
            e = elast = neighbouringEdge;
            do {
                if(e->end == targetVertex){
                    break;
                }
                e = e->inverse->prev;
            } while (e != elast);

            if(e->end != targetVertex){
                return;
            }

            //make connection with target vertex
            EDGE* newEdge = edges + crossGraphEdgeCounter++;
            EDGE* newEdgeInverse = edges + crossGraphEdgeCounter++;
            
            int startVertex = neighbouringEdge->start;
            VERTEXTYPE startVertexType = neighbouringEdge->startType;
            
            newEdge->start = startVertex;
            newEdge->startType = startVertexType;
            newEdge->end = targetVertex;
            newEdge->endType = VERTEX;
            newEdge->edgeNumber = currentEdge;
            
            EDGE *nextEdge = neighbouringEdge->next;
            neighbouringEdge->next = newEdge;
            newEdge->prev = neighbouringEdge;
            nextEdge->prev = newEdge;
            newEdge->next = nextEdge;

            newEdgeInverse->start = targetVertex;
            newEdgeInverse->startType = VERTEX;
            newEdgeInverse->end = startVertex;
            newEdgeInverse->endType = startVertexType;
            newEdgeInverse->edgeNumber = currentEdge;
            
            EDGE *nextEdgeInverse = e->inverse;
            EDGE *prevEdgeInverse = nextEdgeInverse->prev;
            nextEdgeInverse->prev = newEdgeInverse;
            newEdgeInverse->next = nextEdgeInverse;
            prevEdgeInverse->next = newEdgeInverse;
            newEdgeInverse->prev = prevEdgeInverse;

            newEdge->inverse = newEdgeInverse;
            newEdgeInverse->inverse = newEdge;
            
            degree[startVertex]++;
            degree[targetVertex]++;
            
            //go to next edge
            doNextEdge();
            
            //backtrack
            degree[startVertex]--;
            degree[targetVertex]--;
            crossGraphEdgeCounter -= 2;
            nextEdge->prev = neighbouringEdge;
            neighbouringEdge->next = nextEdge;
            nextEdgeInverse->prev = prevEdgeInverse;
            prevEdgeInverse->next = nextEdgeInverse;
        }
    }
}

void doNextEdge(){
    if(edgeCounter == edgeCount){
        //all edges are embedded
        handleThrackle();
        return;
    }
    
    int from, to;
    
    int currentEdge = edgeCounter++;
    
    from = numberedEdges[currentEdge][0];
    to = numberedEdges[currentEdge][1];
    
    //weave edge through current thrackle
    EDGE *e, *elast, newEdge;
    
    bitset nonIntersectedEdges = ALL_UP_TO(currentEdge-1);
    
    DEBUGPRINT("Next edge: %d (%d - %d)\n", currentEdge+1, from + 1, to + 1);
    
    //the vertex from will always have a degree different from 0
    e = elast = firstedge[from];
    do {
        REMOVE(nonIntersectedEdges, e->edgeNumber);
        DEBUGPRINT("Removing %d from non-intersected edges\n", e->edgeNumber + 1);
        e = e->next;
    } while (e != elast);
    if(degree[to]>0){
        e = elast = firstedge[to];
        do {
            REMOVE(nonIntersectedEdges, e->edgeNumber);
            DEBUGPRINT("Removing %d from non-intersected edges\n", e->edgeNumber + 1);
            e = e->next;
        } while (e != elast);
    }
    
    //add the first part of edge
    e = elast = firstedge[from];
    do {
        intersectNextEdge(e, nonIntersectedEdges, currentEdge, to);
        e = e->next;
    } while (e != elast);
    
    edgeCounter--;
}

void startThrackling(){
    int i, from, to;
    
    intersectionCounter = 0;
    crossGraphEdgeCounter = 0;
    edgeCounter = 0;
    
    for(i = 0; i < nv + intersectionCount; i++){
        firstedge[i] = NULL;
        degree[i] = 0;
    }
    
    //first edge
    from = numberedEdges[edgeCounter][0];
    to = numberedEdges[edgeCounter][1];
    
    EDGE *firstEdge = edges + crossGraphEdgeCounter++;
    firstEdge->edgeNumber = edgeCounter;
    firstEdge->start = from;
    firstEdge->end = to;
    firstEdge->startType = firstEdge->endType = VERTEX;
    firstEdge->next = firstEdge->prev = firstEdge;
    EDGE *inverseFirstEdge = edges + crossGraphEdgeCounter++;
    inverseFirstEdge->edgeNumber = edgeCounter;
    inverseFirstEdge->start = to;
    inverseFirstEdge->end = from;
    inverseFirstEdge->startType = inverseFirstEdge->endType = VERTEX;
    inverseFirstEdge->next = inverseFirstEdge->prev = inverseFirstEdge;
    
    firstEdge->inverse = inverseFirstEdge;
    inverseFirstEdge->inverse = firstEdge;
    
    firstedge[from] = firstEdge;
    firstedge[to] = inverseFirstEdge;
    degree[from] = degree[to] = 1;
    
    edgeCounter++;
    
    if(edgeCount == 1){
        handleThrackle();
        return;
    }
    
    //second edge
    from = numberedEdges[edgeCounter][0];
    to = numberedEdges[edgeCounter][1];
    
    EDGE *secondEdge = edges + crossGraphEdgeCounter++;
    secondEdge->edgeNumber = edgeCounter;
    secondEdge->start = from;
    secondEdge->end = to;
    secondEdge->startType = secondEdge->endType = VERTEX;
    if(degree[from]==0){
        secondEdge->next = secondEdge->prev = secondEdge;
        firstedge[from] = secondEdge;
    } else {
        secondEdge->next = secondEdge->prev = firstedge[from];
        firstedge[from]->next = firstedge[from]->prev = secondEdge;
    }
    EDGE *inverseSecondEdge = edges + crossGraphEdgeCounter++;
    inverseSecondEdge->edgeNumber = edgeCounter;
    inverseSecondEdge->start = to;
    inverseSecondEdge->end = from;
    inverseSecondEdge->startType = inverseSecondEdge->endType = VERTEX;
    if(degree[to]==0){
       inverseSecondEdge->next = inverseSecondEdge->prev = inverseSecondEdge;
       firstedge[to] = inverseSecondEdge;
    } else {
        inverseSecondEdge->next = inverseSecondEdge->prev = firstedge[to];
        firstedge[to]->next = firstedge[to]->prev = inverseSecondEdge;
    }
    
    secondEdge->inverse = inverseSecondEdge;
    inverseSecondEdge->inverse = secondEdge;
    
    degree[from]++;
    degree[to]++;
    
    edgeCounter++;
    
    if(edgeCount == 2){
        handleThrackle();
        return;
    }
    
    doNextEdge();
}

//some macros for the stack in the next method
#define INITSTACK(stack, maxsize) int top = 0; int stack[maxsize]
#define PUSH(stack, value) stack[top++] = (value)
#define POP(stack) stack[--top]
#define STACKISEMPTY top==0
#define STACKISNOTEMPTY top>0

/**
 * Stores the edges in the array numberedEdges.
 * The edges are numbered such that for each 0 <= i < #edges we have that
 * the graph induced by the edges 0 up to i is connected.
 */
void orderEdges(GRAPH graph, ADJACENCY adj){
    int i, j;
    edgeCount = 0;
    boolean isStored[graph[0][0]+1][graph[0][0]+1];
    boolean isVisited[graph[0][0]+1];
    for(i = 1; i <= graph[0][0]; i++){
        for(j = 1; j <= graph[0][0]; j++){
            isStored[i][j] = FALSE;
        }
        isVisited[i] = FALSE;
    }
    
    INITSTACK(vertexStack, nv);
    PUSH(vertexStack, 1); //push first vertex on the stack
    isVisited[1] = TRUE;
    
    while(STACKISNOTEMPTY){
        int currentVertex = POP(vertexStack);
        for(j = 0; j < adj[currentVertex]; j++){
            int currentNeighbour = graph[currentVertex][j];
            if(!isVisited[currentNeighbour]){
                PUSH(vertexStack, currentNeighbour);
                isVisited[currentNeighbour] = TRUE;
            }
            if(!isStored[currentVertex][currentNeighbour]){
                numberedEdges[edgeCount][0] = currentVertex - 1;
                numberedEdges[edgeCount][1] = currentNeighbour - 1;
                edgeCount++;
                isStored[currentVertex][currentNeighbour] =
                        isStored[currentNeighbour][currentVertex] = 
                        TRUE;
            }
        }
    }
    
    //verify that the input graph was connected
    for(i = 1; i <= graph[0][0]; i++){
        if(!isVisited[i]){
            fprintf(stderr, "Input graph was not connected -- exiting!\n");
            exit(EXIT_FAILURE);
        }
    }
    
    if(edgeCount > 64){
        fprintf(stderr, "Currently only supports up to 64 edges -- exiting!\n");
        exit(EXIT_FAILURE);
    }
}

void calculateCounts(GRAPH graph, ADJACENCY adj){
    int i;
    //number of vertices
    nv = graph[0][0];
    //number of intersections
    intersectionCount = edgeCount*edgeCount + edgeCount;
    for(i = 1; i <= nv; i++){
        intersectionCount -= adj[i]*adj[i];
    }
    intersectionCount /= 2;
}

void printStartSummary(){
    fprintf(stderr, "Input graph has %d %s and %d edge%s.\n",
            nv, nv == 1 ? "vertex" : "vertices",
            edgeCount, edgeCount==1 ? "" : "s");
    fprintf(stderr, "A thrackle embedding for this graph will have %d intersection%s.\n",
            intersectionCount, intersectionCount == 1 ? "" : "s");
}

//=============== Writing thrackle_code of graph ===========================

void writeThrackleCodeChar(){
    int i;
    EDGE *e, *elast;
    
    //write the number of vertices
    fputc(nv, stdout);
    //write the number of intersections
    fputc(ni, stdout);
    
    for(i=0; i<nv + ni; i++){
        e = elast = firstedge[i];
        do {
            fputc(e->end + 1, stdout);
            e = e->next;
        } while (e != elast);
        fputc(0, stdout);
    }
}

void writeShort(unsigned short value){
    if (fwrite(&value, sizeof (unsigned short), 1, stdout) != 1) {
        fprintf(stderr, "fwrite() failed -- exiting!\n");
        exit(-1);
    }
}

void writeThrackleCodeShort(){
    int i;
    EDGE *e, *elast;
    
    fputc(0, stdout);
    //write the number of vertices
    writeShort(nv);
    //write the number of intersections
    writeShort(ni);
    
    
    for(i=0; i<nv+ni; i++){
        e = elast = firstedge[i];
        do {
            writeShort(e->end + 1);
            e = e->next;
        } while (e != elast);
        writeShort(0);
    }
}

void writeThrackleCode(){
    static int first = TRUE;
    
    if(first){
        first = FALSE;
        
        fprintf(stdout, ">>thrackle_code<<");
    }
    
    if (nv + ni + 1 <= 255) {
        writeThrackleCodeChar();
    } else if (nv + ni + 1 <= 65535) {
        writeThrackleCodeShort();
    } else {
        fprintf(stderr, "Graphs of that size are currently not supported -- exiting!\n");
        exit(-1);
    }
    
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s computes thrackle embeddings for a given graph.\n\n", name);
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options]\n\n", name);
    fprintf(stderr, "\nThis program can handle graphs up to %d vertices. Recompile if you need larger\n", MAXN);
    fprintf(stderr, "graphs.\n\n");
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -h, --help\n");
    fprintf(stderr, "       Print this help and return.\n");
}

void usage(char *name) {
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "For more information type: %s -h \n\n", name);
}

int main(int argc, char *argv[]) {

    /*=========== commandline parsing ===========*/

    int c, i;
    char *name = argv[0];
    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'}
    };
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                help(name);
                return EXIT_SUCCESS;
            case '?':
                usage(name);
                return EXIT_FAILURE;
            default:
                fprintf(stderr, "Illegal option %c.\n", c);
                usage(name);
                return EXIT_FAILURE;
        }
    }
    
    /*=========== read graph ===========*/

    unsigned short code[MAXCODELENGTH];
    int length;
    GRAPH graph;
    ADJACENCY adj;
    if (readMultiCode(code, &length, stdin)) {
        decodeMultiCode(code, length, graph, adj);
        orderEdges(graph, adj);
        calculateCounts(graph, adj);
        printStartSummary();
        DEBUGCALL(printEdgeNumbering());
        startThrackling();
    } else {
        fprintf(stderr, "Input contains no graph -- exiting!\n");
    }
}
