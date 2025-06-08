


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
void dltpoints(int x, int y, PVOID userData);

void resetConnectArray() {
	indicesToConnect[0] = indicesToConnect[1] = NO_INDEX;
}


int findAvailableIndex() {
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] == NULL) {
			
			return i;
		}
		
	}
	return NO_INDEX;
}

void myMessage(PSTR title, PSTR message, UINT type)
{
	MessageBox(cagdGetWindow(), message, title, MB_OK | MB_APPLMODAL | type);
}

LRESULT CALLBACK myDialogProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_INITDIALOG:
	{
		char orderStr[16];
		char stepStr[32];

		sprintf(orderStr, "%d", defaultDegree + 1);
		sprintf(stepStr, "%lf", stepSize);

		SetDlgItemText(hDialog, IDC_EDIT_ORDER, orderStr);
		SetDlgItemText(hDialog, IDC_EDIT_SSIZE, stepStr);
		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDialog, IDC_EDIT_SSIZE, sizeBuffer, sizeof(sizeBuffer));
			GetDlgItemText(hDialog, IDC_EDIT_ORDER, orderBuffer, sizeof(orderBuffer));
			EndDialog(hDialog, TRUE);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDialog, FALSE);
			return TRUE;
		}
		break;
	}
	return FALSE;
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

	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	CURVE_STRUCT *crv = curveArray[activeIndex];
	if (curveArray[activeIndex] == NULL) {
		curveArray[activeIndex] = (CURVE_STRUCT*)calloc(1, sizeof(CURVE_STRUCT));
		printf("what the fuck??\n");
	}
	crv = curveArray[activeIndex];
	if (crv->pointVec == NULL) {
		crv->pointNum = 1;
		crv->pointVec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT));
		//crv->weightVec = (UINT*)malloc(sizeof(UINT));
		crv->pointDisp = NULL;
		crv->weightVec = NULL;
		//crv->pointDisp = (UINT*)malloc(sizeof(UINT));
		crv->pointVec[0].x = t[0].x;
		crv->pointVec[0].y = t[0].y;
		crv->pointVec[0].z = (double)1;
		crv->hodograph = NO_CURVE;
		crv->curvePolyline = NO_CURVE;
		
		crv->s = default_ds;
		
		//cagdSetColor(255, 0, 0);
	}
	else {
		// clear segments
		clearCurveSegmentsByIndex(activeIndex);
		crv->pointNum++;
		CAGD_POINT* tmp = (CAGD_POINT*)realloc(crv->pointVec, sizeof(CAGD_POINT) * crv->pointNum);
		
		if (tmp == NULL ) {
			fprintf(stderr, "\033[0;31m UNEXPECTED MEMORY ERROR!\n");
			free(crv->pointVec);
			free(crv->weightVec);
			crv->pointNum--;
			return;
		}
		crv->pointVec = tmp;
		
	}
	

	crv->order = crv->pointNum - 1;
	crv->pointVec[(crv->pointNum) - 1].x = t[0].x;
	crv->pointVec[(crv->pointNum) - 1].y = t[0].y;
	crv->pointVec[(crv->pointNum) - 1].z = 1.0;
	createCurveFromIndex(activeIndex);
	cagdRedraw();
}
// to deinfe  a bspline curve of degree p with n+1 points  we have to supply n + p + 2 knots
//ex p=3 n+1=2 => 1 + 3 + 2 = 6
// so I need to add enough knots for it to make sense?
// check for open/ flaoting
void addBspline(int x, int y, PVOID userData) {
	fprintf(stdout,"\033[0mClicked on point (%d,%d) to add to the curve in index %d \n", x, y, activeIndex);
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	CURVE_STRUCT *crv = curveArray[activeIndex];
	if (crv->pointVec == NULL) {
		crv->pointNum = 1;
		crv->splineType = BSPLINE_FLOATING;
		crv->knotNum = 1;
		crv->order = defaultDegree + 1;
		crv->pointVec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT));
		/*crv->weightVec = (UINT*)malloc(sizeof(UINT));
		crv->pointDisp = (UINT*)malloc(sizeof(UINT));*/
		crv->pointDisp = NULL;
		crv->weightVec = NULL;
		crv->knotVec = (double*)malloc(sizeof(double));
		crv->pointVec[0].x = t[0].x;
		crv->pointVec[0].y = t[0].y;
		crv->pointVec[0].z = 1.0;
		crv->knotVec[0] = 0.0;
		crv->polyVec = NO_CURVE;
		crv->curvePolyline = NO_CURVE;
		crv->hodograph = NO_CURVE;
		
		crv->s = default_ds;
		
		//cagdSetColor(255, 0, 0);
	}
	else {

		clearCurveSegmentsByIndex(activeIndex);

		crv->pointNum++;
		crv->knotNum = crv->pointNum + crv->order;
		
		CAGD_POINT* tmp = (CAGD_POINT*)realloc(crv->pointVec, sizeof(CAGD_POINT) * crv->pointNum);
		//UINT* tmpDisp = realloc(crv->pointDisp, sizeof(UINT) * crv->pointNum);
		//crv->pointDisp = tmpDisp;
		//UINT* tmp2 = (UINT*)realloc(crv->weightVec, sizeof(UINT) * crv->pointNum);
		double* tmp3 = (double*)realloc(crv->knotVec, sizeof(double) * crv->knotNum);
		if (tmp == NULL ) {
			fprintf(stderr, "\033[0;31mUNEXPECTED MEMORY ERROR!\n");
			free(crv->pointVec);
			free(crv->weightVec);
			crv->pointNum--;
			return;
		}
		crv->pointVec = tmp;
		crv->knotVec = tmp3;

		crv->pointVec[(crv->pointNum) - 1].x = t[0].x;
		crv->pointVec[(crv->pointNum) - 1].y = t[0].y;
		crv->pointVec[(crv->pointNum) - 1].z = 1.0;

		for (int i = 0; i < crv->knotNum; i++) {
			crv->knotVec[i] = (double)i;
		}

		
	}
	createCurveFromIndex(activeIndex);
	cagdRedraw();
}

