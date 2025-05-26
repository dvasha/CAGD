#include <cagd.h>
#include <stdio.h>
#include "resource.h"
#include <expr2tree.h>

#if defined(_WIN32)
    #if _MSC_VER >= 1900
	#pragma comment(lib, "legacy_stdio_definitions.lib")
    #endif
#endif

#define MAX_CURVES 32
#define NO_INDEX -3
#define CIRCLE_PTS 64
#define PI 3.141592653589793

enum {
	MY_CREATEBEZIER = CAGD_USER,
	MY_CREATEBSPLINE,
	MY_CONNECTC0,
	MY_CONNECTG1,
	MY_CONNECTC1,
	MY_CLEARALL,
	M_PROPERTIES,
};


char *animText[] = {
	"Animation Demo",
	"During the animation you can freely\n"
	"rotate, translate and scale the scene."
};

char *dragText[] = {
	"Drag, Popups & Dialog Demo",
	"Click right mouse button to call popup menu.\n"
	"A contents of the menu depends on where you\n"
	"clicked. Clicking on an existing point allows\n"
	"you to remove the point or change its color via\n"
	"dialog. If there is no point you could add one.\n\n"
	"Using left mouse button you can drag an existing\n"
	"points. Watch carefully a text appearing near\n"
	"the point being draged and don't miss."
};

char *clickText[] = {
	"Click Demo",
	"CAGD unable to convert 2D point defined by your\n"
	"click to single 3D point located in the object\n"
	"space. Instead it returns you two 3D points\n"
	"as specification of vector. Initially you are\n"
	"looking in the direction of this vector and cannot\n"
	"see anything. Try to rotate the scene after few\n"
	"clicks. You'll see polylines defined by the returned\n"
	"vectors."
};

char *polyText[] = {
	"Polyline Demo",
	"Click polyline. The nearest vertex will be marked with\n"
	"text. Remember that \"nearest\" defined by the minimal\n"
	"distance on the screen (window coordinates) and not\n"
	"in the object space."
};

typedef struct {
	boolean isSpline;
	int order;
	int knotNum;
	int pointNum;
	double* knotVec;
	CAGD_POINT *pointVec;
	UINT* weightVec; // has all the circles of the weight
	UINT polyVec; // has all linear polys to create the polygon vector
	int index;
}
CURVE_STRUCT;

HMENU myPopup;
UINT myText;
char myBuffer[BUFSIZ];
FILE* fptr;

int curveCount;
double stepSize;
CURVE_STRUCT *curveArray[MAX_CURVES];
UINT cagd_curve[MAX_CURVES];
int activeIndex;
int displayKnotIndex;
int defaultDegree;


// CREATE/ REMOVE CURVE

void removeCurveFromIndex(int index) {
	if (curveArray[index] != NULL) {
		for (int i = 0; i < curveArray[index]->pointNum; i++) {
			cagdFreeSegment(curveArray[index]->weightVec);
		}
		for (int i = 0; i < curveArray[index]->pointNum - 1; i++) {
			cagdFreeSegment(curveArray[index]->pointVec);
		}
		cagdFreeSegment(cagd_curve[index]);

		if (curveArray[index]->isSpline) {
			free(curveArray[index]->knotVec);
			curveArray[index]->knotVec = NULL;
		}
		free(curveArray[index]->pointVec);
		curveArray[index]->pointVec = NULL;


		free(curveArray[index]->weightVec);
		curveArray[index]->weightVec = NULL;

		//free(curveArray[index]->polyVec);
		//curveArray[index]->polyVec = NULL;
	}
	free(curveArray[index]);
	curveArray[index] = NULL;
}

void removeAllCurves() {
	for (int i = 0; i < MAX_CURVES; i++) {
		removeCurveFromIndex(i);
	}
}

