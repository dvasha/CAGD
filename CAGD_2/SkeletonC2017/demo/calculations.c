
#include "cagd.h"
#include "calculations.h"

double m_max(double a, double b) {
	return a > b ? a : b;
}
double m_min(double a, double b) {
	return a < b ? a : b;
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
		for (int i = 0; i < n - k; i++) {
			tempPoints[i].x = tempPoints[i].x * (1 - t) + tempPoints[i + 1].x * t;
			tempPoints[i].y = tempPoints[i].y * (1 - t) + tempPoints[i + 1].y * t;
			tempPoints[i].z = tempPoints[i].z * (1 - t) + tempPoints[i + 1].z * t;
		}
	}
	
	result.x = tempPoints[0].x;
	result.y = tempPoints[0].y;
	result.z = tempPoints[0].z;

	free(tempPoints);
	return result;
}

CAGD_POINT weightedDeCasteljauDerivative(int index, double t, CAGD_POINT raw) {
	int n = curveArray[index]->pointNum;
	int ord = curveArray[index]->order;
	CAGD_POINT result;
	CAGD_POINT *derivativePoints = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * (n - 1));
	for (int i = 0; i < n - 1; i++) {
		derivativePoints[i].x = ord * (curveArray[index]->pointVec[i + 1].x - curveArray[index]->pointVec[i].x);
		derivativePoints[i].y = ord * (curveArray[index]->pointVec[i + 1].y - curveArray[index]->pointVec[i].y);
		derivativePoints[i].z = ord * (curveArray[index]->pointVec[i + 1].z - curveArray[index]->pointVec[i].z);

	}
	for (int k = 1; k <= ord - 1; k++) {
		for (int i = 0; i < n - 1 - k; i++) {
			derivativePoints[i].x = derivativePoints[i].x * (1 - t) + derivativePoints[i + 1].x * t;
			derivativePoints[i].y = derivativePoints[i].y * (1 - t) + derivativePoints[i + 1].y * t;
			derivativePoints[i].z = derivativePoints[i].z * (1 - t) + derivativePoints[i + 1].z * t;
		}
	}
	// rational (weighted)


	double w = raw.z;
	double w_prime = derivativePoints[0].z;
	result.x = (derivativePoints[0].x * w - raw.x * w_prime) / (w*w);
	result.y = (derivativePoints[0].y * w - raw.y * w_prime) / (w*w);
	result.z = 0;

	free(derivativePoints);
	return result;
}

CAGD_POINT WeightedDeBoor(int index, double t) {
	int n = curveArray[index]->pointNum - 1;
	int k = curveArray[index]->order - 1;
	int m = curveArray[index]->knotNum - 1;
	double* knotVector = curveArray[index]->knotVec;
	CAGD_POINT* pointVector = curveArray[index]->pointVec;
	CAGD_POINT* B_i_p;
	CAGD_POINT result;

	double domain_max;

	if (curveArray[index]->splineType == BSPLINE_CLAMPED) {
		domain_max = knotVector[n + 1];
	}
	else {
		domain_max = knotVector[m - k];
	}

	if (t >= domain_max) {
		t = domain_max - MY_ZERO;
	}

	int di;
	for (di = k; di <= n; di++) {
		if (t >= knotVector[di] && t < knotVector[di + 1]) {
			break;
		}
	}
	if (di > n) di = n;

	B_i_p = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * (k + 1));
	for (int i = 0; i <= k; i++) {
		B_i_p[i] = pointVector[i + di - k];
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
			B_i_p[i].x = (1.0 - alpha) * B_i_p[i - 1].x + alpha * B_i_p[i].x;
			B_i_p[i].y = (1.0 - alpha) * B_i_p[i - 1].y + alpha * B_i_p[i].y;
			B_i_p[i].z = (1.0 - alpha) * B_i_p[i - 1].z + alpha * B_i_p[i].z;
		}
	}

	result = B_i_p[k];

	if (result.z) {
		result.x = result.x / result.z;
		result.y = result.y / result.z;
	}

	free(B_i_p);
	B_i_p = NULL;
	return result;
}

