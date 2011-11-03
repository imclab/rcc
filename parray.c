/*
 * An array of points with a length.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "point.h"

typedef struct parray {
	int len;
	point* points;
} parray;

signed int addpa(parray* pa, point* p);
signed int rempa(parray* pa, point* p);
parray* mergepa(parray* pa1, parray* pa2);
size_t lenpa(parray* pa);
void tostringpa(parray* pa);

/* adds a point to the array */
signed int addpa(parray* pa, point* p) {
	if(pa && p) {
		printf("Adding point (%.2f, %.2f, %.2f) to point array with %d points.\n", p->x, p->y, p->z, pa->len);

		pa->points = (point*) realloc(pa->points, sizeof(point) * (++pa->len));
		printf("added point.\n");
		pa->points[pa->len - 1] = *p;
		return 1;
	}
	return -1;
}

/* performs a brute force search through the array and removes all matching points */
signed int rempa(parray* pa, point* p) {
	unsigned int i;
	if(pa && p) {
		for(i = 0; i < pa->len; i++) {
			if(peq(p, &pa->points[i])) {
				memmove(&pa->points[i], &pa->points[--pa->len], sizeof(point));
				pa->points = (point*) realloc(pa->points, sizeof(point) * pa->len);
			}
		}
		return 1;
	}
	return -1;
}

/* returns a parray that contains the contents of both arguments */
parray* mergepa(parray* pa1, parray* pa2) {
	parray* sumpa = malloc(sizeof(parray));

	sumpa->len = pa1->len + pa2->len;
	sumpa->points = (point*) realloc(sumpa->points, sizeof(point) * sumpa->len);
	memcpy(sumpa->points, pa1->points, pa1->len * sizeof(point));
	memcpy(&sumpa->points[pa1->len], pa2->points, pa2->len * sizeof(point));
	
	return sumpa;
}

/* returns the number of points in the array (parray.len) */
size_t lenpa(parray* pa) {
	return pa->len;
}

/* prints the coordinates of all points in the parray */
/* TODO: should return a string representation not print */
void tostringpa(parray* pa) {
	int i;
	for(i = 0; i < pa->len; i++) {
		printf("%s\n", tostringp(&pa->points[i]));
	}
}

void defaultpa(parray* pa){
	pa->len = 0;
	pa->points = (point*)malloc(sizeof(point));
}