CAGD_POINT weightedDeCasteljau(int index, double t) {
	int n = curveArray[index]->pointNum;
	int ord = curveArray[index]->order;
	CAGD_POINT result;
	CAGD_POINT *tempPoints = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * n);
	for (int i = 0; i < n; i++) {
		tempPoints[i] = curveArray[index]->pointVec[i];
	}
	for (int k = 1; k <= ord; k++) {
		for (int i = 0; i < n-k; i++) {
			tempPoints[i].x = tempPoints[i].x * (1 - t) + tempPoints[i + 1].x * t;
			tempPoints[i].y = tempPoints[i].y * (1 - t) + tempPoints[i + 1].y * t;
			tempPoints[i].z = tempPoints[i].z * (1 - t) + tempPoints[i + 1].z * t;
		}
	}
	// rational (weighted)
	if(tempPoints[0].z){
		result.x = tempPoints[0].x / tempPoints[0].z;
		result.y = tempPoints[0].y / tempPoints[0].z;
	}
	else {
		result.x = 0;
		result.y = 0;
	}
	result.z = 0;
	free(tempPoints);
	return result;
}

CAGD_POINT NURBS(int index, double t) {
	int n = curveArray[index]->pointNum;
	int k = curveArray[index]->order - 1;
	double* knotVector = curveArray[index]->knotVec;
	CAGD_POINT* pointVector = curveArray[index]->pointVec;
	CAGD_POINT* nurb_a;
	CAGD_POINT result;
	int di;
	if (t == knotVector[n + 1]) {
		di = n;
	}
	else{
		for (di = k; di <= n; di++) {
			if (t >= knotVector[di] && t < knotVector[di + 1]) {
				break;
			}
		}
	}
	nurb_a = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * (k + 1));
	for (int i = 0; i <= k; i++) {
		nurb_a[i] = pointVector[i + di - k];
	}

	for (int p = 1; p <= k; p++) {
		for (int i = k; i >= p; i--) {
			double denom = (knotVector[i + 1 + di - p] - knotVector[i + di - k]);
			double alpha;
			if (denom != 0) {
				 alpha = (t - knotVector[i + di - k]) / denom;
			}
			else {
				alpha = 0;
			}
			nurb_a[i].x = (1.0 - alpha) * nurb_a[i - 1].x + alpha * nurb_a[i].x;
			nurb_a[i].y = (1.0 - alpha) * nurb_a[i - 1].y + alpha * nurb_a[i].y;
			nurb_a[i].z = (1.0 - alpha) * nurb_a[i - 1].z + alpha * nurb_a[i].z;
		}
	}
	result.x = nurb_a[k].x / nurb_a[k].z;
	result.y = nurb_a[k].y / nurb_a[k].z;
	result.z = 0;
	free(nurb_a);
	nurb_a = NULL;
	return result;
}

void printCurve(int index) {
	printf("curve index = %d \n", index);
	printf("isSpline = %d \n", curveArray[index]->isSpline);
	printf("order = %d \n", curveArray[index]->order);
	
	if (curveArray[index]->isSpline) {
		printf("knotNum = %d \n", curveArray[index]->knotNum);
		for (int i = 0; i < curveArray[index]->knotNum; i++) {
			printf("knot[%d] = %lf \n", i, curveArray[index]->knotVec[i]);
		}
	}
	printf("pointNum = %d \n", curveArray[index]->pointNum);
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		printf("point[%d] = %lf \t %lf \t %lf\n", i, curveArray[index]->pointVec[i]);
	}
}

UINT createCurvePolyline(int index) {
	if (curveArray[index] == NULL) {
		printf("unexpected error");
		return;
	}
	printCurve(index);
	UINT polyID;
	if (curveArray[index]->isSpline) {
		int degree = curveArray[index]->order - 1;
		int domain_min = curveArray[index]->knotVec[degree];
		int domain_max = curveArray[index]->knotVec[curveArray[index]->knotNum - degree -1];
		int totalsteps = ((domain_max - domain_min) / stepSize) + 1;
		CAGD_POINT* vec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * (totalsteps));

		double step = 0;
		for (int i = 0; i < totalsteps; i++) {
			
			vec[i] = NURBS(index, step);
			printf("t = %lf \t vec[%d] = %lf %lf %lf\n", step, i, vec[i].x, vec[i].y, vec[i].z);
			step = step + stepSize;
			
		}
		polyID = cagdAddPolyline(vec, totalsteps);
		free(vec);
	}
	else {
		int totalsteps = (1 / stepSize) + 1;
		CAGD_POINT* vec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * totalsteps);
		double step = 0;
		for (int i = 0; i < totalsteps; i++) {
			vec[i] = weightedDeCasteljau(index, step);
			step = step + stepSize;
			if (step > 1.0) {
				step = 1.0;
			}
		}
		polyID = cagdAddPolyline(vec, totalsteps);
		free(vec);
	}
	return polyID;
}


