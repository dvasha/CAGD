#include <cagd.h>
#include <stdio.h>
#include "resource.h"
#include <expr2tree.h>
#include "calculations.h"
#include "togglers.h"

#if defined(_WIN32)
    #if _MSC_VER >= 1900
	#pragma comment(lib, "legacy_stdio_definitions.lib")
    #endif
#endif


void resetConnectArray() {
	indicesToConnect[0] = indicesToConnect[1] = NO_INDEX;
}


int findAvailableIndex() {
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] == NULL)
			return i;
	}
	return NO_INDEX;
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

void rainbowify(BYTE* r, BYTE* g, BYTE* b) {
	static double h = 0; // persist across calls
	double s = 1.0, v = 1.0; // full saturation & brightness

	// Update hue
	h += 10.0;
	if (h >= 360.0) h -= 360.0;

	// Convert HSV → RGB
	double c = v * s;
	double x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
	double m = v - c;

	double r1, g1, b1;
	if (h < 60) { r1 = c; g1 = x; b1 = 0; }
	else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
	else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
	else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
	else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
	else { r1 = c; g1 = 0; b1 = x; }

	*r = (BYTE)((r1 + m) * 255);
	*g = (BYTE)((g1 + m) * 255);
	*b = (BYTE)((b1 + m) * 255);
}

void addBezier(int x, int y, PVOID userData) {
	//printf("Clicked on point (%d,%d) to add to the curve in index %d \n", x, y, activeIndex);
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	CURVE_STRUCT *crv = curveArray[activeIndex];
	if (crv->pointVec == NULL) {
		crv->pointNum = 1;
		crv->pointVec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT));
		crv->weightVec = (UINT*)malloc(sizeof(UINT));
		crv->pointDisp = (UINT*)malloc(sizeof(UINT));
		crv->pointVec[0].x = t[0].x;
		crv->pointVec[0].y = t[0].y;
		crv->pointVec[0].z = (double)1;
		crv->hodograph = NO_CURVE;
		crv->curvePolyline = NO_CURVE;
		crv->pointDisp[0] = cagdAddPoint(&(crv->pointVec[0]));
		crv->s = default_ds;
		
		//cagdSetColor(255, 0, 0);
	}
	else {

		for (int i = 0; i < crv->pointNum; i++) {
			cagdFreeSegment(crv->weightVec[i]);
		}
		cagdFreeSegment(crv->curvePolyline);
		cagdFreeSegment(crv->polyVec);

		crv->pointNum++;
		CAGD_POINT* tmp = (CAGD_POINT*)realloc(crv->pointVec, sizeof(CAGD_POINT) * crv->pointNum);
		UINT* tmp2 = (UINT*)realloc(crv->weightVec, sizeof(UINT) * crv->pointNum);
		if (tmp == NULL || tmp2 == NULL) {
			fprintf(stderr, "UNEXPECTED MEMORY ERROR!\n");
			free(crv->pointVec);
			free(crv->weightVec);
			crv->pointNum--;
			return;
		}
		crv->pointVec = tmp;
		crv->weightVec = tmp2;
		//BYTE r, g, b;
		//cagdGetSegmentColor(crv->polyVec, &r, &g, &b);
		//rainbowify(&r, &g, &b);
		//cagdSetColor(r, g, b);
	}
	
	

	crv->order = crv->pointNum - 1;
	crv->pointVec[(crv->pointNum) - 1].x = t[0].x;
	crv->pointVec[(crv->pointNum) - 1].y = t[0].y;
	crv->pointVec[(crv->pointNum) - 1].z = 1.0;
	createCurveFromIndex(activeIndex);
}
// to deinfe  a bspline curve of degree p with n+1 points  we have to supply n + p + 2 knots
//ex p=3 n+1=2 => 1 + 3 + 2 = 6
// so I need to add enough knots for it to make sense?
// check for open/ flaoting
void addBspline(int x, int y, PVOID userData) {
	printf("Clicked on point (%d,%d) to add to the curve in index %d \n", x, y, activeIndex);
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	CURVE_STRUCT *crv = curveArray[activeIndex];
	if (crv->pointVec == NULL) {
		crv->pointNum = 1;
		crv->splineType = BSPLINE_FLOATING;
		crv->knotNum = 1;
		crv->order = defaultDegree + 1;
		crv->pointVec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT));
		crv->weightVec = (UINT*)malloc(sizeof(UINT));
		crv->pointDisp = (UINT*)malloc(sizeof(UINT));
		crv->knotVec = (double*)malloc(sizeof(double));
		crv->pointVec[0].x = t[0].x;
		crv->pointVec[0].y = t[0].y;
		crv->pointVec[0].z = 1.0;
		crv->knotVec[0] = 0.0;
		crv->polyVec = NO_CURVE;
		crv->curvePolyline = NO_CURVE;
		crv->hodograph = NO_CURVE;
		crv->pointDisp[0] = cagdAddPoint(&(crv->pointVec[0]));
		crv->s = default_ds;
		//cagdSetColor(255, 0, 0);
	}
	else {

		for (int i = 0; i < crv->pointNum; i++) {
			cagdFreeSegment(crv->weightVec[i]);
		}
		cagdFreeSegment(crv->polyVec);

		crv->pointNum++;
		crv->knotNum = crv->pointNum + crv->order;
		
		CAGD_POINT* tmp = (CAGD_POINT*)realloc(crv->pointVec, sizeof(CAGD_POINT) * crv->pointNum);
		UINT* tmp2 = (UINT*)realloc(crv->weightVec, sizeof(UINT) * crv->pointNum);
		double* tmp3 = (double*)realloc(crv->knotVec, sizeof(double) * crv->knotNum);
		if (tmp == NULL || tmp2 == NULL ||tmp3 == NULL) {
			fprintf(stderr, "UNEXPECTED MEMORY ERROR!\n");
			free(crv->pointVec);
			free(crv->weightVec);
			crv->pointNum--;
			return;
		}
		crv->pointVec = tmp;
		crv->weightVec = tmp2;
		crv->knotVec = tmp3;

		crv->pointVec[(crv->pointNum) - 1].x = t[0].x;
		crv->pointVec[(crv->pointNum) - 1].y = t[0].y;
		crv->pointVec[(crv->pointNum) - 1].z = 1.0;

		for (int i = 0; i < crv->knotNum; i++) {
			crv->knotVec[i] = (double)i;
		}

		BYTE r, g, b;
		//cagdGetSegmentColor(crv->polyVec, &r, &g, &b);
		//rainbowify(&r, &g, &b);
		//cagdSetColor(r, g, b);
	}
	createCurveFromIndex(activeIndex);
}