/*
CAGD_POINT WeightedDeBoorDerivative(int index, double t, CAGD_POINT raw) {
	//Based on Theorem 6.18 and NURBS wikipage.
	int n = curveArray[index]->pointNum - 1;
	int k = curveArray[index]->order - 2; // new order
	int m = curveArray[index]->knotNum - 1;
	double* knotVector = curveArray[index]->knotVec;
	CAGD_POINT* pointVector = curveArray[index]->pointVec;
	CAGD_POINT* Q1_pts;
	CAGD_POINT result;
	double domain_max;

	if (curveArray[index]->splineType == BSPLINE_CLAMPED) {
		domain_max = knotVector[n + 1];
	}
	else {
		domain_max = knotVector[m - k];
	}
	if (t >= domain_max) {
		t = domain_max - MY_ZERO;
	}
	int di; // domain index
	for (di = k; di <= n; di++) {
		if (t >= knotVector[di] && t < knotVector[di + 1]) {
			break;
		}
	}
	if (di > n) di = n;

	double c;
	Q1_pts = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * (k));
	for (int i = 0; i < k; i++) {
		double denom = (knotVector[i+di+1] - knotVector[i + di - k + 1]);
		if (denom != 0) {
			c = k / denom;
		}
		else {
			c = k;
		}

		Q1_pts[i].x = c * (pointVector[i + di - k + 1].x - pointVector[i + di - k].x);
		Q1_pts[i].y = c * (pointVector[i + di - k + 1].y - pointVector[i + di - k].y);
		Q1_pts[i].z = c * (pointVector[i + di - k + 1].z - pointVector[i + di - k].z);

	}

	for (int p = 1; p < k; p++) {
		for (int i = k - 1; i >= p; i--) {
			double denom = (knotVector[i+di +1 -p] - knotVector[i + di - k]);
			double alpha;
			if (denom != 0) {
				alpha = (t - knotVector[i + di - k]) / denom;
			}
			else {
				alpha = 0;
			}
			Q1_pts[i].x = (1.0 - alpha) * Q1_pts[i - 1].x + alpha * Q1_pts[i].x;
			Q1_pts[i].y = (1.0 - alpha) * Q1_pts[i - 1].y + alpha * Q1_pts[i].y;
			Q1_pts[i].z = (1.0 - alpha) * Q1_pts[i - 1].z + alpha * Q1_pts[i].z;
		}
		//printf("di = %d, Q1_pts[%d] = (%lf, %lf, %lf), t = %lf\n", di, p,Q1_pts[p].x, Q1_pts[p].y, Q1_pts[p].z, t);
	}


	double w = raw.z;
	double w_prime = Q1_pts[k - 1].z;

	result.x = (Q1_pts[k - 1].x * w - raw.x * w_prime) / (w * w);
	result.y = (Q1_pts[k - 1].y * w - raw.y * w_prime) / (w * w);
	result.z = 0;

	free(Q1_pts);
	Q1_pts = NULL;
	//printf("result for t = %lf is (%lf, %lf) and the raw.z = %lf, w_prime = %lf\n", t, result.x, result.y, w, w_prime);
	return result;
}
*/