void createWeightCircles(int index) {
	CAGD_POINT circlePoints[CIRCLE_PTS+1];
	double x, y, R;
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		double theta = 0;
		double step = 2* PI / CIRCLE_PTS;
		R = curveArray[index]->pointVec[i].z;
		for (int j = 0; j < CIRCLE_PTS; j++) {
			x = R * cos(theta + step * j);
			y = R * sin(theta + step * j);
			circlePoints[j].x = curveArray[index]->pointVec[i].x + x;
			circlePoints[j].y = curveArray[index]->pointVec[i].y + y;
			circlePoints[j].z = 0;
		}
		circlePoints[CIRCLE_PTS] = circlePoints[0];
		curveArray[index]->weightVec[i] = cagdAddPolyline(circlePoints, CIRCLE_PTS+1);
	}
}

UINT createControlPolygon(int index) {
	return cagdAddPolyline(curveArray[index]->pointVec, curveArray[index]->pointNum);
}

void myMessage(PSTR title, PSTR message, UINT type)
{
	MessageBox(cagdGetWindow(), message, title, MB_OK | MB_APPLMODAL | type);
}


LRESULT CALLBACK myDialogProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_COMMAND)
		return FALSE;
	switch (LOWORD(wParam)) {
	case IDOK:
		GetDlgItemText(hDialog, IDC_EDIT, myBuffer, sizeof(myBuffer));
		EndDialog(hDialog, TRUE);
		return TRUE;
	case IDCANCEL:
		EndDialog(hDialog, FALSE);
		return TRUE;
	default:
		return FALSE;
	}
}


void addBezier(int x, int y, PVOID userData) {
}


CAGD_CALLBACK myCommand(int x, int y, PVOID userData) {

}

void createCurveFromIndex(int index) {
	cagd_curve[index] = createCurvePolyline(index);
	createWeightCircles(index);
	curveArray[index]->polyVec = createControlPolygon(index);
	cagdRedraw();
}

int findAvailableIndex() {
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] == NULL)
			return i;
	}
	return NO_INDEX;
}

// IO
void myRead(int x, int y, PVOID userData) {
	boolean isBspline = FALSE;
	boolean curveIncomplete = FALSE;
	double* knotVector =NULL ;
	CAGD_POINT *controlVector = NULL;
	UINT* weightVector =NULL;
	UINT* polygonVector =NULL;
	int order = 0;
	int numPts = 0;
	int numKnots = 0;
	int idx = -1;
	CURVE_STRUCT *newCurve = NULL;


	fptr = fopen((char*)x, "r");
	if (fptr) {
		while (fgets(myBuffer, 1024, fptr)) { //reads a line
			idx = findAvailableIndex();
			if (idx < 0 ) {
				printf("TOO MANY CURVES ARE DISPLAYED!\n Please remove a curve before adding a new one.\n");
				return;
			}
			else {
				newCurve = (CURVE_STRUCT*)malloc(sizeof(CURVE_STRUCT));
				curveIncomplete = TRUE;
				while (curveIncomplete) {
					if (myBuffer[0] == '#' || myBuffer[0] == '\n') {
						fgets(myBuffer, 1024, fptr);
						continue;
					}
					if (order == 0 && myBuffer[0] >= '0' && myBuffer[0] <= '9') {
						sscanf(myBuffer, "%d", &order);
						fgets(myBuffer, 1024, fptr);
						continue;
					}
					if (strstr(myBuffer, "knots")) {
						// we have a bspline
						int offset = 0;
						int n = 0;
						isBspline = TRUE;
						sscanf(myBuffer, " knots [%d] = %n", &numKnots, &n);
						offset += n;
						knotVector = (double*)malloc(sizeof(double) * numKnots);
						for (int i = 0; i < numKnots; ) {
							while (sscanf(myBuffer + offset, "%lf%n", &knotVector[i], &n) == 1) {
								i++;
								offset += n;
								if (i == numKnots) {
									break;
								}
							}
							fgets(myBuffer, 1024, fptr); //newline
							offset = 0;

						}
					}
					if (isBspline) {
						numPts = numKnots - order;
					}
					else {
						numPts = order;
						order = order - 1;
					}
					controlVector = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * numPts);
					weightVector = (UINT *)malloc(sizeof(UINT) * numPts);
					//polygonVector = (UINT *)malloc(sizeof(UINT) * (numPts - 1));
					for (int i = 0; i < numPts;) {
						fgets(myBuffer, 1024, fptr);
						if (myBuffer[0] == '#' || myBuffer[0] == '\n') {
							continue;
						}
						else {
							if (sscanf(myBuffer, "%lf %lf %lf", &controlVector[i].x, &controlVector[i].y, &controlVector[i].z) == 2) {
								controlVector[i].z = 1.0;
							}
							i++;
						}
						curveIncomplete = FALSE;
					}
					
				}
			
				//add curve to global curve list
				CURVE_STRUCT tmp = { .isSpline = isBspline,
							   .order = order,
							   .knotNum = numKnots,
							   .pointNum = numPts,
							   .knotVec = knotVector,
							   .pointVec = controlVector,
							   .weightVec = weightVector,
							   .polyVec = polygonVector,
							   .index = idx };
				(*newCurve) = tmp;
				curveArray[idx] = newCurve;
				createCurveFromIndex(idx);
			}
		}
		fclose(fptr);
	}
}