int activateCurveFromClick(int x, int y) {
	cagdPick(x, y);
	UINT picked = cagdPickNext();
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] != NULL && (curveArray[i]->curvePolyline == picked || curveArray[i]->polyVec == picked)) {
			cagdSetSegmentColor(curveArray[i]->curvePolyline, 79, 119, 45);
			fprintf(stdout, "\033[0mFOUND CURVE %d\n", i);
			cagdRedraw();
			return i;
		}
	}
	fprintf(stderr, "\033[0;33mCould not find a curve in this spot\n");
	return NO_INDEX;
}


void tempPointTimer(int x, int y, PVOID data) {

		cagdRedraw();

		// Unregister timer when done
		cagdRegisterCallback(CAGD_TIMER, NULL, NULL);
		dltpoints(x, y, (PVOID)data);
}

void displayTangentAtPoint(int index, double t) {
	if (curveArray[activeIndex]->s.hodograph) {
		CAGD_POINT onCurve, onHodograph;
		if (curveArray[index]->isSpline) {
			onCurve = WeightedDeBoor(index, t);
			onHodograph = WeightedDeBoorDerivative(index, t, onCurve);
			onCurve.z = 0;
		}
		else {
			onCurve = weightedDeCasteljau(index, t);
			onHodograph = weightedDeCasteljauDerivative(index, t, onCurve);
			if (onCurve.z) {
				onCurve.x = onCurve.x / onCurve.z;
				onCurve.y = onCurve.y / onCurve.z;
			}
			else {
				onCurve.x = 0;
				onCurve.y = 0;
			}
			onCurve.z = 0;
		}
		crvpt = cagdAddPoint(&onCurve);
		hodopt = cagdAddPoint(&onHodograph);
		cagdSetSegmentColor(crvpt, 0, 0, 255);
		cagdSetSegmentColor(hodopt, 0, 0, 255);
		cagdRedraw();
		cagdRegisterCallback(CAGD_TIMER, tempPointTimer, NULL);
	}
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
	clearCheck(MY_CONNECTC1);
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
	clearCheck(MY_CONNECTG1);
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
	clearCheck(MY_CONNECTC0);
	cagdRedraw();
}

