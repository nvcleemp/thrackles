/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE for details.
 */

/* This program constructs the chord diagrams corresponding to a thrackle
 * embedding of two 5-cycles sharing a vertex.
 * 
 * 
 * Compile with:
 *     
 *     cc -o chords_5-5 -O4 chords_5-5.c
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

typedef int boolean;
#define TRUE 1
#define FALSE 0

boolean onlyCount = FALSE;

//================= data structures ==================

int graph[32];
int graphSize;

int leftPart[31];
int leftPartSize;

int rightPart[31];
int rightPartSize;

int transversal2RightGroup[31];
boolean isFirstTransversalInRightGroup[31];
int transversal2PositionInRightGroup[31];

#define LEFT_GROUP_COUNT 5
int leftGroupsStart[LEFT_GROUP_COUNT];
int leftGroupCurrentSize[LEFT_GROUP_COUNT];
int leftGroupTargetSize[LEFT_GROUP_COUNT];

#define RIGHT_GROUP_COUNT 5
int rightGroupSize[RIGHT_GROUP_COUNT];
boolean rightGroupPossibleNeighbour[RIGHT_GROUP_COUNT][LEFT_GROUP_COUNT];
int rightGroupStart[RIGHT_GROUP_COUNT];

unsigned long long int completedGraphCount = 0;

boolean leftPartIsGroupEnd[31];
boolean rightPartIsGroupEnd[31];

#define IS_TRANSVERSAL_CHORD(n) ((n) > 9)

//==================== GENERATION ====================

void initialState(){
    int i, j;
    for(i = 0; i < 5; i++){
        leftPart[i] = i;
        leftPart[i+5] = i;
        leftGroupCurrentSize[i] = 2;
        leftGroupTargetSize[i] = 7;
        leftGroupsStart[i] = 2*i;
    }
    leftGroupTargetSize[0] = leftGroupTargetSize[4] = 5;
    leftPartSize = 10;
    
    for(i = 0; i < 5; i++){
        for(j = 0; j < 5; j++){
            rightGroupPossibleNeighbour[i][j] = TRUE;
        }
    }
    rightGroupPossibleNeighbour[0][0] = rightGroupPossibleNeighbour[0][4] =
            rightGroupPossibleNeighbour[4][0] = rightGroupPossibleNeighbour[4][4] = FALSE;
    
    
    for(i = 0; i < 10; i++){
        //these are not transversal chords
        transversal2RightGroup[i] = -1;
    }
    for(i = 10; i < 13; i++){
        transversal2RightGroup[i] = 0;
    }
    for(i = 13; i < 18; i++){
        transversal2RightGroup[i] = 1;
    }
    for(i = 18; i < 23; i++){
        transversal2RightGroup[i] = 2;
    }
    for(i = 23; i < 28; i++){
        transversal2RightGroup[i] = 3;
    }
    for(i = 28; i < 31; i++){
        transversal2RightGroup[i] = 4;
    }
    
    for(i = 0; i < 31; i++){
        isFirstTransversalInRightGroup[i] = FALSE;
    }
    isFirstTransversalInRightGroup[10] = isFirstTransversalInRightGroup[13] =
            isFirstTransversalInRightGroup[18] = isFirstTransversalInRightGroup[23] =
            isFirstTransversalInRightGroup[28] = TRUE;
    
    //right part
    for(i = 0; i < 31; i++){
        rightPart[i] = -1;
    }
    rightPart[0] = rightPart[13] = 5;
    rightPart[1] = rightPart[19] = 6;
    rightPart[5] = rightPart[20] = 7;
    rightPart[6] = rightPart[26] = 8;
    rightPart[12] = rightPart[27] = 9;
    
    for(i = 0; i < 5; i++){
        rightGroupSize[i] = 2;
    }
    
    rightGroupStart[0] = 0;
    rightGroupStart[1] = 5;
    rightGroupStart[2] = 12;
    rightGroupStart[3] = 19;
    rightGroupStart[4] = 26;
    
    //group ends for printing
    for(i = 0; i < 31; i++){
        leftPartIsGroupEnd[i] = rightPartIsGroupEnd[i] = FALSE;
    }
    leftPartIsGroupEnd[4] = leftPartIsGroupEnd[11] = 
            leftPartIsGroupEnd[18] = leftPartIsGroupEnd[25] =
            leftPartIsGroupEnd[30] = rightPartIsGroupEnd[4] =
            rightPartIsGroupEnd[11] = rightPartIsGroupEnd[18] =
            rightPartIsGroupEnd[25] = TRUE;
}

void printGraph(FILE *f, boolean pretty){    
    int i;
    for(i = 0; i < leftPartSize; i++){
        fprintf(f, "%d ", leftPart[i]);
        if(pretty){
            if(leftPartIsGroupEnd[i]){
                fprintf(f, "| ");
            }
        }
    }
    for(i = 0; i < 31; i++){
        fprintf(f, "%d ", rightPart[i]);
        if(pretty){
            if(rightPartIsGroupEnd[i]){
                fprintf(f, "| ");
            }
        }
    }
    fprintf(f, "\n");
}

void handleCompletedGraph(){
    completedGraphCount++;
    if(!onlyCount){
        printGraph(stdout, FALSE);
    }
}

boolean checkParityTransversalChords(int completedGroup){
    //boolean debug = (completedGroup == 4);
    int i, j;
    
    for(i = 0; i < leftGroupCurrentSize[completedGroup]; i++){
        int currentVertex = leftPart[leftGroupsStart[completedGroup] + i];
        if(IS_TRANSVERSAL_CHORD(currentVertex)){
            int rightGroup = transversal2RightGroup[currentVertex];
            int rightGroupPosition = transversal2PositionInRightGroup[currentVertex];

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

boolean checkParityRemainingChords(){
    int i, j;
    for(i = 0; i < 5; i++){
        j = 0;
        while(leftPart[j]!=i){
            j++;
        }
        int start = j;
        j++;
        while(leftPart[j]!=i){
            j++;
        }
        if((start - j + 1) % 2){
            return FALSE;
        }
    }
    for(i = 5; i < 10; i++){
        j = 0;
        while(rightPart[j]!=i){
            j++;
        }
        int start = j;
        j++;
        while(rightPart[j]!=i){
            j++;
        }
        if((start - j + 1) % 2){
            return FALSE;
        }
    }
    return TRUE;
}

void makeNextConnection(int currentChord){
    if(currentChord == 31){
        if(checkParityRemainingChords()){
            handleCompletedGraph();
        }
        return;
    }
    
    int i, j;
    
    int currentGroup = transversal2RightGroup[currentChord];
    
    int start;
    if(isFirstTransversalInRightGroup[currentChord]){
        start = 0;
    } else {
        start = transversal2PositionInRightGroup[currentChord - 1] + 1;
    }
    
    //insert at position start in group and shift remainder of group
    for(i = rightGroupSize[currentGroup]; i > start; i--){
        rightPart[rightGroupStart[currentGroup] + i] = 
                rightPart[rightGroupStart[currentGroup] + i-1];
    }
    rightPart[rightGroupStart[currentGroup] + start] = currentChord;
    transversal2PositionInRightGroup[currentChord] = start;
    rightGroupSize[currentGroup]++;
    
    while(transversal2PositionInRightGroup[currentChord] < rightGroupSize[currentGroup] - 1){
        //check position
        for(i = 0; i < LEFT_GROUP_COUNT; i++){
            if(rightGroupPossibleNeighbour[currentGroup][i]){
                //global changes (i.e. for complete this if-block)
                rightGroupPossibleNeighbour[currentGroup][i] = FALSE;
                leftGroupCurrentSize[i]++;
                for(j = leftPartSize; j > leftGroupsStart[i]; j--){
                    leftPart[j] = leftPart[j-1];
                }
                for(j = i + 1; j < LEFT_GROUP_COUNT; j++){
                    leftGroupsStart[j] += 1;
                }
                leftPartSize++;

                leftPart[leftGroupsStart[i]] = currentChord;
                if(leftGroupCurrentSize[i] == leftGroupTargetSize[i]){
                    //we can bound
                    if(checkParityTransversalChords(i)){
                        makeNextConnection(currentChord + 1);
                    }
                } else {
                    makeNextConnection(currentChord + 1);
                }

                for(j = 0; j < leftGroupCurrentSize[i]-1; j++){
                    //switch position j and j+1 in the group
                    int tmp = leftPart[leftGroupsStart[i]+j];
                    leftPart[leftGroupsStart[i]+j] = leftPart[leftGroupsStart[i]+j+1];
                    leftPart[leftGroupsStart[i]+j+1] = tmp;

                    if(leftGroupCurrentSize[i] == leftGroupTargetSize[i]){
                        //we can bound
                        if(checkParityTransversalChords(i)){
                            makeNextConnection(currentChord + 1);
                        }
                    } else {
                        makeNextConnection(currentChord + 1);
                    }
                }

                leftPartSize--;
                for(j = i + 1; j < LEFT_GROUP_COUNT; j++){
                    leftGroupsStart[j] -= 1;
                }
                for(j = leftGroupsStart[i] + leftGroupCurrentSize[i] - 1; j < leftPartSize; j++){
                    leftPart[j] = leftPart[j+1];
                }
                leftGroupCurrentSize[i]--;
                rightGroupPossibleNeighbour[currentGroup][i] = TRUE;
            }
        }
        
        //shift current chord one position in the group
        int position = rightGroupStart[currentGroup] + transversal2PositionInRightGroup[currentChord];
        rightPart[position] = rightPart[position + 1];
        rightPart[position + 1] = currentChord;
        transversal2PositionInRightGroup[currentChord]++;
    }
    
    //current chord is at the end of the group: check this position and then backtrack
    for(i = 0; i < LEFT_GROUP_COUNT; i++){
        if(rightGroupPossibleNeighbour[currentGroup][i]){
            //global changes (i.e. for complete this if-block)
            rightGroupPossibleNeighbour[currentGroup][i] = FALSE;
            leftGroupCurrentSize[i]++;
            for(j = leftPartSize; j > leftGroupsStart[i]; j--){
                leftPart[j] = leftPart[j-1];
            }
            for(j = i + 1; j < LEFT_GROUP_COUNT; j++){
                leftGroupsStart[j] += 1;
            }
            leftPartSize++;

            leftPart[leftGroupsStart[i]] = currentChord;
            if(leftGroupCurrentSize[i] == leftGroupTargetSize[i]){
                //we can bound
                if(checkParityTransversalChords(i)){
                    makeNextConnection(currentChord + 1);
                }
            } else {
                makeNextConnection(currentChord + 1);
            }

            for(j = 0; j < leftGroupCurrentSize[i]-1; j++){
                //switch position j and j+1 in the group
                int tmp = leftPart[leftGroupsStart[i]+j];
                leftPart[leftGroupsStart[i]+j] = leftPart[leftGroupsStart[i]+j+1];
                leftPart[leftGroupsStart[i]+j+1] = tmp;

                if(leftGroupCurrentSize[i] == leftGroupTargetSize[i]){
                    //we can bound
                    if(checkParityTransversalChords(i)){
                        makeNextConnection(currentChord + 1);
                    }
                } else {
                    makeNextConnection(currentChord + 1);
                }
            }

            leftPartSize--;
            for(j = i + 1; j < LEFT_GROUP_COUNT; j++){
                leftGroupsStart[j] -= 1;
            }
            for(j = leftGroupsStart[i] + leftGroupCurrentSize[i] - 1; j < leftPartSize; j++){
                leftPart[j] = leftPart[j+1];
            }
            leftGroupCurrentSize[i]--;
            rightGroupPossibleNeighbour[currentGroup][i] = TRUE;
        }
    }
    
    rightGroupSize[currentGroup]--;
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s constructs the chord diagrams corresponding to a\n", name);
    fprintf(stderr, "thrackle embedding of two 5-cycles sharing a vertex.\n\n");
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options]\n\n", name);
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -n\n");
    fprintf(stderr, "       Only count the number of chord diagrams: do not output them.\n");
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

    while ((c = getopt_long(argc, argv, "hn", long_options, &option_index)) != -1) {
        switch (c) {
            case 'n':
                onlyCount = TRUE;
                break;
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
    
    makeNextConnection(10);
    
    fprintf(stderr, "Found %llu bipartite chord graph%s.\n", completedGraphCount,
            completedGraphCount == 1 ? "" : "s");
    
    return EXIT_SUCCESS;
}

