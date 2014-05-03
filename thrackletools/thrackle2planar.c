/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE for details.
 */

/* This program writes the cross graph of a thrackle in thrackle_code to standard
 * out in planar_code format.   
 * 
 * 
 * Compile with:
 *     
 *     cc -o thrackle2planar -O4 thrackle2planar.c
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#define MAXN 100
#define MAXE (6*MAXN-12)     /* the maximum number of oriented edges in the cross graph */
#define MAXCODELENGTH (MAXN+MAXE+4)

#define INFI (MAXN + 1)

typedef int boolean;

#define FALSE 0
#define TRUE  1

typedef struct e /* The data type used for edges */ {
    int start; /* vertex where the edge starts */
    int end; /* vertex where the edge ends */

    struct e *prev; /* previous edge in clockwise direction */
    struct e *next; /* next edge in clockwise direction */
    struct e *inverse; /* the edge that is inverse to this one */
} EDGE;

EDGE *firstedge[MAXN]; /* pointer to arbitrary edge out of vertex i. */
int degree[MAXN];

EDGE edges[MAXE];

int nv; //number of vertices
int ni; //number of intersections
int ne; //number of (undirected) edges in the cross graph

//////////////////////////////////////////////////////////////////////////////

//=============== Writing planar_code of graph ===========================

