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

typedef struct {
	GLubyte   red;
	GLubyte   green;
	GLubyte blue;
} My_Color;

enum {
	MY_CREATEBEZIER = CAGD_USER,
	MY_CREATEBSPLINE,
	MY_CONNECTC0,
	MY_CONNECTG1,
	MY_CONNECTC1,

	MY_CLEARCURVE,
	MY_CLEARALL,

	MY_DRAW_CONTROLPOLYGONS,
	MY_DRAW_HODOGRAPHS,
	MY_DRAWHODOGRAPHS,
	MY_DRAW_WEIGHTS,

	M_PROPERTIES,

	MY_VIEWKNOTS,
	MY_HIDEKNOTS,
	MY_INSERTKNOT,
	MY_REMOVEKNOT,

	MY_INSERTPOINT,
	MY_REMOVEPOINT,
	MY_APPENDPOINT,
	MY_PREPENDPOINT,

	MY_DRAWWEIGHTINDEX,
	MY_DRAWHODOGRAPHINDEX,
	MY_DRAWCONTROLPOLYGONINDEX,

	MY_BSPLINEFLOAT,
	MY_BSPLINECLAMPED,

	MY_BEZIERTOBSPLINE,
	MY_MODIFYCOLOR,
	M_DEFAULT,

};

typedef enum {
	CLICK_CONTROLPOINT,
	CLICK_CURVE,
	CLICK_POLYGON,
	CLICK_WEIGHT_CIRCLE,
	CLICK_KNOT,
	CLICK_KNOTLINE,
	CLICK_NONE,
} CLICK_TYPE;

typedef enum {
	BSPLINE_UNKNOWN,
	BSPLINE_CLAMPED,
	BSPLINE_FLOATING
} BsplineType;

typedef struct{
	boolean weightVectors;
	boolean controlPolygon;
	boolean hodograph;
	boolean knotVector;
	My_Color curveColor;
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
	int index;
	UINT line;
	int breakpoints_num;
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
knotVisualizer KV;


HMENU hMenu,  connectMenu;
HMENU myPopup;
UINT myText;
char myBuffer[BUFSIZ];
char sizeBuffer[BUFSIZ];
char orderBuffer[BUFSIZ];
FILE* fptr;

void myRead(int x, int y, PVOID userData);

void mySave(int x, int y, PVOID userData);

#endif