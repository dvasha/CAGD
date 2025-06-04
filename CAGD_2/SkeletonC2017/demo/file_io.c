// IO
#include "structures.h"

void myRead(int x, int y, PVOID userData) {
	boolean isBspline = FALSE;
	BsplineType bsplineT = BSPLINE_UNKNOWN;
	boolean curveIncomplete = FALSE;
	double* knotVector = NULL;
	CAGD_POINT *controlVector = NULL;
	UINT* weightVector = NULL;
	UINT* pointDisp = NULL;
	UINT* polygonVector = NULL;
	int order = 0;
	int numPts = 0;
	int numKnots = 0;
	int idx = -1;
	DISPLAY_STATUS my_ds = default_ds;
	CURVE_STRUCT *newCurve = NULL;


	fptr = fopen((char*)x, "r");
	if (fptr) {
		while (fgets(myBuffer, 1024, fptr)) { //reads a line
			idx = findAvailableIndex();
			if (idx < 0) {
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
						boolean isClamped = TRUE;
						for (int i = 1; i <= order - 1; i++) {
							if (knotVector[i] != knotVector[0] || knotVector[numKnots - 1 - i] != knotVector[numKnots - 1]) {
								isClamped = FALSE;
								break;
							}
						}
						if (isClamped) {
							bsplineT = BSPLINE_CLAMPED;
						}
						else {
							double delta = knotVector[1] - knotVector[0];
							boolean isUniform = TRUE;
							for (int i = 2; i <= numKnots - 1; i++) {
								if (fabs(knotVector[i] - knotVector[i - 1] - delta) > MY_ZERO) {
									isUniform = FALSE;
									break;
								}
							}
							if (isUniform) {
								bsplineT = BSPLINE_FLOATING;
							}
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
								.splineType = bsplineT,
							   .order = order,
							   .knotNum = numKnots,
							   .pointNum = numPts,
							   .knotVec = knotVector,
							   .pointVec = controlVector,
							   .weightVec = weightVector,
							   .polyVec = polygonVector,
								.pointDisp = NULL,
								.hodograph = NO_CURVE,
								.curvePolyline = NO_CURVE,
							   .index = idx,
								.s = my_ds, };
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