int activateCurveFromClick(int x, int y) {
	cagdPick(x, y);
	UINT picked = cagdPickNext();
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] != NULL && (curveArray[i]->curvePolyline == picked || curveArray[i]->polyVec == picked)) {
			cagdSetSegmentColor(curveArray[i]->curvePolyline, 255, 255, 255);
			fprintf(stdout, "FOUND CURVE %d\n", i);
			cagdRedraw();
			return i;
		}
	}
	fprintf(stderr, "\033[31m%s\033[0m", "Could not find a curve in this spot\n");
	return NO_INDEX;
}

void ConnectC1(int x, int y, PVOID userData) {
	int pick = activateCurveFromClick(x, y);
	if (pick == NO_INDEX) {
		return;
	}
	if (indicesToConnect[0] == NO_INDEX) {
		indicesToConnect[0] = pick;
		return;
	}
	indicesToConnect[1] = pick;
	cagdSetSegmentColor(curveArray[indicesToConnect[0]]->curvePolyline, 0, 0, 0);
	cagdSetSegmentColor(curveArray[indicesToConnect[1]]->curvePolyline, 0, 0, 0);
	// using the hodograph, we will get the last tangent of curve 1 and first tangent of curve 2 and by that create the rotation matrix.


	ConnectC1Aux();
	resetConnectArray();
	cagdRedraw();
}
void ConnectG1(int x, int y, PVOID userData) {
	int pick = activateCurveFromClick(x, y);
	if (pick == NO_INDEX) {
		return;
	}
	if (indicesToConnect[0] == NO_INDEX) {
		indicesToConnect[0] = pick;
		return;
	}
	indicesToConnect[1] = pick;
	cagdSetSegmentColor(curveArray[indicesToConnect[0]]->curvePolyline, 0, 0, 0);
	cagdSetSegmentColor(curveArray[indicesToConnect[1]]->curvePolyline, 0, 0, 0);
	// using the hodograph, we will get the last tangent of curve 1 and first tangent of curve 2 and by that create the rotation matrix.

	ConnectC0Aux();
	ConnectG1Aux();
	resetConnectArray();
	cagdRedraw();
}
void ConnectC0(int x, int y, PVOID userData) {

	int pick = activateCurveFromClick(x, y);
	if (pick == NO_INDEX) {
		return;
	}
	if (indicesToConnect[0] == NO_INDEX) {
		indicesToConnect[0] = pick;
		return;
	}
	indicesToConnect[1] = pick;
	cagdSetSegmentColor(curveArray[indicesToConnect[0]]->curvePolyline, 0, 0, 0);
	cagdSetSegmentColor(curveArray[indicesToConnect[1]]->curvePolyline, 0, 0, 0);
	// to connect two curves in C0 manner, we need to move the second curve first point to be in the first curve last point
	ConnectC0Aux();
	resetConnectArray();
	cagdRedraw();
}

