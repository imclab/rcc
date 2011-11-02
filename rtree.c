/*  ______________________
 * |  ____                | * * * * * * * * * * * * *
 * | |    |               |   _         __      __  *
 * | | ___|________       |  ||\\      / _|    / _| *
 * | ||   |        |      |  || \\    / /     / /   *
 * | ||___|  ___   |      |  || //   | |     | |    *
 * |  |     |   |  |      |  ||/\\    \ \_    \ \_  *
 * |  |     |___|  |      |  ||  \\    \__|    \__| *
 * |  |____________|      |                         *
 * |______________________| * * * * * * * * * * * * *
 *
 *	 	 	 	 	RTREE COMPRESSION CODEC
 *
 *	 	BY
 *-------Lane "Laaame" Aasen
 *	 	  ------Eamon "G-Dawg" Gaffney
 *	 	 	 	  ------Michael "Nerdberger" Rosenberger
 *	 	 	 	 	 	  ------Dylan "D-Swag" Swiggett
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "point.h"
#include "rect.h"
#include "parray.h"

static int maxsdev = 30; /* Maximum standard deviation within each rtree */

typedef struct rtree{
	parray pa; /* array of points in the rtree */

	rect mbr; /* Maximum Bounding Rectangle of the node */

	int leaf;/* True if this tree is a leaf, false if a branch */
	/* Possibly make into an n-child rtree, if faster */
	struct rtree * sub1;/* First sub tree if a leaf node */
	struct rtree * sub2;/* Second sub tree if a leaf node */
} rtree;

rtree* putrt(rtree * tree, point * p);
void bputrt(rtree* tree, point* p, int n);
rtree* remrt(rtree * tree, point * p);
double sdevrt(rtree * tree, point * max, point * min);
int subrt(rtree * tree);
int resizert(rtree * tree);
void rebuildrt(rtree * tree);
rtree* pfindrt(rtree* tree, point * p);
point* psinrrt(rtree* tree, rect* qbox);


/* Add the specified point to the specified rtree
 * Can not resize the rtree, only expand it. e.g. a large prism has been predefined
 * so that all points that fall within it will be put in it. After a single point
 * is inserted, the tree should NOT be shrunk to only fit that point.
 */
rtree* putrt(rtree * tree, point * p){
	if (tree->sub1 == NULL && tree->sub2 == NULL){
		tree->leaf = 1;
	}
	if (p != NULL){
		if (tree->leaf){
			addpa(&tree->pa, p);
		} else {
			/* Select the subtree that can contain the point with the least expansion, or
			 * if both require the same expansion, add to the first.
			 */

			double rszsub1 = rszsum(&tree->sub1->mbr, p);
			double rszsub2 = rszsum(&tree->sub2->mbr, p);
			putrt(rszsub1 >= rszsub2 ? tree->sub1 : tree->sub2, p);
		}
	} else {
	  perror("[Error]: putrt() passed a null point pointer");
	}
}

/*
 * Efficiently bulk add all of the points in p.
 * Fails if tree is not a leaf.
 */
void bputrt(rtree* tree, point* p, int n) {
	int i;
	if (tree->leaf){
		tree->pa.points = (point*)realloc(tree->pa.points, (n + tree->pa.len) * sizeof(point));
		memcpy(&tree->pa.points[tree->pa.len], p, n * sizeof(point));
		tree->pa.len += n;
		printf("%d points in tree", (int) tree->pa.len);
		subrt(tree);
		resizert(tree);
	} else {
	  perror("Warning: Did not insert points into non-leaf rtree");
	}
}

/* Recursively find and remove the point from the tree */
/* does not resize or delete the node after deletion */
rtree* remrt(rtree* tree, point * p){
	int i; 
	
	if (tree->sub1 == NULL && tree->sub2 == NULL){
		tree->leaf = 1;
	}
	if (p != NULL) {
		if (tree->leaf){
			for(i = 0; i < tree->pa.len; i++) {
				if(peq(p, &tree->pa.points[i])) {
					rempa(&tree->pa, p);
				}
			}
		} else {
			remrt(tree->sub1, p);
			remrt(tree->sub2, p);
		}
	}
}

/*
 * Returns the standard deviation of the y (depth) value for the rtree.
 * Returns 0 if the tree is not a leaf - a branch can not and should not be subdivided.
 * Currently used for checking if the tree should be subdivided.
 * max and min are used to find the highest and lowest points in the tree.
 */
double sdevrt(rtree * tree, point * max, point * min){
	int i;
	int total;
	point* maxp;
	point* minp;
	double mean;
	double sumsqr;

	if (tree->pa.len <= 1 || !(tree->leaf)){
		return 0;
	}
	total = 0;
	maxp = &(tree->pa.points[0]);/* Keep track of maximum z value point */
	minp = &(tree->pa.points[0]);/* Keep track of minimum z value point */
	for (i = 0; i < tree->pa.len; i++){
		total += tree->pa.points[i].z;
		if (tree->pa.points[i].z > maxp->z){
			maxp = &(tree->pa.points[i]);
		}
		if (tree->pa.points[i].z < minp->z){
			minp = &(tree->pa.points[i]);
		}
	}
	if (max && min){
		/* Transfer x,y,z for max and min points */
		/* free(max); */
		/* free(min); */
		setxyz(max, maxp->x, maxp->y, maxp->z);
		setxyz(min, minp->x, minp->y, minp->z);
	}
	mean = total / tree->pa.len;
	sumsqr = 0;
	for (i = 0; i < tree->pa.len; i++){
	  sumsqr += pow(tree->pa.points[i].z - mean, 2);
	}
	return sqrt(sumsqr / (tree->pa.len - 1));
}

