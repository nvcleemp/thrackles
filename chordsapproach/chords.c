/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE for details.
 */

/* This program constructs the chord diagrams corresponding to a thrackle
 * embedding of two odd cycles sharing a vertex.
 * 
 * 
 * Compile with:
 *     
 *     cc -o chords -O4 chords.c
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

//================================================================

//booleans

typedef int boolean;
#define TRUE 1
#define FALSE 0

//bit vectors

typedef unsigned int bitset;

#define ONE 1UL
#define EMPTY_SET 0UL
#define SINGLETON(el) (ONE << (el))
#define CONTAINS(s, el) ((s) & SINGLETON(el))
#define ADD(s, el) ((s) |= SINGLETON(el))

//lists

typedef struct le { 
    bitset scheme;
    struct le *smaller;
    struct le *larger;
    
    int multiplicity;
} LISTENTRY;

//================================================================

int completedSchemesCount = 0;
int n1, n2;
int joinGroupSize, cycleGroupSize;

#define MAXIMUM_GROUP_SIZE 11

bitset joinTypeSchemes[126]; //126 is for size 9
int joinTypeSchemeCount = 0;

bitset cycleTypeSchemes[462]; //462 is for size 11
int cycleTypeSchemeCount = 0;

int *permutation;

#define EMPTY -2
#define UNSET -1

#define IS_EMPTY_POSITION(r,c) (closeIntersectionMatrix[r][c] == EMPTY)
#define IS_UNSET_POSITION(r,c) (closeIntersectionMatrix[r][c] == UNSET) 

int closeIntersectionMatrix[MAXIMUM_GROUP_SIZE+3][MAXIMUM_GROUP_SIZE+3];

boolean isCycleTypeGroup[MAXIMUM_GROUP_SIZE+3];

LISTENTRY *crossingSchemes = NULL;

//==============================================================================

LISTENTRY *newListEntry(bitset scheme){
    LISTENTRY *entry  = (LISTENTRY *)malloc(sizeof(LISTENTRY));
    if(entry == NULL){
        fprintf(stderr, "Insufficient memory to store parity vectors -- exiting!\n");
        exit(EXIT_FAILURE);
    }
    entry->scheme = scheme;
    entry->multiplicity = 1;
    entry->larger = entry->smaller = NULL;
    return entry;
}

void addToList(LISTENTRY *entry, bitset scheme){
    int compare = scheme - entry->scheme;
    
    if(compare == 0){
        entry->multiplicity++;
    } else if(compare < 0){
        if(entry->smaller == NULL){
            entry->smaller = newListEntry(scheme);
        } else {
            addToList(entry->smaller, scheme);
        }
    } else { // compare > 0
        if(entry->larger == NULL){
            entry->larger = newListEntry(scheme);
        } else {
            addToList(entry->larger, scheme);
        }
    }
}

void storeCrossingScheme(int n){
    int i;
    
    bitset crossingScheme = EMPTY_SET;
    
    for(i = 0; i < n; i++){
        if((permutation[i] - i) % 2){
            ADD(crossingScheme, i);
        }
    }
    
    if(crossingSchemes == NULL){
        crossingSchemes = newListEntry(crossingScheme);
    } else {
        addToList(crossingSchemes, crossingScheme);
    }
}

boolean nextPermutation(int n){
    int k, l, i, tmp;
    
    k = n - 2;
    while(k >= 0 && permutation[k] > permutation[k+1]){
        k--;
    }
    if(k < 0){
        //there is no more permutation
        return FALSE;
    }
    l = n - 1;
    while(permutation[k] > permutation[l]){
        l--;
    }
    
    //switch position k and l
    tmp = permutation[k];
    permutation[k] = permutation[l];
    permutation[l] = tmp;
    
    //reverse sequence from position k+1 to position n-1
    i = 1;
    while(k + i < n - i){
        tmp = permutation[k + i];
        permutation[k + i] = permutation[n - i];
        permutation[n - i] = tmp;
        i++;
    }
    
    return TRUE;
}

void prepareCrossingSchemes(int n){
    int i;
    
    permutation = (int *)malloc(n * sizeof(int));
    
    for(i = 0; i < n; i++){
        permutation[i] = i;
    }
}

void freeList(LISTENTRY *entry){
    if(entry->smaller != NULL){
        freeList(entry->smaller);
    }
    if(entry->larger != NULL){
        freeList(entry->larger);
    }
    free(entry);
}

void finishCrossingSchemes(){
    
    free(permutation);
    freeList(crossingSchemes);
    crossingSchemes = NULL;
}

void storeJoinTypeSchemes(LISTENTRY *entry){
    if(entry->smaller != NULL){
        storeJoinTypeSchemes(entry->smaller);
    }
    
    joinTypeSchemes[joinTypeSchemeCount] = entry->scheme;
    joinTypeSchemeCount++;
    
    if(entry->larger != NULL){
        storeJoinTypeSchemes(entry->larger);
    }
}

void storeCycleTypeSchemes(LISTENTRY *entry){
    if(entry->smaller != NULL){
        storeCycleTypeSchemes(entry->smaller);
    }
    
    cycleTypeSchemes[cycleTypeSchemeCount] = entry->scheme;
    cycleTypeSchemeCount++;
    
    if(entry->larger != NULL){
        storeCycleTypeSchemes(entry->larger);
    }
}

void generateCrossingSchemes(){
    //first for the join-type groups
    prepareCrossingSchemes(joinGroupSize);
    
    storeCrossingScheme(joinGroupSize);
    
    while(nextPermutation(joinGroupSize)){
        storeCrossingScheme(joinGroupSize);
    }
    
    storeJoinTypeSchemes(crossingSchemes);
    
    finishCrossingSchemes();
    
    //then for the cycle-type groups
    prepareCrossingSchemes(cycleGroupSize);
    
    storeCrossingScheme(cycleGroupSize);
    
    while(nextPermutation(cycleGroupSize)){
        storeCrossingScheme(cycleGroupSize);
    }
    
    storeCycleTypeSchemes(crossingSchemes);
    
    finishCrossingSchemes();
    
}

void handleCompletedMatrix(){
    completedSchemesCount++;
    
    int i,j;
    
    for(i = 0; i < n1 + n2; i++){
        for(j = 0; j < n1 + n2; j++){
            if(IS_EMPTY_POSITION(i, j)){
                fprintf(stdout, " ");
            } else {
                fprintf(stdout, "%d", closeIntersectionMatrix[i][j]);
            }
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
}

boolean checkVectorMatchesPrefilledPart_cycleType(bitset vector, int row){
    int i;
    int schemePosition = n1 + n2 - 3 - 1;
    
    for(i = row; i >= 0; i--){
        if(!IS_EMPTY_POSITION(row, i)){
            if(CONTAINS(vector, schemePosition)){
                if(!closeIntersectionMatrix[row][i]){
                    return FALSE;
                }
            } else {
                if(closeIntersectionMatrix[row][i]){
                    return FALSE;
                }
            }
            schemePosition--;
        }
    }
    
    return TRUE;
}

boolean checkVectorMatchesPrefilledPart_joinType(bitset vector, int row){
    int i;
    int schemePosition = n1 + n2 - 5 - 1;
    
    for(i = row; i >= 0; i--){
        if(!IS_EMPTY_POSITION(row, i)){
            if(CONTAINS(vector, schemePosition)){
                if(!closeIntersectionMatrix[row][i]){
                    return FALSE;
                }
            } else {
                if(closeIntersectionMatrix[row][i]){
                    return FALSE;
                }
            }
            schemePosition--;
        }
    }
    
    return TRUE;
}

void fillRowWithCurrentScheme(bitset scheme, int row){
    int i, schemePosition;
    
    schemePosition = 0;
    for(i = row; i < n1 + n2; i++){
        if(!IS_EMPTY_POSITION(row, i)){
            int value = CONTAINS(scheme, schemePosition) ? 1 : 0;
            schemePosition++;
            closeIntersectionMatrix[row][i] = value;
            if((row + i) % 2){
                closeIntersectionMatrix[i][row] = value;
            } else {
                closeIntersectionMatrix[i][row] = 1 - value;
            }
        }
    }
}

void fillRow(int row){
    if(row == n1 + n2){
        handleCompletedMatrix();
        return;
    }
    
    int i;
    
    if(isCycleTypeGroup[row]){
        for(i = 0; i < cycleTypeSchemeCount; i++){
            if(checkVectorMatchesPrefilledPart_cycleType(cycleTypeSchemes[i], row)){
                //fill matrix with current scheme
                fillRowWithCurrentScheme(cycleTypeSchemes[i], row);
                
                //recurse
                fillRow(row + 1);
            }
        }
    } else {
        for(i = 0; i < joinTypeSchemeCount; i++){
            if(checkVectorMatchesPrefilledPart_joinType(joinTypeSchemes[i], row)){
                //fill matrix with current scheme
                fillRowWithCurrentScheme(joinTypeSchemes[i], row);
                
                //recurse
                fillRow(row + 1);
            }
        }
    }
}

void prepareMatrix(){
    int i, j;
    
    //prefill the matrix with unset values
    for(i = 0; i < n1 + n2; i++){
        for(j = 0; j < n1 + n2; j++){
            closeIntersectionMatrix[i][j] = UNSET;
        }
    }
    
    //handle the join of the two matrices
    closeIntersectionMatrix[0][n1] = EMPTY;
    closeIntersectionMatrix[0][n1 + n2 -1] = EMPTY;
    closeIntersectionMatrix[n1 - 1][n1] = EMPTY;
    closeIntersectionMatrix[n1 - 1][n1 + n2 -1] = EMPTY;
    closeIntersectionMatrix[n1][0] = EMPTY;
    closeIntersectionMatrix[n1][n1 - 1] = EMPTY;
    closeIntersectionMatrix[n1 + n2 -1][0] = EMPTY;
    closeIntersectionMatrix[n1 + n2 -1][n1 - 1] = EMPTY;
    
    //handle internal edges of cycle 1
    for(i = 0; i < n1; i++){
        closeIntersectionMatrix[i][(i + n1 - 1) % n1] = EMPTY;
        closeIntersectionMatrix[i][i] = EMPTY;
        closeIntersectionMatrix[i][(i + 1) % n1] = EMPTY;
    }
    
    //handle internal edges of cycle 2
    for(i = 0; i < n2; i++){
        closeIntersectionMatrix[n1 + i][n1 + ((i + n2 - 1) % n2)] = EMPTY;
        closeIntersectionMatrix[n1 + i][n1 + i] = EMPTY;
        closeIntersectionMatrix[n1 + i][n1 + ((i + 1) % n2)] = EMPTY;
    }
    
    //mark the cycle edges
    for(i = 0; i < n1 + n2; i++){
        isCycleTypeGroup[i] = TRUE;
    }
    isCycleTypeGroup[0] = isCycleTypeGroup[n1 - 1] =
            isCycleTypeGroup[n1] = isCycleTypeGroup[n1 + n2 -1] = FALSE;
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s constructs the chord diagrams corresponding to a\n", name);
    fprintf(stderr, "thrackle embedding of two odd cycles sharing a vertex.\n\n");
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options] n1 n2\n\n", name);
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -h, --help\n");
    fprintf(stderr, "       Print this help and return.\n");
}

void usage(char *name) {
    fprintf(stderr, "Usage: %s [options] n1 n2\n", name);
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
    
    if (argc - optind != 2){
        usage(name);
        return (EXIT_FAILURE);
    }
    
    n1 = atoi(argv[optind]);
    n2 = atoi(argv[optind + 1]);
    
    if((n1 < 3) || (n1 % 2 == 0)){
        fprintf(stderr, "First cycle size is not an odd integer greater than 1 -- exiting!\n");
        return (EXIT_FAILURE);
    }
    
    if((n2 < 3) || (n2 % 2 == 0)){
        fprintf(stderr, "Second cycle size is not an odd integer greater than 1 -- exiting!\n");
        return (EXIT_FAILURE);
    }
    
    if(n1 + n2 - 3 > MAXIMUM_GROUP_SIZE){
        fprintf(stderr, "The sum of the two cycles is too large (maximum: %d) -- exiting!\n", MAXIMUM_GROUP_SIZE + 3);
        return (EXIT_FAILURE);
    }
    
    joinGroupSize = n1 + n2 - 5;
    cycleGroupSize = n1 + n2 - 3;
    
    generateCrossingSchemes();
    
    prepareMatrix();
    
    fillRow(0);
    
    fprintf(stderr, "Found %d bipartite close intersection scheme%s.\n",
            completedSchemesCount, completedSchemesCount == 1 ? "" : "s");
    
    return EXIT_SUCCESS;
}