void createBsplineFromBezier(int index) {
	CURVE_STRUCT *crv = curveArray[index];
	if (crv->isSpline) {
		fprintf(stdout, "This curve is already a bspline!\n");
		return;
	}
	clearCurveSegmentsByIndex(index);
	crv->isSpline = TRUE;
	crv->knotNum = crv->pointNum * 2;
	for (int i = 0; i < crv->pointNum; i++) {
		crv->knotVec[i] = 0;
		crv->knotVec[i + crv->pointNum] = 1.0;
	}
	createCurveFromIndex(index);
}

void dragCurve(int x, int y, PVOID userData) {
	printf("%d, %d\n", x, y);
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	t[1] = helper[1];
	
	double newX = t[0].x - t[1].x;
	double newY = t[0].y - t[1].y;
	printf("%lf, %lf || %lf, %lf\n", t[0].x, t[0].y, t[1].x, t[1].y);
	translateCurveByDelta(activeIndex, newX - curveArray[activeIndex]->pointVec[0].x, newY - curveArray[activeIndex]->pointVec[0].y);
}

void dragControlPoint(int x, int y, PVOID userData) {
	printf("%d, %d\n", x, y);
	CAGD_POINT t[2];
	cagdToObject(x, y, t);

	double newX = t[0].x;
	double newY = t[0].y;

	printf("%lf, %lf || %lf, %lf\n", t[0].x, t[0].y, helper[1].x, helper[1].y);

	curveArray[activeIndex]->pointVec[msg_idx].x = newX;
	curveArray[activeIndex]->pointVec[msg_idx].y = newY;

	createCurveFromIndex(activeIndex);
}

void dragWeight(int x, int y, PVOID userData) {
	
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	t[1] = helper[1];

	double oldWeight = curveArray[activeIndex]->pointVec[msg_idx].z;
	double newX = t[0].x;
	double newY = t[0].y;
	double newR = sqrt((newX - curveArray[activeIndex]->pointVec[msg_idx].x / oldWeight) * (newX - curveArray[activeIndex]->pointVec[msg_idx].x / oldWeight)
						+ (newY - curveArray[activeIndex]->pointVec[msg_idx].y / oldWeight)* (newY - curveArray[activeIndex]->pointVec[msg_idx].y / oldWeight));

	
	double newWeight = newR / CIRCLE_SCALE;

	
	// original (xw, yw, w)
	double update = 1 / oldWeight * newWeight;

	printf("old weight = %lf \told points : %lf %lf %lf\n ", oldWeight, curveArray[activeIndex]->pointVec[msg_idx].x, curveArray[activeIndex]->pointVec[msg_idx].y, curveArray[activeIndex]->pointVec[msg_idx].z);

	curveArray[activeIndex]->pointVec[msg_idx].x *= update;
	curveArray[activeIndex]->pointVec[msg_idx].y *= update;
	curveArray[activeIndex]->pointVec[msg_idx].z *= update;
	printf("new weight = %lf \tnew points : %lf %lf %lf\n", newWeight, curveArray[activeIndex]->pointVec[msg_idx].x, curveArray[activeIndex]->pointVec[msg_idx].y, curveArray[activeIndex]->pointVec[msg_idx].z);

	createCurveFromIndex(activeIndex);
}

