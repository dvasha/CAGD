
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


/*


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
						cagdRegisterCallback(CAGD_MOUSEMOVE, dragControlPoint, msg_idx);
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

*/


#pragma once