void quitDrag(int x, int y, PVOID useData) {
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	helper[0].x = helper[0].y = helper[0].z = helper[1].x = helper[1].y = helper[1].z = 0.0;
}
void dltpoints(int x, int y, PVOID userData) {

	if (displayHodographFlag) {
		cagdFreeSegment(crvpt);
		cagdFreeSegment(hodopt);
	}
	displayHodographFlag = FALSE;
	quitDrag(x, y, (PVOID)userData);
	cagdRedraw();
}



void displayPoint(int x, int y, PVOID userData) {
	cagdRedraw();
}



void dragCurve(int x, int y, PVOID userData) {
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	t[1] = helper[1];
	
	double newX = t[0].x - t[1].x;
	double newY = t[0].y - t[1].y;
	double deltaX = newX - curveArray[activeIndex]->pointVec[0].x;
	double deltaY = newY - curveArray[activeIndex]->pointVec[0].y;
	translateCurveByDelta(activeIndex, newX - curveArray[activeIndex]->pointVec[0].x, newY - curveArray[activeIndex]->pointVec[0].y);
	if (displayHodographFlag&& curveArray[activeIndex]->s.hodograph && msg_t >= 0.0) {
		CAGD_POINT tangent;
		cagdGetSegmentLocation(crvpt, &tangent);
		tangent.x += deltaX;
		tangent.y += deltaY;
		cagdReusePoint(crvpt, &tangent);

	}
	printf("curve drag...\n");
	cagdRegisterCallback(CAGD_LBUTTONUP, dltpoints, NULL);
	cagdRedraw();
}

void dragControlPoint(int x, int y, PVOID userData) {
	CAGD_POINT t[2];
	cagdToObject(x, y, t);

	double newX = t[0].x;
	double newY = t[0].y;

	double w = curveArray[activeIndex]->pointVec[msg_idx].z; // z holds w
	curveArray[activeIndex]->pointVec[msg_idx].x = newX * w;
	curveArray[activeIndex]->pointVec[msg_idx].y = newY * w;

	createCurveFromIndex(activeIndex);
}

void dragWeight(int x, int y, PVOID userData) {
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	t[1] = helper[1];
	CAGD_POINT p = curveArray[activeIndex]->pointVec[msg_idx];
	double oldWeight = p.z;
	// Center in screen space
	double centerX = p.x / oldWeight;
	double centerY = p.y / oldWeight;
	// Previous R (based on old weight)
	double prevR = fabs(oldWeight * CIRCLE_SCALE); 
	
	double newX = t[0].x;
	double newY = t[0].y;
	double dx = newX - centerX;
	double dy = newY - centerY;
	double newR = sqrt(dx * dx + dy * dy);
	double oldX = t[1].x;
	double oldY = t[1].y;
	
	// original (xw, yw, w)

	//printf("current click %lf %lf. previous click %lf %lf\n", newX, newY, oldX, oldY);
	boolean beforeLeft = oldX < centerX;
	boolean nowLeft = newX < centerX;
	double sign = (beforeLeft == nowLeft) ? 1.0 : -1.0;
	double newWeight = sign * (newR / CIRCLE_SCALE);
	double update = 1 / oldWeight * newWeight;

	curveArray[activeIndex]->pointVec[msg_idx].x *= update;
	curveArray[activeIndex]->pointVec[msg_idx].y *= update;
	curveArray[activeIndex]->pointVec[msg_idx].z *= update;
	/*helper[1].x = t[0].x;
	helper[1].y = t[0].y;*/

	createCurveFromIndex(activeIndex);
}

void dragKnot(int x, int y, PVOID userData) {
	/*
	msg_idx = j;
					helper[1].x = helper[0].x;
					helper[1].y = helper[0].y;
	*/
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	
	double newX = t[0].x;
	double newY = t[0].y;
	int knotIDX = getKnotIndexFromBreakpointIndex(msg_idx);
	printf("%d", knotIDX);
	double newKnotValue = getKnotValueFromCAGDXPosition(newX, curveArray[activeIndex]->knotVec[0], curveArray[activeIndex]->knotVec[curveArray[activeIndex]->knotNum - 1]);
	if (knotIDX != 0 && knotIDX < curveArray[activeIndex]->knotNum - 1) {
		removeKnot(activeIndex, knotIDX);
		knotIDX = insertKnot(activeIndex, newKnotValue);
		msg_idx = getBreakPointIndexFromKnotIndex(curveArray[activeIndex], knotIDX);
		createCurveFromIndex(activeIndex);
	}
	showKnotDisplay(activeIndex);
}


