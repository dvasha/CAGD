#ifndef CALCULATIONS_H
#define CALCULATIONS_H
#include "structures.h"
#include <cagd.h>



double m_max(double a, double b);
double m_min(double a, double b);

CAGD_POINT weightedDeCasteljau(int index, double t);

CAGD_POINT weightedDeCasteljauDerivative(int index, double t, CAGD_POINT raw);

CAGD_POINT WeightedDeBoor(int index, double t);

CAGD_POINT WeightedDeBoorDerivative(int index, double t, CAGD_POINT raw);

void translateCurveByDelta(int index, double deltaX, double deltaY);

void rotateCurveByDegree(int index, double angleRad, CAGD_POINT pivotPoint);

void ConnectC0Aux();

void ConnectG1Aux();

void ConnectC1Aux();

void clearCurveSegmentsByIndex(int index);

void removeCurveFromIndex(int index);

void removeAllCurves();

void printCurve(int index);

void createcurvePolyline(int index);

void createWeightCircles(int index);

UINT createControlPolygon(int index);

createCurveFromIndex(int index);

#endif