void mySave(int x, int y, PVOID userData) {
	CURVE_STRUCT *currCurve;
	fptr = fopen((char*)x, "w");
	if (fptr) {
		fprintf(fptr, "#generated file!\n");
		for (int curveIndex = 0; curveIndex < curveCount; curveIndex++) {
			fprintf(fptr, "##curve %i of %i\n", curveIndex + 1, curveCount);
			currCurve = curveArray[curveIndex];
			if (currCurve->isSpline) {
				fprintf(fptr, "## this curve is a bspline! How fun!");
				fprintf(fptr, "%d\n", currCurve->order);
				fprintf("knots [%d] = \n", currCurve->knotNum);
				for (int i = 0; i < currCurve->knotNum; i++) {
					fprintf(fptr, "%lf\n", currCurve->knotVec[i]);
				}
			}
			else {
				fprintf(fptr, "%d\n", currCurve->pointNum);
			}
			for (int pi = 0; pi < currCurve->pointNum; pi++) {
				fprintf(fptr, "%lf %lf %lf\n",
					currCurve->pointVec[pi].x,
					currCurve->pointVec[pi].y,
					currCurve->pointVec[pi].z
					);
			}
		}
		fclose(fptr);
	}
}

void initializeGlobals() {
	curveCount = 0;
	stepSize = 0.01;
	for (int i = 0; i < MAX_CURVES; i++) {
		curveArray[i] = NULL;
		cagd_curve[i] = NULL;
	}
	activeIndex = NO_INDEX;
	displayKnotIndex = NO_INDEX;
	defaultDegree = 3;
}

int main(int argc, char *argv[])
{
	initializeGlobals();

	HMENU hMenu, myPopup, connectMenu;
	cagdBegin("CAGD", 512, 512);

	//connectMenu
	connectMenu = CreatePopupMenu();
	AppendMenu(connectMenu, MF_STRING, MY_CONNECTC0, "Connect C0");
	AppendMenu(connectMenu, MF_STRING, MY_CONNECTG1, "Connect G1");
	AppendMenu(connectMenu, MF_STRING, MY_CONNECTC1, "Connect C1");
	

	hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, MY_CREATEBEZIER, "Create Bezier");
	AppendMenu(hMenu, MF_STRING, MY_CREATEBSPLINE, "Create B-Spline");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | MF_POPUP, connectMenu , "Connect");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, MY_CLEARALL, "Clear all curves");
	AppendMenu(hMenu, MF_STRING, M_PROPERTIES, "Properties");
	cagdAppendMenu(hMenu, "Curves");


	cagdRegisterCallback(CAGD_MENU, myCommand, (PVOID)hMenu);



	cagdRegisterCallback(CAGD_LOADFILE, myRead, NULL);
	cagdRegisterCallback(CAGD_SAVEFILE, mySave, NULL);

	
	//cagdShowHelp();
	cagdMainLoop();
	return 0;
}