CAGD_POINT WeightedDeBoorDerivative(int index, double t, CAGD_POINT raw) {
	//Based on Theorem 6.18 and NURBS wikipage. 
	int pointNum = curveArray[index]->pointNum - 1;
	int degree = curveArray[index]->order - 1; // new order
	int knotNum = curveArray[index]->knotNum - 1;
	double* knotVector = curveArray[index]->knotVec;
	CAGD_POINT* pointVector = curveArray[index]->pointVec;
	CAGD_POINT* Q1_pts;
	CAGD_POINT result;
	double domain_max;
	double c;
	int di; // domain index

	if (curveArray[index]->splineType == BSPLINE_CLAMPED) {
		domain_max = knotVector[pointNum + 1];
	}
	else {
		domain_max = knotVector[knotNum - degree];
	}
	if (t >= domain_max) {
		t = domain_max - MY_ZERO;
	}
	for (di = degree; di <= pointNum; di++) {
		if (t >= knotVector[di] && t < knotVector[di + 1]) {
			break;
		}
	}
	if (di > knotNum) di = knotNum;


	Q1_pts = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * (degree));
	for (int i = 0; i < degree; i++) {
		double denom = (knotVector[i + di + 1] - knotVector[i + di - degree + 1]);
		if (denom != 0) {
			c = degree / denom;
		}
		else {
			c = degree;
		}
		Q1_pts[i].x = c * (pointVector[i + di - degree + 1].x - pointVector[i + di - degree].x);
		Q1_pts[i].y = c * (pointVector[i + di - degree + 1].y - pointVector[i + di - degree].y);
		Q1_pts[i].z = c * (pointVector[i + di - degree + 1].z - pointVector[i + di - degree].z);
	}

	for (int r = 1; r < degree + 1; r++) {
		for (int j = degree - 1; j > r - 1; j--) {
			//if (!(r < degree && j < degree)) { printf("\n\n r= %d, j =%d, degree= %d AHHHHHHHHH\n\n\n", r, j, degree); continue; }
			//printf("r= %d, j =%d, degree= %d very good\n\n\n", r, j, degree);
			double denom = (knotVector[j + 1 + di - r] - knotVector[j + di - degree + 1]);
			double alpha;
			if (denom != 0) {
				alpha = (t - knotVector[j + di - degree + 1]) / denom;
			}
			else {
				alpha = 0;
			}
			Q1_pts[j].x = (1.0 - alpha) * Q1_pts[j - 1].x + alpha * Q1_pts[j].x;
			Q1_pts[j].y = (1.0 - alpha) * Q1_pts[j - 1].y + alpha * Q1_pts[j].y;
			Q1_pts[j].z = (1.0 - alpha) * Q1_pts[j - 1].z + alpha * Q1_pts[j].z;
		}
	}


	double w = raw.z;


	result.x = (Q1_pts[degree - 1].x - raw.x / w * Q1_pts[degree - 1].z) / (w);
	result.y = (Q1_pts[degree - 1].y - raw.y / w * Q1_pts[degree - 1].z) / (w);
	result.z = 0;

	free(Q1_pts);
	Q1_pts = NULL;
	return result;
}

void translateCurveByDelta(int index, double deltaX, double deltaY) {
	// recreate the curve entirely by moving the pointVec by delta then redrawing the line
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		curveArray[index]->pointVec[i].x += deltaX;
		curveArray[index]->pointVec[i].y += deltaY;
	}
	createCurveFromIndex(index);

}

void rotateCurveByDegree(int index, double angleRad, CAGD_POINT pivotPoint) {
	if (curveArray[index]->isSpline) {

		for (int i = 0; i < curveArray[index]->pointNum; i++) {
			double dx = curveArray[index]->pointVec[i].x - pivotPoint.x; //translate to origin
			double dy = curveArray[index]->pointVec[i].y - pivotPoint.y;

			double rotateX = dx * cos(angleRad) - dy * sin(angleRad); // rotate with matrix [[cos t, -sin t] [sin t, cost]]
			double rotateY = dx * sin(angleRad) + dy * cos(angleRad);

			curveArray[index]->pointVec[i].x = rotateX + pivotPoint.x;
			curveArray[index]->pointVec[i].y = rotateY + pivotPoint.y;
		}
	}
	else {
		CAGD_POINT pivotPoint = curveArray[index]->pointVec[0];
		for (int i = 0; i < curveArray[index]->pointNum; i++) {
			double dx = curveArray[index]->pointVec[i].x - pivotPoint.x; //translate to origin
			double dy = curveArray[index]->pointVec[i].y - pivotPoint.y;

			double rotateX = dx * cos(angleRad) - dy * sin(angleRad); // rotate with matrix [[cos t, -sin t] [sin t, cost]]
			double rotateY = dx * sin(angleRad) + dy * cos(angleRad);

			curveArray[index]->pointVec[i].x = rotateX + pivotPoint.x;
			curveArray[index]->pointVec[i].y = rotateY + pivotPoint.y;
		}
	}
	createCurveFromIndex(index);
}