void writePlanarCodeChar(){
    int i;
    EDGE *e, *elast;
    
    //write the number of vertices
    fputc(nv + ni, stdout);
    
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

void writePlanarCodeShort(){
    int i;
    EDGE *e, *elast;
    
    fputc(0, stdout);
    //write the number of vertices
    writeShort(nv + ni);
    
    
    for(i=0; i<nv+ni; i++){
        e = elast = firstedge[i];
        do {
            writeShort(e->end + 1);
            e = e->next;
        } while (e != elast);
        writeShort(0);
    }
}

void writePlanarCode(){
    static int first = TRUE;
    
    if(first){
        first = FALSE;
        
        fprintf(stdout, ">>planar_code<<");
    }
    
    if (nv + ni + 1 <= 255) {
        writePlanarCodeChar();
    } else if (nv + ni + 1 <= 65535) {
        writePlanarCodeShort();
    } else {
        fprintf(stderr, "Graphs of that size are currently not supported -- exiting!\n");
        exit(-1);
    }
    
}
//=============== Reading and decoding planarcode ===========================

EDGE *findEdge(int from, int to) {
    EDGE *e, *elast;

    e = elast = firstedge[from];
    do {
        if (e->end == to) {
            return e;
        }
        e = e->next;
    } while (e != elast);
    fprintf(stderr, "error while looking for edge from %d to %d.\n", from, to);
    exit(0);
}

void decodeThrackleCode(unsigned short* code) {
    /* complexity of method to determine inverse isn't that good, but will have to satisfy for now
     */
    int i, j, codePosition;
    int edgeCounter = 0;
    EDGE *inverse;

    nv = code[0];
    ni = code[1];
    codePosition = 2;

    for (i = 0; i < nv + ni; i++) {
        degree[i] = 0;
        firstedge[i] = edges + edgeCounter;
        edges[edgeCounter].start = i;
        edges[edgeCounter].end = code[codePosition] - 1;
        edges[edgeCounter].next = edges + edgeCounter + 1;
        if (code[codePosition] - 1 < i) {
            inverse = findEdge(code[codePosition] - 1, i);
            edges[edgeCounter].inverse = inverse;
            inverse->inverse = edges + edgeCounter;
        } else {
            edges[edgeCounter].inverse = NULL;
        }
        edgeCounter++;
        codePosition++;
        for (j = 1; code[codePosition]; j++, codePosition++) {
            edges[edgeCounter].start = i;
            edges[edgeCounter].end = code[codePosition] - 1;
            edges[edgeCounter].prev = edges + edgeCounter - 1;
            edges[edgeCounter].next = edges + edgeCounter + 1;
            if (code[codePosition] - 1 < i) {
                inverse = findEdge(code[codePosition] - 1, i);
                edges[edgeCounter].inverse = inverse;
                inverse->inverse = edges + edgeCounter;
            } else {
                edges[edgeCounter].inverse = NULL;
            }
            edgeCounter++;
        }
        firstedge[i]->prev = edges + edgeCounter - 1;
        edges[edgeCounter - 1].next = firstedge[i];
        degree[i] = j;

        codePosition++; /* read the closing 0 */
    }

    ne = edgeCounter;
}

/**
 * 
 * @param code
 * @param laenge
 * @param file
 * @return returns 1 if a code was read and 0 otherwise. Exits in case of error.
 */
int readThrackleCode(unsigned short code[], int *length, FILE *file) {
    static int first = 1;
    unsigned char c;
    char testheader[20];
    int bufferSize, zeroCounter;
    
    int readCount;


    if (first) {
        first = 0;

        if (fread(&testheader, sizeof (unsigned char), 15, file) != 15) {
            fprintf(stderr, "can't read header ((1)file too small)-- exiting\n");
            exit(1);
        }
        testheader[15] = 0;
        if (strcmp(testheader, ">>thrackle_code") == 0) {

        } else {
            fprintf(stderr, "No thrackle_code header detected -- exiting!\n");
            exit(1);
        }
        //read reminder of header (either empty or le/be specification)
        if (fread(&c, sizeof (unsigned char), 1, file) == 0) {
            return FALSE;
        }
        while (c!='<'){
            if (fread(&c, sizeof (unsigned char), 1, file) == 0) {
                return FALSE;
            }
        }
        //read one more character
        if (fread(&c, sizeof (unsigned char), 1, file) == 0) {
            return FALSE;
        }
    }

    /* possibly removing interior headers -- only done for planarcode */
    if (fread(&c, sizeof (unsigned char), 1, file) == 0) {
        //nothing left in file
        return (0);
    }

    if (c == '>') {
        // could be a header, or maybe just a 62 (which is also possible for unsigned char
        code[0] = c;
        bufferSize = 1;
        zeroCounter = 0;
        code[1] = (unsigned short) getc(file);
        if (code[1] == 0) zeroCounter++;
        code[2] = (unsigned short) getc(file);
        if (code[2] == 0) zeroCounter++;
        bufferSize = 3;
        // 3 characters were read and stored in buffer
        if ((code[1] == '>') && (code[2] == 't')) /*we are sure that we're dealing with a header*/ {
            while ((c = getc(file)) != '<');
            /* read 2 more characters: */
            c = getc(file);
            if (c != '<') {
                fprintf(stderr, "Problems with header -- single '<'\n");
                exit(1);
            }
            if (!fread(&c, sizeof (unsigned char), 1, file)) {
                //nothing left in file
                return (0);
            }
            bufferSize = 1;
            zeroCounter = 0;
        }
    } else {
        //no header present
        bufferSize = 1;
        zeroCounter = 0;
    }

    if (c != 0) /* unsigned chars would be sufficient */ {
        code[0] = c;
        code[bufferSize] = (unsigned short) getc(file);
        bufferSize++;
        if (code[0] + code[1] > MAXN) {
            fprintf(stderr, "Constant N too small %d > %d \n", code[0] + code[1], MAXN);
            exit(1);
        }
        while (zeroCounter < code[0] + code[1]) {
            code[bufferSize] = (unsigned short) getc(file);
            if (code[bufferSize] == 0) zeroCounter++;
            bufferSize++;
        }
    } else {
        readCount = fread(code, sizeof (unsigned short), 1, file);
        if(!readCount){
            fprintf(stderr, "Unexpected EOF.\n");
            exit(1);
        }
        readCount = fread(code + 1, sizeof (unsigned short), 1, file);
        if(!readCount){
            fprintf(stderr, "Unexpected EOF.\n");
            exit(1);
        }
        if (code[0] + code[1] > MAXN) {
            fprintf(stderr, "Constant N too small %d > %d \n", code[0] + code[1], MAXN);
            exit(1);
        }
        bufferSize = 2;
        zeroCounter = 0;
        while (zeroCounter < code[0] + code[1]) {
            readCount = fread(code + bufferSize, sizeof (unsigned short), 1, file);
            if(!readCount){
                fprintf(stderr, "Unexpected EOF.\n");
                exit(1);
            }
            if (code[bufferSize] == 0) zeroCounter++;
            bufferSize++;
        }
    }

    *length = bufferSize;
    return (1);


}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s writes the cross graphs of thrackles in planar_code.\n\n", name);
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
    
    /*=========== read graph ===========*/

    unsigned short code[MAXCODELENGTH];
    int length;
    while (readThrackleCode(code, &length, stdin)) {
        decodeThrackleCode(code);
        writePlanarCode();
    }
    
    return EXIT_SUCCESS;
}
