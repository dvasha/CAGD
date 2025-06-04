#include "structures.h"
#include "calculations.h"
#include "cagd.h"

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
	}
	else {
		hideWeightVectorofCurve(index);
	}
}

void createKnotDisplay(int index) {
	CURVE_STRUCT*crv = curveArray[index];
	// draw knot line
	// normalize knot vector for this case
	// handle multiplicity
}