void ConnectC0Aux() {
	UINT numberOfPointsInFirstCurve = cagdGetSegmentLength(curveArray[indicesToConnect[0]]->curvePolyline);
	UINT numberOfPointsInSecondCurve = cagdGetSegmentLength(curveArray[indicesToConnect[1]]->curvePolyline);
	CAGD_POINT* firstCurveVec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * numberOfPointsInFirstCurve);
	CAGD_POINT* secondCurveVec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * numberOfPointsInSecondCurve);

	cagdGetSegmentLocation(curveArray[indicesToConnect[0]]->curvePolyline, firstCurveVec);
	cagdGetSegmentLocation(curveArray[indicesToConnect[1]]->curvePolyline, secondCurveVec);

	CAGD_POINT connectionPoint = firstCurveVec[numberOfPointsInFirstCurve - 1];

	double deltaX = -secondCurveVec[0].x + connectionPoint.x;
	double deltaY = -secondCurveVec[0].y + connectionPoint.y;

	translateCurveByDelta(indicesToConnect[1], deltaX, deltaY);

	free(firstCurveVec);
	free(secondCurveVec);
}

void ConnectG1Aux() {
	CAGD_POINT firstTangent;
	CAGD_POINT secondTangent;
	CAGD_POINT *tmp;
	int n;
	n = cagdGetSegmentLength(curveArray[indicesToConnect[0]]->hodograph);
	tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) *n);
	cagdGetSegmentLocation(curveArray[indicesToConnect[0]]->hodograph, tmp);
	firstTangent = tmp[n - 1];
	free(tmp);
	n = cagdGetSegmentLength(curveArray[indicesToConnect[1]]->hodograph);
	tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) *n);
	cagdGetSegmentLocation(curveArray[indicesToConnect[1]]->hodograph, tmp);
	secondTangent = tmp[0];
	free(tmp);
	double firstDegree = atan2(firstTangent.y, firstTangent.x);
	double secondDegree = atan2(secondTangent.y, secondTangent.x);
	double rotationAngleRad = secondDegree - firstDegree;
	n = cagdGetSegmentLength(curveArray[indicesToConnect[0]]->curvePolyline);
	tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) *n);
	cagdGetSegmentLocation(curveArray[indicesToConnect[0]]->curvePolyline, tmp);
	CAGD_POINT pivot = tmp[n - 1];
	free(tmp);
	rotateCurveByDegree(indicesToConnect[1], -rotationAngleRad, pivot);
}