CLICK_TYPE getClickType(int x, int y) {
	UINT picked;
	CAGD_POINT helper2;
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] != NULL) {
			int n = curveArray[i]->pointNum;
			cagdPick(x, y);
			picked = cagdPickNext();
			while (picked) {
				for (int j = 0; j < n; j++) {
					cagdToObject(x, y, helper);
					cagdGetSegmentLocation(curveArray[i]->pointDisp[j], &helper2);
					if (curveArray[i]->pointDisp[j] == picked) {
						fprintf(stdout,"\033[0mclicked on a control point!\n");
						activeIndex = i;
						msg_idx = j;
						helper[1].x = helper[0].x;
						helper[1].y = helper[0].y;
						return CLICK_CONTROLPOINT;
					}
				}
				picked = cagdPickNext();
			}
			cagdPick(x, y);
			picked = cagdPickNext();
			cagdToObject(x, y, helper);
			for (int j = 0; j < n; j++) {
				if (curveArray[i]->weightVec[j] == picked) {
					fprintf(stdout,"\033[0mclicked on a weight circle!\n");
					activeIndex = i;
					msg_idx = j;
					return CLICK_WEIGHT_CIRCLE;
				}
			}
			// if polyline polygon or curve -> move control polygon
			if (curveArray[i]->curvePolyline == picked) {
				fprintf(stdout, "\033[0mclicked on a curve\n");
				activeIndex = i;

				
				int numStepsT = cagdGetSegmentLength(curveArray[i]->curvePolyline);
				CAGD_POINT* stepPoints = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * numStepsT);
				double* tSamples = (double*)malloc(sizeof(double) * numStepsT);
				if (curveArray[i]->isSpline) {
					int degree = curveArray[i]->order - 1;
					double domain_min = curveArray[i]->knotVec[degree];
					double domain_max = curveArray[i]->knotVec[curveArray[i]->knotNum - degree - 1];
					for (int j = 0; j < numStepsT; ++j) {
						tSamples[j] = domain_min + ((double)j / (numStepsT - 1)) * (domain_max - domain_min);
					}
				}
				else {
					for (int j = 0; j < numStepsT; ++j) {
						tSamples[j] = (double)j / (numStepsT - 1);
					}
				}
				cagdGetSegmentLocation(curveArray[i]->curvePolyline, stepPoints);
				// helper[0] has the point we clicked on the screen. 
				double threshold = 1e-3;
				double bestT = msg_t;
				boolean found = FALSE;
				for (int j = 0; j < numStepsT - 1; j++) {
					double ax = stepPoints[j].x, ay = stepPoints[j].y;
					double bx = stepPoints[j+1].x, by = stepPoints[j+1].y;
					double dx = bx - ax, dy = by - ay;
					double len_sq = dx * dx + dy * dy; // length of the tiny polyline segment
					double t_local = 0;
					if (len_sq > MY_ZERO) {
						t_local = ((helper[0].x - ax)*dx + (helper[0].y - ay)*dy) / len_sq;
					}
					t_local = fmin(fmax(t_local, 0), 1); // just clamp it if the t oversteps boudaries

					double proj_x = ax + t_local * dx; //get the value we think it is
					double proj_y = ay + t_local * dy;

					double distsquared = pow((helper[0].x - proj_x), 2) + pow((helper[0].y - proj_y), 2);
						if (distsquared < threshold) {
							bestT = tSamples[j] + t_local * (tSamples[j + 1] - tSamples[j]);
							found = TRUE;
							break;
						}
						
						if (found) {
							fprintf(stdout,"\033[0mClicked on curve at t =~ %lf\n", bestT);
						}
				}
				free(stepPoints);
				stepPoints = NULL;
				free(tSamples);
				tSamples = NULL;

				helper[1].x = helper[0].x - curveArray[activeIndex]->pointVec[0].x;
				helper[1].y = helper[0].y - curveArray[activeIndex]->pointVec[0].y;
				msg_t = bestT;
				return CLICK_CURVE;
			}
			if (curveArray[i]->polyVec == picked) {
				// get polygon pointIndex (closest "before"?)
				CAGD_POINT* tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * curveArray[i]->pointNum);
				cagdGetSegmentLocation(picked, tmp);
				double proj_x, proj_y;
				boolean found = FALSE;
				double threshold = 1e-3; // square of tolerance
				for (int j = 0; j < curveArray[i]->pointNum - 1; ++j) {
					double ax = tmp[j].x, ay = tmp[j].y;
					double bx = tmp[j + 1].x, by = tmp[j + 1].y;
					double px = helper[0].x, py = helper[0].y;

					double dx = bx - ax;
					double dy = by - ay;
					double len_sq = dx * dx + dy * dy;

					double t = 0.0;
					if (len_sq > 1e-12) {
						t = ((px - ax) * dx + (py - ay) * dy) / len_sq;
						if (t < 0) t = 0;
						if (t > 1) t = 1;
					}

					proj_x = ax + t * dx;
					proj_y = ay + t * dy;

					double d2 = (px - proj_x) * (px - proj_x) + (py - proj_y) * (py - proj_y);
					if (d2 < threshold) {
						fprintf(stdout,"\033[0mPicked segment [%d - %d]\n", j, j + 1);
						msg_idx = j;
						found = TRUE;
						break;
					}
				}
				activeIndex = i;
				helper[1].x = helper[0].x - curveArray[activeIndex]->pointVec[0].x;
				helper[1].y = helper[0].y - curveArray[activeIndex]->pointVec[0].y;
				helper[1].z = 0;
				free(tmp);
				tmp = NULL;
				if (found) {
					helper[0].x = proj_x;
					helper[0].y = proj_y;
				}
				helper[0].z = 0;
				return CLICK_POLYGON;
			}
		}
	}

	if (KV.index != NO_INDEX) {
		cagdPick(x, y);
		picked = cagdPickNext();
		while (picked) {
			for (int j = 0; j < KV.breakpoints_num; j++) {
				cagdToObject(x, y, helper);
				//fprintf("(%lf, %lf)  |  (%lf, %lf)\n", helper[0].x, helper[0].y, helper2.x, helper2.y);
				if (KV.knotTriangle[j] == picked) {
					fprintf(stdout,"\033[0mclicked on a knot! point!\n");
					helper[1].x = helper[0].x;
					helper[1].y = helper[0].y;
					msg_idx = j;
					return CLICK_KNOT;
				}
			}
			picked = cagdPickNext();
		}
		cagdPick(x, y);
		picked = cagdPickNext();
		while (picked) {
			cagdToObject(x, y, helper);
			if (KV.line == picked) {
				fprintf(stdout, "\033[0mclicked on a Knot line!\n");
				helper[1].x = helper[0].x;
				helper[1].y = helper[1].y;
				return CLICK_KNOTLINE;
			}
			picked = cagdPickNext();
		}
		
	}
	//fprintf(stdout,"\033[0;33mDidn't find anything here...\n");
	return CLICK_NONE;
}


