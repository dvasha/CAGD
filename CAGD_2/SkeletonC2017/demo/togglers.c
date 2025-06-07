#include "structures.h"
#include "calculations.h"
#include "cagd.h"
#include "togglers.h"

void showHodographofCurve(int index) {
	curveArray[index]->s.hodograph = TRUE;
	cagdShowSegment(curveArray[index]->hodograph);
}

void hideHodographofCurve(int index) {
	curveArray[index]->s.hodograph = FALSE;
	cagdHideSegment(curveArray[index]->hodograph);
}

void drawHideAllHodographs() {
	default_ds.hodograph = !default_ds.hodograph;
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] != NULL) {
			if (default_ds.hodograph) {
				showHodographofCurve(i);
			}
			else {
				hideHodographofCurve(i);
			}
		}
	}

}


void hideControlPointsOfCurve(int index) {
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		cagdHideSegment(curveArray[index]->pointDisp[i]);
	}
}

void showControlPointsOfCurve(int index) {
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		cagdShowSegment(curveArray[index]->pointDisp[i]);
	}
}

void showControlPolygonOfCurve(int index) {
	curveArray[index]->s.controlPolygon = TRUE;
	cagdShowSegment(curveArray[index]->polyVec);
}

void hideControlPolygonOfCurve(int index) {
	curveArray[index]->s.hodograph = FALSE;
	cagdHideSegment(curveArray[index]->polyVec);
}

void drawHideAllControlPolygons() {
	default_ds.controlPolygon = !default_ds.controlPolygon;
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] != NULL) {
			if (default_ds.controlPolygon) {
				showControlPolygonOfCurve(i);
			}
			else {
				hideControlPolygonOfCurve(i);
			}
		}
	}
}

void hideWeightVectorofCurve(int index) {
	curveArray[index]->s.weightVectors = FALSE;
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		cagdHideSegment(curveArray[index]->weightVec[i]);
	}
}

void showWeightVectorofCurve(int index) {
	curveArray[index]->s.weightVectors = TRUE;
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		cagdShowSegment(curveArray[index]->weightVec[i]);
	}
}

void drawHideAllWeightVec() {
	default_ds.weightVectors = !default_ds.weightVectors;
	for (int i = 0; i < MAX_CURVES; i++) {
		if (curveArray[i] != NULL) {
			if (default_ds.controlPolygon) {
				showWeightVectorofCurve(i);
			}
			else {
				hideWeightVectorofCurve(i);
			}
		}
	}
}

void hide_show_by_disp_state(int index) {
	if (curveArray[index]->s.controlPolygon) {
		showControlPolygonOfCurve(index);
		showControlPointsOfCurve(index);
	}
	else {
		hideControlPolygonOfCurve(index);
	}
	if (curveArray[index]->s.hodograph) {
		showHodographofCurve(index);
	}
	else {
		hideHodographofCurve(index);
	}
	if (curveArray[index]->s.weightVectors) {
		showWeightVectorofCurve(index);
		showControlPointsOfCurve(index);
	}
	else {
		hideWeightVectorofCurve(index);
	}
	if (!(curveArray[index]->s.weightVectors) && !(curveArray[index]->s.controlPolygon)) {
		hideControlPointsOfCurve(index);
	}
	cagdSetSegmentColor(curveArray[index]->curvePolyline, curveArray[index]->s.curveColor.red, curveArray[index]->s.curveColor.green, curveArray[index]->s.curveColor.blue);
	cagdRedraw();
}

void normalizeBreakPoints(int sz, double* bps, double width, double line_start) {
	// [a,b] -> [linestart,width]
	// [a,b] -> [0,1] = (x-a)/(b-a)
	// [0,1] -> [line_start, width] = y * width + line_start
	double k_max = bps[sz - 1];
	double k_min = bps[0];
	double scale = 1 / (k_max - k_min);
	for (int i = 0; i < sz; i++) {
		bps[i] = (bps[i] - k_min) * scale  * width + line_start;
	}
}

int getBreakPointCount(CURVE_STRUCT *c) {
	int count = 1;
	double lastKnot = c->knotVec[0];
	for (int i = 1; i < c->knotNum; i++) {
		if (c->knotVec[i] != lastKnot) {
			count++;
		}
		lastKnot = c->knotVec[i];
	}
	return count;
}