/*
 * Subdivide the selected rtree if a leaf and if meets subdivision reqs
 * Returns false if rtree doesn't need to be subdivided under current rule
*/
int subrt(rtree* tree){
	point* max;
	point* min;
	double sdev;
	int i;
	double sum1;
	double sum2;

	if (tree->leaf){
		max = (point*)malloc(sizeof(point));
		min = (point*)malloc(sizeof(point));
		sdev = sdevrt(tree, max, min);
		/*	printf("max z: %f\nmin z: %f\n", max->z, min->z); */

		if (sdev > maxsdev){
			tree->leaf = 0;
			tree->sub1 = (rtree*)malloc(sizeof(rtree));
			tree->sub2 = (rtree*)malloc(sizeof(rtree));
			/*
			 * Create new subtrees, starting with the lowest and highest z values possible.
			 * Sub1 starts with the highest point, sub2 with the lowest.
			 */
			if (max){
				setxyz(&tree->sub1->mbr.p1, max->x, max->y, max->z);
				setxyz(&tree->sub1->mbr.p2, max->x, max->y, max->z);
			}
			if (min){
				setxyz(&tree->sub2->mbr.p1, min->x, min->y, min->z);
				setxyz(&tree->sub2->mbr.p2, min->x, min->y, min->z);
			}
			for (i = 0; i < tree->pa.len; i++){

				putrt(tree, &tree->pa.points[i]);
				/*			sum1 = rszsum(tree->sub1, &tree->pa.points[i]);
							sum2 = rszsum(tree->sub2, &tree->pa.points[i]);
							if (sum2 > sum1){
							putrt(tree->sub1, &tree->pa.points[i]);
							} else {
							putrt(tree->sub2, &tree->pa.points[i]);
							}*/
			}
			/* Free any unneeded memory in the subtrees, then free the original point array */
			free(tree->pa.points);
			tree->pa.points = NULL;
			tree->pa.len = 0;
			subrt(tree->sub1);
			subrt(tree->sub2);
			free(max);
			free(min);
			return 1;
		}
		free(max);
		free(min);
	} else {
		subrt(tree->sub1);
		subrt(tree->sub2);
	}
	return 0;
}

/* Recursively resize the tree, return false if rebuilding might be necessary */
int resizert(rtree * tree){
	return 0;
}

/* Recursively rebuild the entire tree, optimizing search time */
void rebuildrt(rtree * tree) {

}

/*
 * SINGLE POINT SEARCH
 * Recursively search through the rtree to find the rtree containing the
 * specified point. Return null if the point is not in the tree.
 * Assumes the rtree is properly resized.
 */
rtree* pfindrt(rtree* tree, point * p){
	int i;
	rtree* tree1;
	rtree* tree2;

	if (tree && p){
		if (tree->leaf){
			for (i = 0; i < tree->pa.len; i++){
				if (peq(&tree->pa.points[i], p)){
					return tree;
				}
			}
		} else {
			tree1 = pfindrt(tree->sub1, p);
			if (tree1){
				return tree1;
			}
			tree2 = pfindrt(tree->sub2, p);
			if (tree2){
				return tree2;
			}
		}
	} else {
		perror("[Error]: pfindrt sent a null pointer");
	}
	return NULL;/* Returned if error occurs, or if point is not found */
}

/* returns an array of points which are in the query box */
point* psinrrt(rtree* tree, rect* qbox) {
        int i;
        int numpin = 0;
        if (tree->sub1 == NULL && tree->sub2 == NULL){
                tree->leaf = 1;
        }
        if (tree && qbox) {
                if(tree->leaf) {
                        point* buf = malloc(sizeof(point) * tree->pa.len + 1);
                        for(i = 0; i < tree->pa.len; i++) {
                                if(pinr(qbox, &tree->pa.points[i])) {
                                        buf[++numpin] = tree->pa.points[i];
                                        //add tree->points[i] to an array of points in the qbox
                                }
                        }
                        point* p1 = malloc(sizeof(point));
                        numpin++;
                        setxyz(p1, numpin, -1, -1);
                        buf[0] = *p1;
                        buf = realloc(buf, numpin);
                        return buf;
                } else {
                        int sub1length, sub2length;
                        point* sub1buf;
                        point* sub2buf;
                        if(rinr(&tree->sub1->mbr, qbox)) {
                                point* sub1buf = psinrrt(tree->sub1, qbox);
                                sub1length = sub1buf[0].x;
                                
                                //add the result of psinrrt(tree->sub1, qbox); to an array of points in the qbox
                        }
                        if(rinr(&tree->sub2->mbr, qbox)) {
                                point* sub2buf = psinrrt(tree->sub2, qbox);
                                sub2length = sub2buf[0].x;
                        //repeat above for second subtree
                        }
                        point* sumbuf = malloc((sub1length + sub2length - 2) * sizeof(point));
                        memcpy(sumbuf, &sub1buf[1], sizeof(point) * (sub1length-1));
                        memcpy(&sumbuf[sub1length-1], &sub2buf[1], sizeof(point) * (sub2length-1));
                }
                //return array of points in qbox
        }
        return NULL; //if the program flow reaches here the tree or qbox is null 
}

/* Recursively free the rtree and all of its nodes */
void freert(rtree* tree){
	if (tree->leaf){
		free(tree->pa.points);
	} else {
	  freert(tree->sub1);
	  freert(tree->sub2);
	}
	free(tree);
}