void defaultLeftClick(int x, int y, PVOID userData) {
	/*if on control polygon or curve:
		move control polygon
	if on weight of control point :
		modify weight of control point.
	If on knot :
		move knot*/

	CLICK_TYPE clk = getClickType(x, y);
	switch (clk) {
	case CLICK_CONTROLPOINT:
		fprintf(stdout,"\033[0mclicked on a control point!\n");
		cagdRegisterCallback(CAGD_MOUSEMOVE, dragControlPoint,NULL);
		cagdRegisterCallback(CAGD_LBUTTONUP, quitDrag, NULL);
		break;
	case CLICK_WEIGHT_CIRCLE:
		fprintf(stdout,"\033[0mclicked on a weight circle!\n");
		cagdRegisterCallback(CAGD_MOUSEMOVE, dragWeight, NULL);
		cagdRegisterCallback(CAGD_LBUTTONUP, quitDrag, NULL);
		break;
	case CLICK_CURVE:
		displayHodographFlag = TRUE;
		cagdRegisterCallback(CAGD_LBUTTONUP, dltpoints, NULL);
		displayTangentAtPoint(activeIndex, msg_t);
		fprintf(stdout,"\033[0mclicked on a curve \n");
		cagdRegisterCallback(CAGD_MOUSEMOVE, dragCurve, NULL);
		cagdRegisterCallback(CAGD_LBUTTONUP, quitDrag, NULL);
		break;
	case CLICK_POLYGON:
		fprintf(stdout,"\[\033[0m\]clicked on a control polygon\n");
		cagdRegisterCallback(CAGD_MOUSEMOVE, dragCurve, NULL);
		cagdRegisterCallback(CAGD_LBUTTONUP, quitDrag, NULL);
		break;
	case CLICK_KNOT:
		cagdRegisterCallback(CAGD_MOUSEMOVE, dragKnot, NULL);
		cagdRegisterCallback(CAGD_LBUTTONUP, quitDrag, NULL);
		break;
	}
	return;
}