void quitDrag(int x, int y, PVOID useData) {
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	helper[0].x = helper[0].y = helper[0].z = helper[1].x = helper[1].y = helper[1].z = 0.0;
}

void defaultLeftClick(int x, int y, PVOID userData) {
	/*if on control polygon or curve:
		move control polygon
	if on weight of control point :
		modify weight of control point.
	If on knot :
		move knot*/
	UINT picked;
	boolean found = FALSE;
	CAGD_POINT helper2;

	for (int i = 0; i < MAX_CURVES; i++) {
		if (found) {
			break;
		}
		if (curveArray[i] != NULL) {
			int n = curveArray[i]->pointNum;
			cagdPick(x, y);
			picked = cagdPickNext();
			while (picked && !found) {
				for (int j = 0; j < n; j++) {
					cagdToObject(x, y, helper);
					cagdGetSegmentLocation(curveArray[i]->pointDisp[j], &helper2);
					//printf("(%lf, %lf)  |  (%lf, %lf)\n", helper[0].x, helper[0].y, helper2.x, helper2.y);
					if (curveArray[i]->pointDisp[j] == picked) {
						printf("clicked on a control point!\n");
						activeIndex = i;
						
						helper[1].x = helper[0].x;
						helper[1].y = helper[0].y;
						cagdRegisterCallback(CAGD_MOUSEMOVE, dragControlPoint, msg_idx = j);
						cagdRegisterCallback(CAGD_LBUTTONUP, quitDrag, NULL);
						found = TRUE;
						break;
					}
				}
				picked = cagdPickNext();
			}
			cagdPick(x, y);
			picked = cagdPickNext();
			cagdToObject(x, y, helper);
			if (!found) {
				for (int j = 0; j < n; j++) {
					if (curveArray[i]->weightVec[j] == picked) {
						printf("clicked on a weight circle!\n");
						found = TRUE;
						
						activeIndex = i;
						cagdRegisterCallback(CAGD_MOUSEMOVE, dragWeight, msg_idx = j);
						cagdRegisterCallback(CAGD_LBUTTONUP, quitDrag, NULL);
						break;
					}
				}
			}
			// if polyline polygon or curve -> move control polygon
			if (!found) {
				if (curveArray[i]->curvePolyline == picked || curveArray[i]->polyVec == picked) {
					printf("clicked on a curve or its control polygon\n");
					activeIndex = i;
					helper[1].x = helper[0].x - curveArray[activeIndex]->pointVec[0].x;
					helper[1].y = helper[0].y - curveArray[activeIndex]->pointVec[0].y;
					cagdRegisterCallback(CAGD_MOUSEMOVE, dragCurve, NULL);
					cagdRegisterCallback(CAGD_LBUTTONUP, quitDrag, NULL);
					found = TRUE;
					break;
				}
			}

		}
	}
	if (!found) {
		printf("Didn't find anything here...\n");
	}
}


