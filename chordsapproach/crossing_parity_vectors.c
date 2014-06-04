/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE for details.
 */

/* 
 * Compile with:
 *     
 *     cc -o crossing_parity_vectors -O4 crossing_parity_vectors.c
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <malloc.h>

typedef int boolean;
#define TRUE 1
#define FALSE 0

int n;

int *permutation;

unsigned long long int permutationCount = 0;

unsigned long long int parityVectorCount = 0;

//bit vectors

typedef unsigned int bitset;

#define ONE 1UL
#define EMPTY_SET 0UL
#define SINGLETON(el) (ONE << (el))
#define CONTAINS(s, el) ((s) & SINGLETON(el))
#define ADD(s, el) ((s) |= SINGLETON(el))

//list
typedef struct le { 
    bitset vector;
    struct le *smaller;
    struct le *larger;
    
    int multiplicity;
} LISTENTRY;

LISTENTRY *parityVectors = NULL;

LISTENTRY *newListEntry(bitset vector){
    LISTENTRY *entry  = (LISTENTRY *)malloc(sizeof(LISTENTRY));
    if(entry == NULL){
        fprintf(stderr, "Insufficient memory to store parity vectors -- exiting!\n");
        exit(EXIT_FAILURE);
    }
    entry->vector = vector;
    entry->multiplicity = 1;
    entry->larger = entry->smaller = NULL;
    parityVectorCount++;
    return entry;
}

void addToList(LISTENTRY *entry, bitset vector){
    int compare = vector - entry->vector;
    
    if(compare == 0){
        entry->multiplicity++;
    } else if(compare < 0){
        if(entry->smaller == NULL){
            entry->smaller = newListEntry(vector);
        } else {
            addToList(entry->smaller, vector);
        }
    } else { // compare > 0
        if(entry->larger == NULL){
            entry->larger = newListEntry(vector);
        } else {
            addToList(entry->larger, vector);
        }
    }
}

void storeParityVector(){
    int i;
    permutationCount++;
    
    bitset parityVector = EMPTY_SET;
    
    for(i = 0; i < n; i++){
        if((permutation[i] - i) % 2){
            ADD(parityVector, i);
        }
    }
    
    if(parityVectors == NULL){
        parityVectors = newListEntry(parityVector);
    } else {
        addToList(parityVectors, parityVector);
    }
}

boolean nextPermutation(){
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

void prepare(){
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

void printVector(FILE *f, bitset vector){
    int i;
    
    for(i = 0; i < n; i++){
        fprintf(f, CONTAINS(vector, i) ? "1" : "0");
    }
    fprintf(f, "\n");
}

void printList(FILE *f, LISTENTRY *entry){
    if(entry->smaller != NULL){
        printList(f, entry->smaller);
    }
    printVector(f, entry->vector);
    if(entry->larger != NULL){
        printList(f, entry->larger);
    }
}

void finish(){
    printList(stderr, parityVectors);
    fprintf(stderr, "Found %llu permutation%s.\n", permutationCount,
            permutationCount==1 ? "" : "s");
    fprintf(stderr, "Found %llu parity vector%s.\n", parityVectorCount,
            parityVectorCount==1 ? "" : "s");
    free(permutation);
    freeList(parityVectors);
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options] n\n\n", name);
    fprintf(stderr, "The value n has to be at least 0 and at most %d.\n\n",
            8*sizeof(bitset));
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -h, --help\n");
    fprintf(stderr, "       Print this help and return.\n");
}

void usage(char *name) {
    fprintf(stderr, "Usage: %s [options] n\n", name);
    fprintf(stderr, "For more information type: %s -h \n\n", name);
}

int main(int argc, char** argv) {

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
    
    if (argc - optind != 1){
        usage(name);
        return (EXIT_FAILURE);
    }
    
    n = atoi(argv[optind]);
    
    if(n < 0 || n > 8*sizeof(bitset)){
        usage(name);
        return (EXIT_FAILURE);
    }
    
    prepare();
    
    storeParityVector();
    
    while(nextPermutation()){
        storeParityVector();
    }
    
    finish();

    
    return (EXIT_SUCCESS);
}