void myCommand(int id, int unused, PVOID userData) {
	int idx;
	
	HMENU hMenu = (HMENU)userData;
	CheckMenuItem(hMenu, MY_CREATEBEZIER, MF_UNCHECKED);
	CheckMenuItem(hMenu, MY_CREATEBSPLINE, MF_UNCHECKED);
	UINT lastCheckVal = CheckMenuItem(hMenu, id, MF_CHECKED);
	
	cagdRegisterCallback(CAGD_TIMER, NULL, NULL);
	cagdRegisterCallback(CAGD_LBUTTONDOWN, defaultLeftClick, NULL);
	cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	cagdSetColor(0, 0, 0);
	switch (id) {
	case MY_CREATEBEZIER:
		
		if (curveCount == MAX_CURVES) {
			fprintf(stderr, "\033[0;31mToo many curves on display! Please clear one before creating a new one!");
		}
		idx = findAvailableIndex();
		if (idx == NO_INDEX) {
			fprintf(stderr, "\033[0;31mCouldn't find an index... That's weird...\n");
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
			fprintf(stderr, "\033[0;31mToo many curves on display! Please clear one before creating a new one!");
		}
		idx = findAvailableIndex();
		if (idx == NO_INDEX) {
			fprintf(stderr, "\033[0;31mCouldn't find an index... That's weird...\n");
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
		CheckMenuItem(hMenu, id, MF_UNCHECKED);
		removeAllCurves();
		break;
	case MY_DRAW_CONTROLPOLYGONS:

		if (lastCheckVal == MF_CHECKED) {
			CheckMenuItem(hMenu, id, MF_UNCHECKED);
		}
		drawHideAllControlPolygons();
		break;
	case MY_DRAWHODOGRAPHS:
		if (lastCheckVal == MF_CHECKED) {
			CheckMenuItem(hMenu, id, MF_UNCHECKED);
		}
		drawHideAllHodographs();
		break;
	case MY_DRAW_WEIGHTS:

		if (lastCheckVal == MF_CHECKED) {
			CheckMenuItem(hMenu, id, MF_UNCHECKED);
		}
		drawHideAllWeightVec();
		break;
	case M_PROPERTIES:
		CheckMenuItem(hMenu, id, MF_UNCHECKED);
		int newOrder;
		double newStep;
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_EDIT_PARAMS),
			cagdGetWindow(),
			(DLGPROC)myDialogProc)) {
			if (sscanf(orderBuffer, "%d", &newOrder) == 1) {
				defaultDegree = newOrder - 1;
			}
			else
				myMessage("Bad Params", "Order must be an integer!", MB_ICONERROR);
			if(sscanf(sizeBuffer, "%lf", &newStep) == 1) {
				stepSize = newStep;
			}
			else
				myMessage("Bad Params", "Step size must be a real number!", MB_ICONERROR);
		}
		break;
	case M_DEFAULT:
		break;
	}
	cagdRedraw();
}

void insertPoint(int x, int y, PVOID userData) {
	insertPointInLocation(activeIndex, msg_idx, helper[0]);
	cagdRedraw();
	cagdRegisterCallback(CAGD_LBUTTONDOWN, defaultLeftClick, NULL);
}

void removePoint(int x, int y, PVOID userData) {
	removePointInIndex(activeIndex, msg_idx);
	cagdRedraw();
	cagdRegisterCallback(CAGD_LBUTTONDOWN, defaultLeftClick, NULL);

}