CAGD_CALLBACK myCommand(int id, int unused, PVOID userData) {
	int idx;
	static state = M_DEFAULT;
	HMENU hMenu = (HMENU)userData;
	
	if (id == state) {
		// Toggle off
		CheckMenuItem(hMenu, id, MF_UNCHECKED);
		state = M_DEFAULT;
		id = M_DEFAULT;
	}
	else {
		// Switch to new item
		CheckMenuItem(hMenu, state, MF_UNCHECKED);
		CheckMenuItem(hMenu, id, MF_CHECKED);
		state = id;
	}
	cagdRegisterCallback(CAGD_TIMER, NULL, NULL);
	cagdRegisterCallback(CAGD_LBUTTONDOWN, defaultLeftClick, NULL);
	cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	cagdSetColor(0, 0, 0);
	switch (id) {
	case MY_CREATEBEZIER:
		if (curveCount == MAX_CURVES) {
			fprintf(stderr, "Too many curves on display! Please clear one before creating a new one!");
		}
		idx = findAvailableIndex();
		if (idx == NO_INDEX) {
			fprintf(stderr, "Couldn't find an index... That's weird...\n");
		}
		else {
			activeIndex = idx;
			CURVE_STRUCT *newBezier = (CURVE_STRUCT*)malloc(sizeof(CURVE_STRUCT));
			newBezier->isSpline = FALSE;
			newBezier->order = 0;
			newBezier->knotNum = 0;
			newBezier->pointNum = 0;
			newBezier->knotVec = NULL;
			newBezier->pointVec = NULL;
			newBezier->weightVec = NULL;
			newBezier->polyVec = NO_INDEX;
			newBezier->index = idx;
			curveArray[idx] = newBezier;
			cagdRegisterCallback(CAGD_LBUTTONDOWN, addBezier, NULL);
		}
		break;
	case MY_CREATEBSPLINE:
		if (curveCount == MAX_CURVES) {
			fprintf(stderr, "Too many curves on display! Please clear one before creating a new one!");
		}
		idx = findAvailableIndex();
		if (idx == NO_INDEX) {
			fprintf(stderr, "Couldn't find an index... That's weird...\n");
		}
		else {
			activeIndex = idx;
			CURVE_STRUCT *newBspline = (CURVE_STRUCT*)malloc(sizeof(CURVE_STRUCT));
			newBspline->isSpline = TRUE;
			newBspline->order = defaultDegree + 1;
			newBspline->knotNum = 0;
			newBspline->pointNum = 0;
			newBspline->knotVec = NULL;
			newBspline->pointVec = NULL;
			newBspline->weightVec = NULL;
			newBspline->polyVec = NO_INDEX;
			newBspline->index = idx;
			curveArray[idx] = newBspline;
			cagdRegisterCallback(CAGD_LBUTTONDOWN, addBspline, NULL);
		}
		break;
	case MY_CONNECTC0:
		//get first index
		resetConnectArray();
		cagdRegisterCallback(CAGD_LBUTTONDOWN, ConnectC0, NULL);
		break;
	case MY_CONNECTG1:
		resetConnectArray();
		cagdRegisterCallback(CAGD_LBUTTONDOWN, ConnectG1, NULL);
		break;
	case MY_CONNECTC1:
		resetConnectArray();
		cagdRegisterCallback(CAGD_LBUTTONDOWN, ConnectC1, NULL);
		break;
	case MY_CLEARALL:
		removeAllCurves();
		break;
	case MY_DRAW_CONTROLPOLYGONS:
		drawHideAllControlPolygons();
		break;
	case MY_DRAWHODOGRAPHS:
		drawHideAllHodographs();
		break;
	case MY_DRAW_WEIGHTS:
		drawHideAllWeightVec();
		break;
	case M_PROPERTIES:
		break;
	case M_DEFAULT:
		break;
	}
	
	
	cagdRedraw();
}




void openRightMenu(int x, int y, PVOID userData) {
	WORD l = cagdPostMenu(userData, x, y);
	if (l) {
		myCommand(l, 0, userData);
	}
}

void initializeGlobals() {
	curveCount = 0;
	stepSize = 0.01;
	for (int i = 0; i < MAX_CURVES; i++) {
		curveArray[i] = NULL;
	}
	activeIndex = NO_INDEX;
	displayKnotIndex = NO_INDEX;
	defaultDegree = 3;
	resetConnectArray();
	default_ds.controlPolygon = TRUE;
	default_ds.hodograph = FALSE;
	default_ds.weightVectors = FALSE;
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
	AppendMenu(hMenu, MF_STRING | MF_CHECKED, MY_DRAW_CONTROLPOLYGONS, "Draw Control Polygons");
	AppendMenu(hMenu, MF_STRING , MY_DRAWHODOGRAPHS, "Draw Hodographs");
	AppendMenu(hMenu, MF_STRING , MY_DRAW_WEIGHTS, "Draw Weight Circles");
	AppendMenu(hMenu, MF_STRING, M_PROPERTIES, "Properties");
	cagdAppendMenu(hMenu, "Curves");

	

	cagdRegisterCallback(CAGD_MENU, myCommand, (PVOID)hMenu);



	cagdRegisterCallback(CAGD_LOADFILE, myRead, NULL);
	cagdRegisterCallback(CAGD_SAVEFILE, mySave, NULL);
	//cagdRegisterCallback(CAGD_RBUTTONUP, openRightMenu, hMenu);
	cagdRegisterCallback(CAGD_LBUTTONDOWN, defaultLeftClick, NULL);

	cagdRegisterCallback(CAGD_RBUTTONDOWN, openRightMenu, hMenu);


	//cagdShowHelp();
	cagdMainLoop();
	return 0;
}
