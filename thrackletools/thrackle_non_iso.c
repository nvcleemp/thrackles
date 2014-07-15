/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE for details.
 */

/* This program reads thrackles from standard in and checks them for isomorphic
 * copies
 * 
 * 
 * Compile with:
 *     
 *     cc -o thrackle_non_iso -O4 thrackle_non_iso.c
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <malloc.h>

#define MAXN 200
#define MAXE (6*MAXN-12)     /* the maximum number of oriented edges in the cross graph */
#define MAXCODELENGTH (MAXN+MAXE+4)
#define MAX_SIZE_CERTIFICATE (2*MAXE + MAXN)

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

typedef struct t /* The data type used for thrackles in the list */ {
    int nv; //number of vertices
    int ni; //number of intersections
    int ne; //number of (undirected) edges in the cross graph
    
    int *certificate;
    int certificateLength;
    
    int original;

    struct t *smaller;
    struct t *larger;
} THRACKLE_LIST_ELEMENT;

THRACKLE_LIST_ELEMENT *thrackleList = NULL;

THRACKLE_LIST_ELEMENT *newThrackleListElement(int nvEl, int niEl, int neEl, int certificate[], int certificateLength){
    int i;
    
    THRACKLE_LIST_ELEMENT *tle = malloc(sizeof(THRACKLE_LIST_ELEMENT));
    
    tle->nv = nvEl;
    tle->ni = niEl;
    tle->ne = neEl;
    tle->certificate = malloc(sizeof(int) * certificateLength);
    for(i = 0; i < certificateLength; i++){ //TODO: memcpy
        tle->certificate[i] = certificate[i];
    }
    tle->certificateLength = certificateLength;
    
    tle->smaller = tle->larger = NULL;
    
    return tle;
}

void freeThrackleListElement(THRACKLE_LIST_ELEMENT *tle){
    if(tle==NULL) return;
    
    freeThrackleListElement(tle->smaller);
    freeThrackleListElement(tle->larger);
    
    if(tle->certificate!=NULL){
        free(tle->certificate);
    }
    
    free(tle);
}

/*
 * 
 * Returns 0 if they are equal, -1 if certificate1 is smallest, 1 if certificate1 is largest
 */
int compareThrackles(int certificate1[], int certificate1Length, int nv1, int ni1, int ne1,
                     int certificate2[], int certificate2Length, int nv2, int ni2, int ne2){
    int i;
    if(nv1 < nv2){
        return -1;
    } else if(nv1 > nv2){
        return 1;
    }
    if(ni1 < ni2){
        return -1;
    } else if(ni1 > ni2){
        return 1;
    }
    if(ne1 < ne2){
        return -1;
    } else if(ne1 > ne2){
        return 1;
    }
    if(certificate1Length < certificate2Length){
        return -1;
    } else if(certificate1Length > certificate2Length){
        return 1;
    }
    
    i = 0;
    while(i < certificate1Length && certificate1[i]==certificate2[i]){
        i++;
    }
    if(i==certificate1Length){
        return 0;
    } else if(certificate1[i] < certificate2[i]) {
        return -1;
    } else {
        return 1;
    }
}

/* Returns a pointer to the THRACKLE_LIST_ELEMENT containing the stored thrackle 
 */
THRACKLE_LIST_ELEMENT *storeThrackle_impl(THRACKLE_LIST_ELEMENT *currentElement, 
        int certificate[], int certificateLength, int nvT, int niT, int neT,
        int number){
    int comparison = compareThrackles(
            certificate, certificateLength, nvT, niT, neT,
            currentElement->certificate, currentElement->certificateLength,
            currentElement->nv, currentElement->ni, currentElement->ne);
    
    if(comparison==0){
        //add to that element
        return currentElement;
    } else if(comparison < 0){
        if(currentElement->smaller==NULL){
            currentElement->smaller = newThrackleListElement(nvT, niT, neT,
                    certificate, certificateLength);
            currentElement->smaller->original = number;
            return currentElement->smaller;
        } else {
            return storeThrackle_impl(currentElement->smaller, 
                    certificate, certificateLength, nvT, niT, neT, number);
        }
    } else { //comparison > 0
        if(currentElement->larger==NULL){
            currentElement->larger = newThrackleListElement(nvT, niT, neT,
                    certificate, certificateLength);
            currentElement->larger->original = number;
            return currentElement->larger;
        } else {
            return storeThrackle_impl(currentElement->larger, 
                    certificate, certificateLength, nvT, niT, neT, number);
        }
    }
}