void getKnotBreakdown(CURVE_STRUCT* c, int *size, int **mult, double** bps) {
	*size = getBreakPointCount(c);
	*mult = (int*)malloc(sizeof(int) * (*size));
	*bps = (double*)malloc(sizeof(double) * (*size));
	int count = 0;
	double lastKnot = c->knotVec[0];
	(*mult)[0] = 1;
	(*bps)[0] = lastKnot;
	for (int i = 1; i < c->knotNum; i++) {
		if (c->knotVec[i] == lastKnot) {
			(*mult)[count] = (*mult)[count] + 1;
		}
		else {
			count = count + 1;
			(*bps)[count] = c->knotVec[i];
			(*mult)[count] = 1;
			
		}
		lastKnot = c->knotVec[i];
	}
}

void getWindowWidthHeight(int* width, int* height) {
	HWND hwnd = cagdGetWindow();
	RECT rect;
	GetClientRect(hwnd, &rect); // client area (drawable OpenGL area)

	*width = rect.right - rect.left;
	*height = rect.bottom - rect.top;

}

static UINT createTriangleShapes(double normalized_start,double pad ,int multiplicity) {
	// creates christmas tree style arrows
	/*
	|_\
	|_\
	|_\
	*/
	int n = 2 * (multiplicity + 1);
	CAGD_POINT* tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT)* n);
	double step = pad / 40;
	tmp[0].x = normalized_start;
	tmp[0].y = pad;
	tmp[0].z = 0;
	for (int i = 1; i < n - 2; i = i+ 2) {
		
		tmp[i+1].x = normalized_start;
		tmp[i+1].y = pad- step * (i / 2 + 1);
		tmp[i+1].z = 0;

		tmp[i].x = normalized_start + step;
		tmp[i].y = pad - step * (i / 2 + 1);
		tmp[i].z = 0;
		

	}
	tmp[n - 1] = tmp[0];
	UINT triangle = cagdAddPolyline(tmp, n);
	free(tmp);
	return triangle;
}

void showKnotDisplay(int index) {
	if (index < 0 || index >= MAX_CURVES || curveArray[index] == NULL || curveArray[index]->isSpline != TRUE) {
		fprintf(stderr, "\[\033[0;31m\]Not possible. How did you even get here?\n");
		return;
	}
	hideKnotDisplay(KV.index);
	KV.index = index;

	//creating the line
	int w, h;
	getWindowWidthHeight(&w, &h);
	CAGD_POINT hlp[2];
	CAGD_POINT line_p[5];
	cagdToObject(0 + PADDING_X, PADDING_Y + PADDING_Y, hlp);
	line_p[0] = hlp[0];
	cagdToObject(w - PADDING_X, PADDING_Y + PADDING_Y, hlp);
	line_p[1] = hlp[0];
	line_p[0].z = line_p[1].z = 0;
	double line_w = line_p[1].x - line_p[0].x;
	double line_h = line_p[1].y;
	double line_start = line_p[0].x;
	line_p[4] = line_p[0];
	cagdToObject(0 + PADDING_X, 0, hlp);
	line_p[2] = line_p[1];
	line_p[2].y = hlp[0].y;
	line_p[3].y = line_p[2].y;
	line_p[3].x = line_p[0].x;
	line_p[3].z = 0;

	KV.line = cagdAddPolyline(line_p, 5);
	//cagdGetSegmentLocation(KV.line, hlp);
	

	CURVE_STRUCT * crv = curveArray[index];
	int *mult = NULL;
	int break_s;
	double *breakpoints = NULL;

	getKnotBreakdown(crv, &break_s, &mult, &breakpoints);
	KV.breakpoints_num = break_s;
	KV.multiplicity = mult;
	
	normalizeBreakPoints(break_s, breakpoints, line_w, line_start);
	KV.normalizedBreakPoints = breakpoints;

	UINT* trianglePolylineVector = (UINT*)malloc(sizeof(UINT)* break_s);
	int tri_num;

	// draw all triangles
	for (int i = 0; i < break_s; i++) {
		tri_num = (mult[i] + 1) * 2;
		//printf("\nfor normalized break point index %d the value is %lf \n", i, KV.normalizedBreakPoints[i]);
		trianglePolylineVector[i] = createTriangleShapes(KV.normalizedBreakPoints[i], line_h, mult[i]);
	}
	KV.knotTriangle = trianglePolylineVector;
	KVcolor(0xEB, 0x5E, 0x28);
}

