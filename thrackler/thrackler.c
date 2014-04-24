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
#define INTERSECTION 1
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


//////////////////////////////////////////////////////////////////////////////

void doNextEdge(){
    if(edgeCounter == edgeCount){
        //all edges are embedded
        //handle completed thrackle
        return;
    }
    
    int from, to;
    
    int currentEdge = edgeCounter++;
    
    from = numberedEdges[currentEdge][0];
    to = numberedEdges[currentEdge][1];
    
    //weave edge through current thrackle
    
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
    firstEdge->next = firstEdge->prev = NULL;
    EDGE *inverseFirstEdge = edges + crossGraphEdgeCounter++;
    inverseFirstEdge->edgeNumber = edgeCounter;
    inverseFirstEdge->start = to;
    inverseFirstEdge->end = from;
    inverseFirstEdge->startType = firstEdge->endType = VERTEX;
    inverseFirstEdge->next = firstEdge->prev = NULL;
    
    firstEdge->inverse = inverseFirstEdge;
    inverseFirstEdge->inverse = firstEdge;
    
    firstedge[from] = firstEdge;
    firstedge[to] = inverseFirstEdge;
    degree[from] = degree[to] = 1;
    
    edgeCounter++;
    
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
                numberedEdges[edgeCount][0] = currentNeighbour - 1;
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
    } else {
        fprintf(stderr, "Input contains no graph -- exiting!\n");
    }
}