void appendPoint(int x, int y, PVOID userData) {
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	appendControlPoint(activeIndex, t[1]);
	cagdRedraw();
}

void prependPoint(int x, int y, PVOID userData) {
	CAGD_POINT t[2];
	cagdToObject(x, y, t);
	prependControlPoint(activeIndex, t[1]);
	cagdRedraw();
}

void myClickCommand(int id, int unused, PVOID userData) {
	int idx;

	cagdRegisterCallback(CAGD_TIMER, NULL, NULL);
	cagdRegisterCallback(CAGD_LBUTTONDOWN, defaultLeftClick, NULL);
	cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	cagdSetColor(0, 0, 0);
	switch (id) {
	case MY_REMOVEPOINT:{
		removePoint(0, 0, NULL);
		break;
	}
	case MY_INSERTPOINT:{
			insertPoint(helper[0].x, helper[0].y, (PVOID)userData);  // Insert immediately using helper[0]
		break;
	}
	case MY_APPENDPOINT:{
		cagdRegisterCallback(CAGD_LBUTTONDOWN, appendPoint, NULL);
		break;
	}
	case MY_PREPENDPOINT: {
		cagdRegisterCallback(CAGD_LBUTTONDOWN, prependPoint, NULL);
		break;
	}
	case MY_DRAWCONTROLPOLYGONINDEX: {

		curveArray[activeIndex]->s.controlPolygon = !(curveArray[activeIndex]->s.controlPolygon);
		hide_show_by_disp_state(activeIndex);
		break;
	}
	case MY_DRAWHODOGRAPHINDEX: {
		curveArray[activeIndex]->s.hodograph = !(curveArray[activeIndex]->s.hodograph);
		hide_show_by_disp_state(activeIndex);
		break;
	}
	case MY_DRAWWEIGHTINDEX: {
		curveArray[activeIndex]->s.weightVectors = !(curveArray[activeIndex]->s.weightVectors);
		hide_show_by_disp_state(activeIndex);
		break;
	}
	case MY_BSPLINEFLOAT: {
		BsplineFloating(activeIndex);
		break;
	}
	case MY_BSPLINECLAMPED: {
		BsplineClamped(activeIndex);
		break;
	}
	case MY_BEZIERTOBSPLINE: {
		createBsplineFromBezier(activeIndex);
		break;
	}
	case MY_VIEWKNOTS: {
		showKnotDisplay(activeIndex);
		break;
	}
	case MY_HIDEKNOTS: {
		hideKnotDisplay(NO_CURVE);
		break;
	}
	case MY_INSERTKNOT: {
		double knotVal = getKnotValueFromCAGDXPosition(helper[0].x, curveArray[activeIndex]->knotVec[0], curveArray[activeIndex]->knotVec[curveArray[activeIndex]->knotNum - 1]);
		insertKnot(activeIndex, knotVal);
		createCurveFromIndex(activeIndex);
		showKnotDisplay(activeIndex);
		break;
	}
	case MY_REMOVEKNOT: {
		removeKnot(activeIndex, msg_idx);
		createCurveFromIndex(activeIndex);
		showKnotDisplay(activeIndex);
		break;
	}
	case MY_CLEARCURVE: {
		removeCurveFromIndex(activeIndex);
		hideKnotDisplay(activeIndex);
		break;
	}
	case MY_MODIFYCOLOR: {
		CHOOSECOLOR cc = { 0 };
		COLORREF customColors[16] = { 0 };
		cc.lStructSize = sizeof(cc);
		cc.Flags = CC_RGBINIT | CC_FULLOPEN;
		cc.lpCustColors = customColors;
		cc.rgbResult = RGB(0, 0, 0); // default

		if (ChooseColor(&cc)) {
			unsigned char colors[4];
			*(DWORD*)colors = cc.rgbResult;
			setCurveColor(activeIndex, colors[0], colors[1], colors[2]);
		}
		break;
	}
	}
	cagdRedraw();
}