/* Returns a pointer to the THRACKLE_LIST_ELEMENT containing the stored thrackle 
 */
THRACKLE_LIST_ELEMENT *storeThrackle(int certificate[], int certificateLength,
        int nvT, int niT, int neT, int number){
    if(thrackleList==NULL){
        thrackleList = newThrackleListElement(nvT, niT, neT, certificate, 
                certificateLength);
        thrackleList->original = number;
        return thrackleList;
    } else {
        return storeThrackle_impl(thrackleList, certificate, certificateLength, 
                nvT, niT, neT, number);
    }
}

#define NEW_QUEUE(type, queue, maxSize) type queue[maxSize]; int head, tail; head = tail = 0
#define QUEUE_IS_NOT_EMPTY (head!=tail)
#define POP(queue) queue[tail++]
#define PUSH(queue, value) queue[head++] = (value)

void getCertificateFromEdge(EDGE *e, int certificate[], int *length){
    int labels[MAXN];
    EDGE *newFirstEdge[MAXN];
    EDGE *currentEdge, *edge, *lastEdge;
    int vertexCounter, intersectionCounter;
    int i, pos;
    
    //first we find the new labels
    
    vertexCounter = 0;
    intersectionCounter = nv;
    
    for(i = 0; i < nv + ni; i++){
        labels[i] = -1;
    }
    
    NEW_QUEUE(EDGE *, queue, MAXN);
    
    labels[e->start] = vertexCounter;
    newFirstEdge[vertexCounter] = e;
    vertexCounter++;
    PUSH(queue, e);
    
    while(QUEUE_IS_NOT_EMPTY){
        currentEdge = POP(queue);
        
        edge = lastEdge = currentEdge;
        do {
            if(labels[edge->end]==-1){
                if(edge->end < nv){
                    labels[edge->end] = vertexCounter;
                    newFirstEdge[vertexCounter] = edge->inverse;
                    vertexCounter++;
                } else {
                    labels[edge->end] = intersectionCounter;
                    newFirstEdge[intersectionCounter] = edge->inverse;
                    intersectionCounter++;
                }
                PUSH(queue, edge->inverse);
            }
            edge = edge->next;
        } while (edge != lastEdge);
    }
    
    //construct the new certificate
    pos = 0;
    for(i = 0; i < nv + ni; i++){
        edge = lastEdge = newFirstEdge[i];
        do {
            certificate[pos] = labels[edge->end];
            pos++;
            edge = edge->next;
        } while (edge != lastEdge);
        certificate[pos] = INFI;
        pos++;
    }
    *length = pos;
}

void getCertificateFromEdge_mirrorImage(EDGE *e, int certificate[], int *length){
    int labels[MAXN];
    EDGE *newFirstEdge[MAXN];
    EDGE *currentEdge, *edge, *lastEdge;
    int vertexCounter, intersectionCounter;
    int i, pos;
    
    //first we find the new labels
    
    vertexCounter = 0;
    intersectionCounter = nv;
    
    for(i = 0; i < nv + ni; i++){
        labels[i] = -1;
    }
    
    NEW_QUEUE(EDGE *, queue, MAXN);
    
    labels[e->start] = vertexCounter;
    newFirstEdge[vertexCounter] = e;
    vertexCounter++;
    PUSH(queue, e);
    
    while(QUEUE_IS_NOT_EMPTY){
        currentEdge = POP(queue);
        
        edge = lastEdge = currentEdge;
        do {
            if(labels[edge->end]==-1){
                if(edge->end < nv){
                    labels[edge->end] = vertexCounter;
                    newFirstEdge[vertexCounter] = edge->inverse;
                    vertexCounter++;
                } else {
                    labels[edge->end] = intersectionCounter;
                    newFirstEdge[intersectionCounter] = edge->inverse;
                    intersectionCounter++;
                }
                PUSH(queue, edge->inverse);
            }
            edge = edge->prev;
        } while (edge != lastEdge);
    }
    
    //construct the new certificate
    pos = 0;
    for(i = 0; i < nv + ni; i++){
        edge = lastEdge = newFirstEdge[i];
        do {
            certificate[pos] = labels[edge->end];
            pos++;
            edge = edge->prev;
        } while (edge != lastEdge);
        certificate[pos] = INFI;
        pos++;
    }
    *length = pos;
}