void ConnectC1Aux() {
	CAGD_POINT firstTangent;
	CAGD_POINT secondTangent;
	CAGD_POINT *tmp;
	int n;
	n = cagdGetSegmentLength(curveArray[indicesToConnect[0]]->hodograph);
	tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) *n);
	cagdGetSegmentLocation(curveArray[indicesToConnect[0]]->hodograph, tmp);
	firstTangent = tmp[n - 1];
	free(tmp);
	n = cagdGetSegmentLength(curveArray[indicesToConnect[1]]->hodograph);
	tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) *n);
	cagdGetSegmentLocation(curveArray[indicesToConnect[1]]->hodograph, tmp);
	secondTangent = tmp[0];
	free(tmp);

	double firstMagnitude = sqrt(firstTangent.x * firstTangent.x + firstTangent.y * firstTangent.y);
	double secondMagnitude = sqrt(secondTangent.x * secondTangent.x + secondTangent.y * secondTangent.y);

	curveArray[indicesToConnect[1]]->pointVec[0].x = curveArray[indicesToConnect[1]]->pointVec[0].x * (1 + firstMagnitude / secondMagnitude);
	curveArray[indicesToConnect[1]]->pointVec[0].y = curveArray[indicesToConnect[1]]->pointVec[0].y * (1 + firstMagnitude / secondMagnitude);



	double firstDegree = atan2(firstTangent.y, firstTangent.x);
	double secondDegree = atan2(secondTangent.y, secondTangent.x);
	double rotationAngleRad = secondDegree - firstDegree;


	n = cagdGetSegmentLength(curveArray[indicesToConnect[0]]->curvePolyline);
	tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) *n);
	cagdGetSegmentLocation(curveArray[indicesToConnect[0]]->curvePolyline, tmp);
	CAGD_POINT pivot = tmp[n - 1];
	free(tmp);
	rotateCurveByDegree(indicesToConnect[1], -rotationAngleRad, pivot);

	UINT numberOfPointsInFirstCurve = cagdGetSegmentLength(curveArray[indicesToConnect[0]]->curvePolyline);
	UINT numberOfPointsInSecondCurve = cagdGetSegmentLength(curveArray[indicesToConnect[1]]->curvePolyline);
	CAGD_POINT* firstCurveVec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * numberOfPointsInFirstCurve);
	CAGD_POINT* secondCurveVec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * numberOfPointsInSecondCurve);

	cagdGetSegmentLocation(curveArray[indicesToConnect[0]]->curvePolyline, firstCurveVec);
	cagdGetSegmentLocation(curveArray[indicesToConnect[1]]->curvePolyline, secondCurveVec);

	CAGD_POINT connectionPoint = firstCurveVec[numberOfPointsInFirstCurve - 1];

	double deltaX = -secondCurveVec[0].x + connectionPoint.x;
	double deltaY = -secondCurveVec[0].y + connectionPoint.y;

	translateCurveByDelta(indicesToConnect[1], deltaX, deltaY);

	free(firstCurveVec);
	free(secondCurveVec);

}



void clearCurveSegmentsByIndex(int index) {
	if (curveArray[index] != NULL) {
		cagdFreeSegment(curveArray[index]->curvePolyline);
		cagdFreeSegment(curveArray[index]->polyVec);
		for (int i = 0; i < curveArray[index]->pointNum; i++) {
			cagdFreeSegment(curveArray[index]->weightVec[i]);
			if (curveArray[index]->pointDisp)
				cagdFreeSegment(curveArray[index]->pointDisp[i]);
		}
		if (curveArray[index]->hodograph != NO_CURVE)
			cagdFreeSegment(curveArray[index]->hodograph);
	}
}

void removeCurveFromIndex(int index) {
	if (curveArray[index] != NULL) {
		clearCurveSegmentsByIndex(index);
		if (curveArray[index]->isSpline) {
			free(curveArray[index]->knotVec);
			curveArray[index]->knotVec = NULL;
		}
		free(curveArray[index]->pointVec);
		curveArray[index]->pointVec = NULL;
		free(curveArray[index]->weightVec);
		curveArray[index]->weightVec = NULL;
	}
	free(curveArray[index]);
	curveArray[index] = NULL;
}