void openRightMenu(int x, int y, PVOID userData) {
	
	CLICK_TYPE clk = getClickType(x, y);
	WORD menu_cmd;
	CURVE_STRUCT * crv = curveArray[activeIndex];
	HMENU contextMenu = CreatePopupMenu();
	cagdRegisterCallback(CAGD_TIMER, NULL, NULL);
	cagdRegisterCallback(CAGD_LBUTTONDOWN, defaultLeftClick, NULL);
	cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	switch (clk) {
	case CLICK_NONE:
		menu_cmd = cagdPostMenu(hMenu, x, y);
		myCommand(menu_cmd, 0, hMenu);
		break;
	case CLICK_CONTROLPOINT:
		AppendMenu(contextMenu, MF_STRING, MY_REMOVEPOINT, "Remove Control Point");
	case CLICK_POLYGON:
		if (clk != CLICK_CONTROLPOINT) {
			AppendMenu(contextMenu, MF_STRING, MY_INSERTPOINT, "Insert Control Point");
		}
	case CLICK_CURVE:
	{
		AppendMenu(contextMenu, MF_STRING, MY_APPENDPOINT, "Append Control Point");
		AppendMenu(contextMenu, MF_STRING, MY_PREPENDPOINT, "Prepend Control Point");
		AppendMenu(contextMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(contextMenu, MF_STRING, MY_DRAWCONTROLPOLYGONINDEX, "Draw Control Polygon");
		AppendMenu(contextMenu, MF_STRING, MY_DRAWWEIGHTINDEX, "Draw Weight Circles");
		AppendMenu(contextMenu, MF_STRING, MY_DRAWHODOGRAPHINDEX, "Draw Hodograph");
		AppendMenu(contextMenu, MF_SEPARATOR, 0, NULL);
		if (crv->isSpline) {
			AppendMenu(contextMenu, MF_STRING, MY_BSPLINEFLOAT, "Change B-Spline to Floating End");
			AppendMenu(contextMenu, MF_STRING, MY_BSPLINECLAMPED, "Change B-Spline to Open End");
			AppendMenu(contextMenu, MF_STRING, MY_VIEWKNOTS, "View Knots");
			

			if (crv->splineType == BSPLINE_FLOATING) {
				EnableMenuItem(contextMenu, MY_BSPLINEFLOAT, MF_GRAYED);
			}
			else if (crv->splineType == BSPLINE_CLAMPED) {
				EnableMenuItem(contextMenu, MY_BSPLINECLAMPED, MF_GRAYED);
			}
		}
		else {
			AppendMenu(contextMenu, MF_STRING, MY_BEZIERTOBSPLINE, "Transform Bezier to Bspline");
		}
		AppendMenu(contextMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(contextMenu, MF_STRING, MY_MODIFYCOLOR, "Change curve color");
		AppendMenu(contextMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(contextMenu, MF_STRING, MY_CLEARCURVE, "Clear Curve");
		break;
	}
	case CLICK_KNOT:
		AppendMenu(contextMenu, MF_STRING, MY_REMOVEKNOT, "Remove Knot");
	case CLICK_KNOTLINE: {
		AppendMenu(contextMenu, MF_STRING, MY_INSERTKNOT, "Insert Knot");
		AppendMenu(contextMenu, MF_STRING, MY_HIDEKNOTS, "Hide Knots");
	}
	
	}
	cagdRegisterCallback(CAGD_CLICKMENU, myClickCommand, (PVOID)contextMenu);
	WORD l = cagdPostMenu(contextMenu, x, y);
	if (l) {
		myClickCommand(l, 0, userData);
	}
	
	DestroyMenu(contextMenu);
}
void attachConsole() {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
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
	default_ds.knotVector = FALSE;
	default_ds.curveColor.red = default_ds.curveColor.green = default_ds.curveColor.blue = 0;
	KV.index = NO_INDEX;
	KV.knotTriangle = NULL;
	KV.line = NO_CURVE;
	KV.multiplicity = NULL;
	KV.breakpoints_num = 0;
	KV.normalizedBreakPoints = NULL;
	msg_t = 0;
	msg_idx = NO_INDEX;
	crvpt = UINT_MAX;
	hodopt = UINT_MAX;
	displayHodographFlag = FALSE;
	
	attachConsole();
}


int main(int argc, char *argv[])
{
	initializeGlobals();

	
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

	cagdRegisterCallback(CAGD_EXIT, myExit, NULL);

	//cagdShowHelp();
	cagdMainLoop();
	return 0;
}