void getCanonicalForm(int certificate[], int *certificateLength){
    int alternateCertificate[MAX_SIZE_CERTIFICATE];
    EDGE *edge, *lastEdge;
    int i, j, length;
    
    getCertificateFromEdge(firstedge[0], certificate, &length);
    
    *certificateLength = length;
    
    for(i = 0; i < nv; i++){
        edge = lastEdge = firstedge[i];
        do {
            getCertificateFromEdge(edge, alternateCertificate, &length);
            if(compareThrackles(certificate, length, nv, ni, ne,
                    alternateCertificate, length, nv, ni, ne) > 0){
                for(j = 0; j < length; j++){
                    certificate[j] = alternateCertificate[j];
                }
            }
            getCertificateFromEdge_mirrorImage(edge, alternateCertificate, &length);
            if(compareThrackles(certificate, length, nv, ni, ne,
                    alternateCertificate, length, nv, ni, ne) > 0){
                for(j = 0; j < length; j++){
                    certificate[j] = alternateCertificate[j];
                }
            }
            
            edge = edge->next;
        } while (edge != lastEdge);
    }
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

//=============== Reading and decoding thrackle_code ===========================

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
    fprintf(stderr, "The program %s reads thrackles from standard in and checks them\n", name);
    fprintf(stderr, "for isomorphic copies.\n\n");
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options]\n\n", name);
    fprintf(stderr, "\nThis program can handle graphs up to %d vertices. Recompile if you need larger\n", MAXN);
    fprintf(stderr, "graphs.\n\n");
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -n, --new\n");
    fprintf(stderr, "       If the program encounters a new thrackle, it is written to stdout.\n");
    fprintf(stderr, "    -v, --verbose\n");
    fprintf(stderr, "       Print more information during the run of the program.\n");
    fprintf(stderr, "    -h, --help\n");
    fprintf(stderr, "       Print this help and return.\n");
}

void usage(char *name) {
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "For more information type: %s -h \n\n", name);
}

int main(int argc, char *argv[]) {
    
    boolean verbose = FALSE;
    boolean exportNew = FALSE;

    /*=========== commandline parsing ===========*/

    int c;
    char *name = argv[0];
    static struct option long_options[] = {
        {"verbose", no_argument, NULL, 'v'},
        {"new", no_argument, NULL, 'n'},
        {"help", no_argument, NULL, 'h'}
    };
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "hvn", long_options, &option_index)) != -1) {
        switch (c) {
            case 'v':
                verbose = TRUE;
                break;
            case 'n':
                exportNew = TRUE;
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
    
    /*=========== read graph ===========*/

    unsigned short code[MAXCODELENGTH];
    int length;
    int thracklesRead = 0;
    int uniqueThrackles = 0;
    int certificate[MAX_SIZE_CERTIFICATE];
    int certificateLength = 0;
    
    while (readThrackleCode(code, &length, stdin)) {
        decodeThrackleCode(code);
        thracklesRead++;
        
        getCanonicalForm(certificate, &certificateLength);
        THRACKLE_LIST_ELEMENT *tle = storeThrackle(
                certificate, certificateLength, nv, ni, ne, thracklesRead);
        if(thracklesRead == tle->original){
            //new thrackle
            uniqueThrackles++;
            if(verbose){
                fprintf(stderr, "Thrackle %d is new.\n", thracklesRead);
            }
            if(exportNew){
                writeThrackleCode();
            }
        } else {
            //isomorphic to some older thrackle
            if(verbose){
                fprintf(stderr, "Thrackle %d is not new. Thrackle %d is a copy.\n",
                        thracklesRead, tle->original);
            }
        }
    }
    
    fprintf(stderr, "Read %d thrackle%s. Read %d unique thrackle%s.\n",
            thracklesRead, thracklesRead == 1 ? "" : "s",
            uniqueThrackles, uniqueThrackles == 1 ? "" : "s");
    
    return EXIT_SUCCESS;
}