void removeAllCurves() {
	for (int i = 0; i < MAX_CURVES; i++) {
		removeCurveFromIndex(i);
	}
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

void createcurvePolyline(int index) {
	if (curveArray[index] == NULL) {
		printf("unexpected error");
		return;
	}
	printCurve(index);
	UINT polyID;

	if (curveArray[index]->pointNum == 1) {
		return;
	}
	//BSPLINE check order!!!!
	if (curveArray[index]->isSpline) {
		// bspline with n+1 control points, m+1 knots and degree p
		if (curveArray[index]->pointNum < curveArray[index]->order) {
			printf("Cannot display B-Spline curve. Please add %d more points to satisfy #control points >= degree. The degree is %d. You may adjust this in Curves->Properties.\n", defaultDegree - curveArray[index]->pointNum, defaultDegree);
			curveArray[index]->curvePolyline = NO_CURVE;
			return;
		}
		int degree = curveArray[index]->order - 1;
		double domain_min = curveArray[index]->knotVec[degree];
		double domain_max = curveArray[index]->knotVec[curveArray[index]->knotNum - degree - 1];
		int totalsteps = ((domain_max - domain_min) / stepSize) + 1;
		CAGD_POINT* vec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * (totalsteps));
		CAGD_POINT* hodovec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * (totalsteps));
		double step = domain_min;
		for (int i = 0; i < totalsteps; i++) {
			vec[i] = WeightedDeBoor(index, step);
			hodovec[i] = WeightedDeBoorDerivative(index, step, vec[i]);
			vec[i].z = 0;
			step = step + stepSize;

		}
		curveArray[index]->curvePolyline = cagdAddPolyline(vec, totalsteps);

		curveArray[index]->hodograph = cagdAddPolyline(hodovec, totalsteps);


		free(vec);
		free(hodovec);
	}
	//BEZIER
	else {
		int totalsteps = (1 / stepSize) + 1;
		CAGD_POINT* vec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * totalsteps);
		CAGD_POINT* hodovec = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * totalsteps);
		double step = 0;
		for (int i = 0; i < totalsteps; i++) {
			CAGD_POINT raw = weightedDeCasteljau(index, step);
			hodovec[i] = weightedDeCasteljauDerivative(index, step, raw);
			if (raw.z) {
				vec[i].x = raw.x / raw.z;
				vec[i].y = raw.y / raw.z;
			}
			else {
				vec[i].x = 0;
				vec[i].y = 0;
			}
			vec[i].z = 0;
			step = step + stepSize;
			if (step > 1.0) {
				step = 1.0;
			}
		}
		curveArray[index]->curvePolyline = cagdAddPolyline(vec, totalsteps);
		curveArray[index]->hodograph = cagdAddPolyline(hodovec, totalsteps);
		//printf("just created the polyline with id %d\n", polyID);
		free(hodovec);
		free(vec);
	}
	printCurve(index);
}

void createWeightCircles(int index) {
	CAGD_POINT circlePoints[CIRCLE_PTS + 1];
	double x, y, R;
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		double theta = 0;
		double step = 2 * PI / CIRCLE_PTS;
		R = curveArray[index]->pointVec[i].z * CIRCLE_SCALE;
		for (int j = 0; j < CIRCLE_PTS; j++) {
			x = R * cos(theta + step * j);
			y = R * sin(theta + step * j);
			circlePoints[j].x = (curveArray[index]->pointVec[i].x/ curveArray[index]->pointVec[i].z) + x;
			circlePoints[j].y = (curveArray[index]->pointVec[i].y/ curveArray[index]->pointVec[i].z) + y;
			circlePoints[j].z = 0;
		}
		circlePoints[CIRCLE_PTS] = circlePoints[0];
		curveArray[index]->weightVec[i] = cagdAddPolyline(circlePoints, CIRCLE_PTS + 1);
	}
}

UINT createControlPolygon(int index) {
	CAGD_POINT * tmp = (CAGD_POINT*)malloc(sizeof(CAGD_POINT) * curveArray[index]->pointNum);
	UINT * ct = (UINT*)malloc(sizeof(UINT) * curveArray[index]->pointNum);
	cagdSetColor(255, 0, 0);
	for (int i = 0; i < curveArray[index]->pointNum; i++) {
		tmp[i].x = curveArray[index]->pointVec[i].x / curveArray[index]->pointVec[i].z;
		tmp[i].y = curveArray[index]->pointVec[i].y / curveArray[index]->pointVec[i].z;
		tmp[i].z = 0;
		ct[i] = cagdAddPoint(&(tmp[i]));
	}
	cagdSetColor(0, 0, 0);
	UINT res = cagdAddPolyline(tmp, curveArray[index]->pointNum);
	free(tmp);

	curveArray[index]->polyVec = res;
	if (curveArray[index]->pointDisp) {
		free(curveArray[index]->pointDisp);
	}
	curveArray[index]->pointDisp = ct;
	return res;
}

createCurveFromIndex(int index) {
	clearCurveSegmentsByIndex(index);
	createcurvePolyline(index);
	createWeightCircles(index);
	createControlPolygon(index);
	hide_show_by_disp_state(index);
	cagdRedraw();
}

