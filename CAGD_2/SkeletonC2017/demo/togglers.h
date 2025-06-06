#ifndef TOGGLERS_H
#define TOGGLERS_H

#define PADDING_X 20
#define PADDING_Y 5
#define WIDTH_SENSITIVITY 0.08
void showHodographofCurve(int index);
void hideHodographofCurve(int index);
void drawHideAllHodographs();

void showControlPointsOfCurve(int index);
void hideControlPointsOfCurve(int index);
void showControlPolygonOfCurve(int index);
void hideControlPolygonOfCurve(int index);
void drawHideAllControlPolygons();


void hideWeightVectorofCurve(int index);
void showWeightVectorofCurve(int index);
void drawHideAllWeightVec();


void hide_show_by_disp_state(int index);

void KVcolor(BYTE r, BYTE g, BYTE b);
void hideKnotDisplay(int index);
void showKnotDisplay(int index);
void getKnotBreakdown(CURVE_STRUCT* c, int *size, int **mult, double** bps);
int getBreakPointCount(CURVE_STRUCT *c);

int getBreakPointIndexFromKnotIndex(CURVE_STRUCT* c, int j);
int getKnotIndexFromBreakpointIndex(int j);
int getKnotIndexFromBreakpointIndex(int j);
int getKnotIndexFromBreakpointIndex(int j);
double getKnotValueFromCAGDXPosition(double x, double knotMin, double knotMax);

void clearCheck(enum i);
#endif
