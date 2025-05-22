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


enum {
	MY_CLICK = CAGD_USER,
	MY_POLY,
	MY_ANIM,
	MY_DRAG,
	MY_ADD,
	MY_COLOR,
	MY_REMOVE,
	MY_COORD,
	
	M_PROPERTIES,
};
t

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
	double(*pointVec)[3];
	UINT* weightVec;
	UINT* polyVec;
	int index;
}
CURVE_STRUCT;

HMENU myPopup;
UINT myText;
char myBuffer[BUFSIZ];
FILE* fptr;

int curveCount = 0;
double stepSize = 0.1;
CURVE_STRUCT *curveArray[MAX_CURVES];
UINT cagd_curve[MAX_CURVES];
UINT cagd_controlPolygon[MAX_CURVES];
int activeIndex;
int displayKnotIndex;
int defaultDegree;


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

// get B(t) using t, and the (weighted) points
CAGD_POINT DeCasteljau(double t, CAGD_POINT* pts, int numPts) {
	// https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm#Implementations
	CAGD_POINT* B = (CAGD_POINT*)malloc(numPts * sizeof(CAGD_POINT));
	memcpy(B, pts, numPts * sizeof(CAGD_POINT));
	for (int i = 1; i < numPts; i++) {
		for (int j = 0; j < (numPts - i); j++) {
			B[j].x = B[j].x * (1 - t) + B[j + 1].x * t;
			B[j].y = B[j].y * (1 - t) + B[j + 1].y * t;

		}
	}
	CAGD_POINT val = B[0];
	free(B);
	return val;
}


int findAvailableIndex() {
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] = NULL)
			return i;
	}
	return NO_INDEX;
}


// adds points for each curve in the file, then add them to the array of the controlVectors, for further processing.
void myRead(int x, int y, PVOID userData) {
	boolean isBspline = FALSE;
	double* knotVector;
	double (*controlVector)[3];
	UINT* weightVector;
	UINT* polygonVector;
	int order = 0;
	int numPts = 0;
	int numKnots = 0;
	int idx = -1;
	CURVE_STRUCT *newCurve;


	fptr = fopen((char*)x, "r");
	if (fptr) {
		while (fgets(myBuffer, 1024, fptr)) { //reads a line
			idx = findAvailableIndex();
			if (idx < 0 ) {
				printf("TOO MANY CURVE ARE DISPLAYED!\n Please remove a curve before adding a new one.\n");
				return;
			}
			else {
				newCurve = (CURVE_STRUCT*)malloc(sizeof(CURVE_STRUCT));
			}
			if (myBuffer[0] == '#') {
				continue;
			}
			if (myBuffer[0] == '\n') {
				continue;
			}
			if (strstr(myBuffer, "knots")) {
				// we have a bspline
				isBspline = TRUE;
				sscanf(myBuffer, " knots [ %d ] = ", &numKnots);
				knotVector = (double*)malloc(sizeof(double) * numKnots);
				for (int i = 0; i < numKnots; i++) {
					fgets(myBuffer, 1024, fptr);
					sscanf(myBuffer, "%lf ", &knotVector[i]);
				}
				continue;
			}
			else {
				//sscanf(myBuffer, "%d", &numControlPoints[curveCount]);
				sscanf(myBuffer, "%d", &order);
				if (isBspline) {
					numPts = numKnots - order;
				}
				else {
					numPts = order;
				}
				//controlVectors[curveCount] = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * (numControlPoints[curveCount]));
				controlVector = (double *) malloc(sizeof(double[3]) * numPts);
				weightVector = (UINT *)malloc(sizeof(UINT) * numPts);
				polygonVector = (UINT *)malloc(sizeof(UINT) * (numPts-1));
				for (int i = 0; i < numPts;) {
					fgets(myBuffer, 1024, fptr);
					if (myBuffer[0] == '#') {
						continue;
					}
					else if (myBuffer[0] == '\n') {
						continue;
					}
					else {
						if (sscanf(myBuffer, "%lf %lf %lf", &controlVector[i][0], &controlVector[i][1], &controlVector[i][2]) == 2) {
							controlVector[i][2] = 1.0;
						}
						
						i++;
					}
				}
				//add curve to global curve list
				 CURVE_STRUCT tmp = {.isSpline = isBspline,
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
					currCurve->pointVec[pi][0],
					currCurve->pointVec[pi][1],
					currCurve->pointVec[pi][2]
					);
			}
		}
		fclose(fptr);
	}
}



void initializeGlobals() {
	for (int i = 0; i < MAX_CURVES; i++) {
		curveArray[i] = NULL;
	}
}

int main(int argc, char *argv[])
{
	initalizeGlobals();

	HMENU hMenu;
	cagdBegin("CAGD", 512, 512);
	hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, MY_CLICK, "Click");
	AppendMenu(hMenu, MF_STRING, MY_POLY, "Polyline");
	AppendMenu(hMenu, MF_STRING, MY_ANIM, "Animation");
	AppendMenu(hMenu, MF_STRING, MY_DRAG, "Drag, Popup & Dialog");
	cagdAppendMenu(hMenu, "Demos");
	myPopup = CreatePopupMenu();
	AppendMenu(myPopup, MF_STRING | MF_DISABLED, 0, "Point");
	AppendMenu(myPopup, MF_SEPARATOR, 0, NULL);
	AppendMenu(myPopup, MF_STRING, MY_ADD, "Add");
	AppendMenu(myPopup, MF_SEPARATOR, 0, NULL);
	AppendMenu(myPopup, MF_STRING, MY_COLOR, "Change color...");
	AppendMenu(myPopup, MF_STRING, MY_REMOVE, "Remove");
	//cagdRegisterCallback(CAGD_MENU, myCommand, (PVOID)hMenu);


	cagdRegisterCallback(CAGD_LOADFILE, myRead, NULL);
	cagdRegisterCallback(CAGD_SAVEFILE, mySave, NULL);

	
	cagdShowHelp();
	cagdMainLoop();
	return 0;
}
