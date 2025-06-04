#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <cagd.h>
#include <stdio.h>

#define MAX_CURVES 32
#define NO_INDEX -3
#define CIRCLE_PTS 64
#define PI 3.141592653589793
#define NO_CURVE -1
#define MY_ZERO 1e-8
#define CIRCLE_SCALE 0.1

enum {
	MY_CREATEBEZIER = CAGD_USER,
	MY_CREATEBSPLINE,
	MY_CONNECTC0,
	MY_CONNECTG1,
	MY_CONNECTC1,
	MY_CLEARALL,
	MY_DRAW_CONTROLPOLYGONS,
	MY_DRAW_HODOGRAPHS,
	M_PROPERTIES,
	M_DEFAULT,
	MY_DRAWHODOGRAPHS,
	MY_DRAW_WEIGHTS,
};


typedef enum {
	BSPLINE_UNKNOWN,
	BSPLINE_CLAMPED,
	BSPLINE_FLOATING
} BsplineType;

typedef struct{
	boolean weightVectors;
	boolean controlPolygon;
	boolean hodograph;
} DISPLAY_STATUS;



typedef struct {
	boolean isSpline;
	BsplineType splineType;
	int order;
	int knotNum;
	int pointNum;
	double* knotVec;
	CAGD_POINT *pointVec;
	UINT* weightVec; // has all the circles of the weight
	UINT* pointDisp;
	UINT polyVec; // has all linear polys to create the polygon vector
	UINT curvePolyline;
	UINT hodograph;
	int index;
	DISPLAY_STATUS s;
}
CURVE_STRUCT;

// every knot vector is broken down to this. the shape of a knot triangle is a "christmas tree" of triangles, with levels equal to the multiplicity.
// done that way to allow easier handling of multiplicity and order
typedef struct {
	UINT line;
	double* normalizedBreakPoints;
	int* multiplicity;
	UINT* knotTriangle;
} knotVisualizer;


int curveCount;
double stepSize;
CURVE_STRUCT *curveArray[MAX_CURVES];
int activeIndex;
int displayKnotIndex;
int defaultDegree;
int indicesToConnect[2];
CAGD_POINT helper[2];
int msg_idx;
DISPLAY_STATUS default_ds;




HMENU myPopup;
UINT myText;
char myBuffer[BUFSIZ];
FILE* fptr;

void myRead(int x, int y, PVOID userData);

void mySave(int x, int y, PVOID userData);

#endif