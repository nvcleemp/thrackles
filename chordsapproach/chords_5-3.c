/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE for details.
 */

/* This program constructs the chord diagrams corresponding to a thrackle
 * embedding of a 5-cycle and a 3-cycle sharing a vertex.
 * 
 * 
 * Compile with:
 *     
 *     cc -o chords_5-3 -O4 chords_5-3.c
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

typedef int boolean;
#define TRUE 1
#define FALSE 0

//================= data structures ==================

int graph[32];
int graphSize;

int vertex2RightGroup[16];
int vertex2PositionInRightGroup[16];

#define LEFT_GROUP_COUNT 5
int leftGroupsStart[LEFT_GROUP_COUNT];
int leftGroupCurrentSize[LEFT_GROUP_COUNT];
int leftGroupTargetSize[LEFT_GROUP_COUNT];

#define RIGHT_GROUP_COUNT 3
int rightGroupSize[RIGHT_GROUP_COUNT];
boolean rightGroupPossibleNeighbour[RIGHT_GROUP_COUNT][LEFT_GROUP_COUNT];

int completedGraphCount = 0;

//==================== GENERATION ====================

void initialState(){
    int i, j;
    for(i = 0; i < 5; i++){
        graph[i] = i;
        graph[i+5] = i;
        leftGroupCurrentSize[i] = 2;
        leftGroupTargetSize[i] = 5;
        leftGroupsStart[i] = 2*i;
    }
    leftGroupTargetSize[0] = leftGroupTargetSize[4] = 3;
    for(i = 0; i < 11; i++){
        graph[i+10] = i+5;
    }
    graphSize = 21;
    
    for(i = 0; i < 3; i++){
        for(j = 0; j < 5; j++){
            rightGroupPossibleNeighbour[i][j] = TRUE;
        }
    }
    rightGroupPossibleNeighbour[0][0] = rightGroupPossibleNeighbour[0][4] =
            rightGroupPossibleNeighbour[2][0] = rightGroupPossibleNeighbour[2][4] = FALSE;
    
    for(i = 0; i < 5; i++){
        vertex2RightGroup[i] = -1;
        vertex2PositionInRightGroup[i] = -1;
    }
    for(i = 5; i < 8; i++){
        vertex2RightGroup[i] = 0;
        vertex2PositionInRightGroup[i] = i - 5;
    }
    for(i = 8; i < 13; i++){
        vertex2RightGroup[i] = 1;
        vertex2PositionInRightGroup[i] = i - 8;
    }
    for(i = 13; i < 16; i++){
        vertex2RightGroup[i] = 2;
        vertex2PositionInRightGroup[i] = i - 13;
    }
}

void printGraph(FILE *f){    
    int i;
    for(i = 0; i < graphSize; i++){
        fprintf(f, "%d ", graph[i]);
    }
    fprintf(f, "\n");
}

void handleCompletedGraph(){
    completedGraphCount++;
    printGraph(stdout);
}

boolean checkParity(int completedGroup){
    int i, j;
    
    for(i = 0; i < leftGroupCurrentSize[completedGroup]; i++){
        if(graph[leftGroupsStart[completedGroup] + i] > 4){
            int currentVertex = graph[leftGroupsStart[completedGroup] + i];
            int rightGroup = vertex2RightGroup[currentVertex];
            int rightGroupPosition = vertex2PositionInRightGroup[currentVertex];
            
            int vertexCount = (leftGroupCurrentSize[completedGroup] - i) + rightGroupPosition;
            
            for(j = completedGroup + 1; j < LEFT_GROUP_COUNT; j++){
                vertexCount += leftGroupTargetSize[j];
            }
            for(j = 0; j < rightGroup; j++){
                vertexCount += rightGroupSize[j];
            }
            if(vertexCount%2){
                return FALSE;
            }
        }
    }
    return TRUE;
}

boolean checkParityLeftChords(){
    int i, j;
    for(i = 0; i < 5; i++){
        j = 0;
        while(graph[j]!=i){
            j++;
        }
        int start = j;
        j++;
        while(graph[j]!=i){
            j++;
        }
        if((start - j + 1) % 2){
            return FALSE;
        }
    }
    return TRUE;
}

void makeNextConnection(int vertex){
    if(vertex == 16){
        if(checkParityLeftChords()){
            handleCompletedGraph();
        }
        return;
    }
    
    int i, j;
    
    int currentGroup = vertex2RightGroup[vertex];
    
    for(i = 0; i < LEFT_GROUP_COUNT; i++){
        if(rightGroupPossibleNeighbour[currentGroup][i]){
            //global changes (i.e. for complete this if-block)
            rightGroupPossibleNeighbour[currentGroup][i] = FALSE;
            leftGroupCurrentSize[i]++;
            for(j = graphSize; j > leftGroupsStart[i]; j--){
                graph[j] = graph[j-1];
            }
            for(j = i + 1; j < LEFT_GROUP_COUNT; j++){
                leftGroupsStart[j] += 1;
            }
            graphSize++;
            
            graph[leftGroupsStart[i]] = vertex;
            if(leftGroupCurrentSize[i] == leftGroupTargetSize[i]){
                //we can bound
                if(checkParity(i)){
                    makeNextConnection(vertex + 1);
                }
            } else {
                makeNextConnection(vertex + 1);
            }
            
            for(j = 0; j < leftGroupCurrentSize[i]-1; j++){
                //switch position j and j+1 in the group
                int tmp = graph[leftGroupsStart[i]+j];
                graph[leftGroupsStart[i]+j] = graph[leftGroupsStart[i]+j+1];
                graph[leftGroupsStart[i]+j+1] = tmp;
                
                if(leftGroupCurrentSize[i] == leftGroupTargetSize[i]){
                    //we can bound
                    if(checkParity(i)){
                        makeNextConnection(vertex + 1);
                    }
                } else {
                    makeNextConnection(vertex + 1);
                }
            }
            
            graphSize--;
            for(j = i + 1; j < LEFT_GROUP_COUNT; j++){
                leftGroupsStart[j] -= 1;
            }
            for(j = leftGroupsStart[i] + leftGroupCurrentSize[i] - 1; j < graphSize; j++){
                graph[j] = graph[j+1];
            }
            leftGroupCurrentSize[i]--;
            rightGroupPossibleNeighbour[currentGroup][i] = TRUE;
        }
    }
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s constructs the chord diagrams corresponding to a\n", name);
    fprintf(stderr, "thrackle embedding of a 5-cycle and a 3-cycle sharing a vertex.\n\n");
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options]\n\n", name);
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

    int c;
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
    
    initialState();
    
    makeNextConnection(5);
    
    fprintf(stderr, "Found %d bipartite chord graph%s.\n", completedGraphCount,
            completedGraphCount == 1 ? "" : "s");
    
    return EXIT_SUCCESS;
}