void KVcolor(BYTE r, BYTE g, BYTE b) {
	cagdSetSegmentColor(KV.line, r, g, b);
	for (int i = 0; i < KV.breakpoints_num; i++) {
		cagdSetSegmentColor(KV.knotTriangle[i], r, g, b);
	}
}

void hideKnotDisplay(int curveIndex) {
	if (KV.index == NO_INDEX) {
		return;
	}
	// just destroys the KV
	if (KV.normalizedBreakPoints) {
		free(KV.normalizedBreakPoints);
		KV.normalizedBreakPoints = NULL;
	}
	if (KV.multiplicity) {
		free(KV.multiplicity);
		KV.multiplicity = NULL;
	}
	if (KV.knotTriangle) {
		for (int i = 0; i < KV.breakpoints_num; i++) {
			cagdFreeSegment(KV.knotTriangle[i]);
		}
		free(KV.knotTriangle);
		KV.knotTriangle = NULL;
		cagdFreeSegment(KV.line);
		KV.line = NO_CURVE;
	}
	KV.breakpoints_num = 0;
	KV.index = NO_INDEX;
}

void clearCheck(UINT id) {
	CheckMenuItem(connectMenu, id, MF_UNCHECKED);
}


int getKnotIndexFromBreakpointIndex(int j) {
	if (j == 0) {
		return KV.multiplicity[j] - 1;
	}
	if (j == KV.breakpoints_num - 1) {
		// Sum all multiplicities except the last
		int offset = 0;
		for (int i = 0; i < j; i++) {
			offset += KV.multiplicity[i];
		}
		return offset; // first index of last repeated knot
	}
	int mult = 0;
	int i = 0;
	while (i<j) {
		mult += KV.multiplicity[i];
		i++;
	}
	return mult;
}

int getBreakPointIndexFromKnotIndex(CURVE_STRUCT* c, int j) {
	int count = 0;
	double lastKnot = c->knotVec[0];

	for (int i = 1; i <= j; i++) {
		if (c->knotVec[i] != lastKnot) {
			count++;
			lastKnot = c->knotVec[i];
		}
	}

	return count;
}

double getKnotValueFromCAGDXPosition(double x, double knotMin, double knotMax) {
	// [normbreakmin, normbreakmax] - > [kmin, kmax]
	// (old - min) / (max - min) * (kmax - kmin) + kmin
	double normMin = KV.normalizedBreakPoints[0];
	double normMax = KV.normalizedBreakPoints[KV.breakpoints_num - 1];
	if (normMax == normMin) return knotMin; // prevent divide-by-zero
	double t = (x - normMin) / (normMax - normMin);
	// Optional clamp
	if (t < 0) t = 0;
	if (t > 1) t = 1;
	CAGD_POINT linewidth[5];
	cagdGetSegmentLocation(KV.line, linewidth);
	double threshold = WIDTH_SENSITIVITY * fabs(linewidth[0].x - linewidth[1].x);
	for (int i = 0; i < KV.breakpoints_num; i++) {
		double existing = KV.normalizedBreakPoints[i];
		if (fabs(x - existing) <= threshold) {
			x = existing;
			break;
		}
	}

	// Map snapped x to knotValue
	t = (x - normMin) / (normMax - normMin);
	double knotValue = t * (knotMax - knotMin) + knotMin;
	return knotValue;
	
}

void setCurveColor(int index, BYTE r, BYTE g, BYTE b) {
	curveArray[index]->s.curveColor.red = r;
	curveArray[index]->s.curveColor.green = g;
	curveArray[index]->s.curveColor.blue = b;
	cagdSetSegmentColor(curveArray[index]->curvePolyline, r, g, b);
}