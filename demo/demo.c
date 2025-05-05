#include <cagd.h>
#include <stdio.h>
#include "resource.h"
#include <expr2tree.h>

#if defined(_WIN32)
    #if _MSC_VER >= 1900
	#pragma comment(lib, "legacy_stdio_definitions.lib")
    #endif
#endif

enum {
	MY_CLICK = CAGD_USER,
	MY_POLY,
	MY_ANIM,
	MY_DRAG,
	MY_ADD,
	MY_COLOR,
	MY_REMOVE,
	MY_COORD,
	FRENET_CURVE,
	FRENET_POINTS,
	FRENET_TRIHEDRON,
	FRENET_CURVATURE,
	FRENET_TORSION,
	FRENET_EVOLUTE,
	FRENET_OFFSET,
	FRENET_SPHERE,
	FRENET_AXIS,
	FRENET_ANIMATION,
	M_PROPERTIES,
};

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

HMENU myPopup;
UINT myText;
char myBuffer[BUFSIZ], x_line[BUFSIZ], y_line[BUFSIZ], z_line[BUFSIZ], t_line[BUFSIZ];
FILE* fptr;
double domain_min, domain_max;
double stepSize = 0.1;
int numOfPoints;
UINT *id_pts, id_curve, id_curvature, id_torsion;


void myMessage(PSTR title, PSTR message, UINT type)
{
	MessageBox(cagdGetWindow(), message, title, MB_OK | MB_APPLMODAL | type);
}

void myTimer(int x, int y, PVOID userData)
{
	cagdRotate(2, 0, 1, 0);
	cagdRedraw();
}


e2t_expr_node* buildTreeFromLine(char* line) {
	e2t_expr_node* newTree = e2t_expr2tree(line);
	if (!newTree) {
		printf("Error %d\n", e2t_parsing_error);
		return NULL;
	}

	return newTree;

}


void myCreateCAGD() {
	cagdFreeAllSegments();
	cagdReset();

	CAGD_POINT* my_pts, tmp_pt;
	double x_pt, y_pt, z_pt;
	double Tx_pt, Ty_py, Tz_pt;
	double paramt = 0;
	int index = 0;
	e2t_expr_node* x_tree, *y_tree, *z_tree;

	numOfPoints = floor((domain_max - domain_min) / stepSize);

	if (id_pts) {
		free(id_pts);
	}
	id_pts = (UINT*)malloc(sizeof(UINT) * (numOfPoints));

	my_pts = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * (numOfPoints + 1));


	x_tree = buildTreeFromLine(x_line);
	y_tree = buildTreeFromLine(y_line);
	z_tree = buildTreeFromLine(z_line);

	for (index = 0; index < numOfPoints; index++) {
		e2t_setparamvalue(stepSize * index + domain_min, E2T_PARAM_T);
		x_pt = e2t_evaltree(x_tree);
		y_pt = e2t_evaltree(y_tree);
		z_pt = e2t_evaltree(z_tree);

		tmp_pt = (CAGD_POINT) { .x = x_pt, .y = y_pt, .z = z_pt };
		my_pts[index] = tmp_pt;
		id_pts[index] = cagdAddPoint(&tmp_pt);
	}
	my_pts[index] = my_pts[0];


	id_curve = cagdAddPolyline(my_pts, numOfPoints + 1);


	free(my_pts);

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

INT_PTR CALLBACK myDialogProc2(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char x_buff[BUFSIZ], y_buff[BUFSIZ], z_buff[BUFSIZ], min_buff[32], max_buff[32], ssize_buff[32];
	//char scan_x_buff[BUFSIZ], scan_y_buff[BUFSIZ], scan_z_buff[BUFSIZ], scan_min_buff[32], scan_max_buff[32], scan_ssize_buff[32];
	switch (msg) {

	case WM_INITDIALOG:
		sprintf(x_buff, "%s", x_line);
		SetDlgItemText(hwnd, IDC_EDIT_X, x_buff);
		sprintf(y_buff, "%s", y_line);
		SetDlgItemText(hwnd, IDC_EDIT_Y, y_buff);
		sprintf(z_buff, "%s", z_line);
		SetDlgItemText(hwnd, IDC_EDIT_Z, z_buff);

		sprintf(min_buff, " %lf ", domain_min);
		SetDlgItemText(hwnd, IDC_EDIT_MIN, min_buff);
		sprintf(max_buff, " %lf ", domain_max);
		SetDlgItemText(hwnd, IDC_EDIT_MAX, max_buff);
		sprintf(ssize_buff, " %lf ", stepSize);
		SetDlgItemText(hwnd, IDC_EDIT_SSIZE, ssize_buff);
		break;
	case WM_COMMAND:
		// If OK button is pressed (IDOK)
		if (LOWORD(wParam) == IDOK) {
			// Declare variables for values
			double stepSizeTmp;
			double domain_minTmp, domain_maxTmp;

			// Retrieve text input from the three edit controls (Red, Green, Blue)
			GetDlgItemText(hwnd, IDC_EDIT_X, x_buff, sizeof(x_buff)); 
			GetDlgItemText(hwnd, IDC_EDIT_Y, y_buff, sizeof(y_buff)); 
			GetDlgItemText(hwnd, IDC_EDIT_Z, z_buff, sizeof(z_buff)); 
			GetDlgItemText(hwnd, IDC_EDIT_MIN, min_buff, sizeof(min_buff));
			GetDlgItemText(hwnd, IDC_EDIT_MAX, max_buff, sizeof(max_buff));
			GetDlgItemText(hwnd, IDC_EDIT_SSIZE, ssize_buff, sizeof(ssize_buff));

			strncpy(x_line, x_buff, BUFSIZ);
			strncpy(y_line, y_buff, BUFSIZ);
			strncpy(z_line, z_buff, BUFSIZ);

			sscanf(min_buff, "%lf", &domain_min);
			sscanf(max_buff, "%lf", &domain_max);
			sscanf(ssize_buff, "%lf", &stepSize);
			EndDialog(hwnd, IDOK);
			myCreateCAGD();
			return TRUE;
		}
		// If Cancel button is pressed (IDCANCEL), close the dialog
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hwnd, IDCANCEL);
		}
		return TRUE;

	}
	return FALSE;
}

void myDragRightUp(int x, int y, PVOID userData)
{
	UINT id;
	CAGD_POINT p[2];
	int red, green, blue;
	HMENU hMenu = (HMENU)userData;
	cagdHideSegment(myText);
	for (cagdPick(x, y); id = cagdPickNext();)
		if (cagdGetSegmentType(id) == CAGD_SEGMENT_POINT)
			break;
	EnableMenuItem(hMenu, MY_ADD, id ? MF_GRAYED : MF_ENABLED);
	EnableMenuItem(hMenu, MY_COLOR, id ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, MY_REMOVE, id ? MF_ENABLED : MF_GRAYED);
	switch (cagdPostMenu(hMenu, x, y)) {
	case MY_ADD:
		cagdToObject(x, y, p);
		p->z = 0;
		cagdAddPoint(p);
		break;
	case MY_COLOR:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_COLOR),
			cagdGetWindow(),
			(DLGPROC)myDialogProc))
			if (sscanf(myBuffer, "%d %d %d", &red, &green, &blue) == 3)
				cagdSetSegmentColor(id, (BYTE)red, (BYTE)green, (BYTE)blue);
			else
				myMessage("Change color", "Bad color!", MB_ICONERROR);
		break;
	case MY_REMOVE:
		cagdFreeSegment(id);
		break;
	}
	cagdRedraw();
}
void myDragMove(int x, int y, PVOID userData)
{
	CAGD_POINT p[2];
	cagdToObject(x, y, p);
	p->z = 0;
	cagdReusePoint((UINT)userData, p);
	cagdReuseText(myText, p, " Leave me alone!");
	cagdRedraw();
}

void myDragLeftUp(int x, int y, PVOID userData)
{
	CAGD_POINT p;
	cagdGetSegmentLocation(myText, &p);
	cagdReuseText(myText, &p, " Ufff!");
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	cagdRedraw();
}

void myDragLeftDown(int x, int y, PVOID userData)
{
	UINT id;
	for(cagdPick(x, y); id = cagdPickNext();)
		if(cagdGetSegmentType(id) == CAGD_SEGMENT_POINT)
			break;
	if(id){
		CAGD_POINT p;
		BYTE red, green, blue;
		cagdGetSegmentLocation(id, &p);
		cagdReuseText(myText, &p, " Don't touch me!");
		cagdGetSegmentColor(id, &red, &green, &blue);
		cagdSetSegmentColor(myText, red, green, blue);
		cagdShowSegment(myText);
		cagdRegisterCallback(CAGD_MOUSEMOVE, myDragMove, (PVOID)id);
		cagdRegisterCallback(CAGD_LBUTTONUP, myDragLeftUp, NULL);
	} else
		myMessage("Ha-ha!", "You missed...", MB_ICONERROR);
	cagdRedraw();
}

void myClickLeftDown(int x, int y, PVOID userData)
{
	CAGD_POINT p[2];
	cagdToObject(x, y, p);
	cagdAddPolyline(p, sizeof(p) / sizeof(CAGD_POINT));
}

void myPolyLeftDown(int x, int y, PVOID userData)
{
	CAGD_POINT p;
	UINT id;
	int v;
	for(cagdPick(x, y); id = cagdPickNext();)
		if(cagdGetSegmentType(id) == CAGD_SEGMENT_POLYLINE)
			break;
	if(id){
		if(v = cagdGetNearestVertex(id, x, y)){
			cagdGetVertex(id, --v, &p);
			sprintf(myBuffer, " near #%d", v);
			cagdReuseText(myText, &p, myBuffer);
			cagdShowSegment(myText);
		}
	} else 
		myMessage("Ha-ha!", "You missed...", MB_ICONERROR);
	cagdRedraw();
}

void myFrenet(int id, int unUsed, PVOID userData) {

	HMENU fMenu = (HMENU)userData;
	UINT state = GetMenuState(fMenu, id, MF_BYCOMMAND);
	UINT newState = state;
	if (id != M_PROPERTIES) {
		 newState = (state & MF_CHECKED) ? MF_UNCHECKED : MF_CHECKED;
	}
	int red, green, blue;

	CheckMenuItem(fMenu, id, MF_BYCOMMAND | newState);

	switch (id){
	case FRENET_CURVE:
		if (newState == MF_CHECKED) {
			if (!cagdShowSegment(id_curve)) {
				printf("ERROR: can't show curve!\n");
			}
			
		}
		else {
			if (!cagdHideSegment(id_curve)) {
				printf("ERROR: can't hide curve!\n");
			}
			
		}
		break;
	case FRENET_POINTS:
		if(id_pts){
			if (newState == MF_CHECKED) {

				for (int pointIndex = 0; pointIndex < numOfPoints; pointIndex++) {
					if (!cagdShowSegment(id_pts[pointIndex])) {
						printf("ERROR: can't show point!\n");
					}
				}
			}
			else {
				for (int pointIndex = 0; pointIndex < numOfPoints; pointIndex++) {
					if (!cagdHideSegment(id_pts[pointIndex])) {
						printf("ERROR: can't hide point!\n");
					}
				}

			}
		}
		
		break;
	case FRENET_TRIHEDRON:
		break;
	case FRENET_CURVATURE:
		break;
	case FRENET_TORSION:
		break;
	case FRENET_EVOLUTE:
		break;
	case FRENET_OFFSET:
		break;
	case FRENET_SPHERE:
		break;
	case FRENET_AXIS:
		break;
	case FRENET_ANIMATION:
		break;
	}

	if (id == M_PROPERTIES) {
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_EDIT_PARAMS),
			cagdGetWindow(),
			(DLGPROC)myDialogProc2))
			printf("yeah\n");
			/*if (sscanf(bufferR, "%d", &red) == 1 && sscanf(bufferG, "%d", &green) == 1 && sscanf(bufferB, "%d", &blue) == 1) {
				cagdSetSegmentColor(id, (BYTE)red, (BYTE)green, (BYTE)blue);
				printf("Colors chosen (rgb) : (%d, %d, %d)", red, green, blue);
			}
			else
				myMessage("Change color", "Bad color!", MB_ICONERROR);*/
		
			
	}

	cagdRedraw();

}

void tmp() {
	e2t_expr_node* x_tree, *y_tree, *z_tree;
	e2t_expr_node* Tx_tree, *Ty_tree, *Tz_tree;
	e2t_expr_node* Nx_tree, *Ny_tree, *Nz_tree;
	double step, step_granularity = 100, x_point, y_point, z_point, Tx_point, Ty_point, Tz_point, T_normalize;
	CAGD_POINT m_pts[100], T_pts[2];
	// get trees
	x_tree = e2t_expr2tree(x_line);
	if (!x_tree) {
		printf("Error %d\n", e2t_parsing_error);
	}
	y_tree = e2t_expr2tree(y_line);
	if (!y_tree) {
		printf("Error %d\n", e2t_parsing_error);
	}
	z_tree = e2t_expr2tree(z_line);
	if (!z_tree) {
		printf("Error %d\n", e2t_parsing_error);
	}
	/*Tx_tree = e2t_derivtree(x_tree, E2T_PARAM_T);
	Ty_tree = e2t_derivtree(y_tree, E2T_PARAM_T);
	Tz_tree = e2t_derivtree(z_tree, E2T_PARAM_T);

	Nx_tree = e2t_derivtree(Tx_tree, E2T_PARAM_T);
	Ny_tree = e2t_derivtree(Ty_tree, E2T_PARAM_T);
	Nz_tree = e2t_derivtree(Tz_tree, E2T_PARAM_T);*/

	step = (domain_max - domain_min) / 100;
	

	for (double i = domain_min; i <= domain_max; i = i + step) {
		e2t_setparamvalue(i, E2T_PARAM_T);


		x_point = e2t_evaltree(x_tree);
		y_point = e2t_evaltree(y_tree);
		z_point = e2t_evaltree(z_tree);

		/*Tx_point = e2t_evaltree(Tx_tree);
		Ty_point = e2t_evaltree(Ty_tree);
		Tz_point = e2t_evaltree(Tz_tree);*/



		T_normalize = sqrt(Tx_point*Tx_point + Ty_point * Ty_point + Tz_point * Tz_point);

		T_pts[0] = (CAGD_POINT) { .x = x_point, .y = y_point, .z = z_point };
		T_pts[1] = (CAGD_POINT) { .x = Tx_point / T_normalize, .y = Ty_point / T_normalize, .z = Tz_point / T_normalize };
		cagdAddPolyline(T_pts, sizeof(T_pts) / sizeof(CAGD_POINT));


		//printf("(%lf, %lf, %lf)\n", x_point, y_point, z_point);
		m_pts[0] = (CAGD_POINT) { .x = x_point, .y = y_point, .z = z_point };

	}

	cagdAddPolyline(m_pts, sizeof(m_pts) / sizeof(CAGD_POINT));
}


void my_display() {
	
	cagdRedraw();
}


// loads a file and reads the functions and domain to x_line, y_line, z_line, t_line and min/ max.
void myRead(int x, int y, PVOID userData) {

	int ln = 0;
	fptr = fopen((char*)x, "r");
	if (fptr) {
		while (fgets(myBuffer, 1024, fptr)) {
			if (myBuffer[0] == '#') {
				continue;
			}
			if (myBuffer[0] == '\n') {
				continue;
			}
			else {
				switch (ln) {
				case 0:
					strcpy(x_line, myBuffer);
					ln++;
					break;
				case 1:
					strcpy(y_line, myBuffer);
					ln++;
					break;
				case 2:
					strcpy(z_line, myBuffer);
					ln++;
					break;
				case 3:
					strcpy(t_line, myBuffer);
					ln++;
					break;
				default:
					printf("huh??");
				}
			}
		}
		fclose(fptr);
	}
	
	x_line[strlen(x_line) - 1] = 0;
	y_line[strlen(y_line) - 1] = 0;
	z_line[strlen(z_line) - 1] = 0;
	sscanf(t_line, "%lf %lf\n", &domain_min, &domain_max);

	myCreateCAGD();
	my_display();
}

void mySave(int x, int y, PVOID userData) {
	int ln = 0;
	fptr = fopen((char*)x, "w");
	if (fptr) {
		fprintf(fptr, "%s \n %s \n %s \n\n %lf %lf", x_line, y_line, z_line, domain_min, domain_max);
		fclose(fptr);
	}

}


void myCommand(int id, int unUsed, PVOID userData)
{
	int i;
	CAGD_POINT p[] = {{1, 1, 1}, {1, -1, 1}, {-1, -1, 1}, {-1, 1, 1}, {1, 1, 1}};
	static state = MY_CLICK;
	HMENU hMenu = (HMENU)userData;
	CheckMenuItem(hMenu, state, MF_UNCHECKED);
	CheckMenuItem(hMenu, state = id, MF_CHECKED);
	cagdFreeAllSegments();
	cagdReset();
	cagdSetView(CAGD_ORTHO);
	cagdSetDepthCue(FALSE);
	cagdSetColor(255, 255, 255);
	cagdRegisterCallback(CAGD_TIMER, NULL, NULL);
	cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
	cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	cagdRegisterCallback(CAGD_RBUTTONUP, NULL, NULL);
	cagdRedraw();
	switch(id){
		case MY_ANIM:
			cagdSetView(CAGD_PERSP);
			cagdSetDepthCue(TRUE);
			cagdSetColor(0, 255, 255);
			cagdAddPolyline(p, sizeof(p) / sizeof(CAGD_POINT));
			p[0].z = p[1].z = p[2].z = p[3].z = p[4].z = -1;
			cagdAddPolyline(p, sizeof(p) / sizeof(CAGD_POINT));
			p[1].z = p[2].z = p[1].y = p[2].y = 1;
			cagdAddPolyline(p, sizeof(p) / sizeof(CAGD_POINT));
			p[0].y = p[1].y = p[2].y = p[3].y = p[4].y = -1;
			cagdAddPolyline(p, sizeof(p) / sizeof(CAGD_POINT));
			for(i = 0; i < 100; i++){
				p->x = (GLdouble)rand() / RAND_MAX * 2 - 1;
				p->y = (GLdouble)rand() / RAND_MAX * 2 - 1;
				p->z = (GLdouble)rand() / RAND_MAX * 2 - 1;
				cagdAddPoint(p);
			}
			SetWindowText(cagdGetWindow(), animText[0]);
			cagdSetHelpText(animText[1]);
			cagdShowHelp();
			cagdRegisterCallback(CAGD_TIMER, myTimer, NULL);
			break;
		case MY_DRAG:
			SetWindowText(cagdGetWindow(), dragText[0]);
			cagdSetHelpText(dragText[1]);
			cagdShowHelp();
			cagdHideSegment(myText = cagdAddText(p, ""));
			cagdRegisterCallback(CAGD_RBUTTONUP, myDragRightUp, (PVOID)myPopup);
			cagdRegisterCallback(CAGD_LBUTTONDOWN, myDragLeftDown, NULL);
			break;
		case MY_CLICK:
			cagdSetView(CAGD_PERSP);
			cagdSetDepthCue(TRUE);
			SetWindowText(cagdGetWindow(), clickText[0]);
			cagdSetHelpText(clickText[1]);
			cagdShowHelp();
			cagdRegisterCallback(CAGD_LBUTTONDOWN, myClickLeftDown, NULL);
			break;
		case MY_POLY:
			cagdScale(0.5, 0.5, 0.5);
			cagdRotate(45, 0, 0, 1);
			cagdAddPolyline(p, sizeof(p) / sizeof(CAGD_POINT) - 1);
			cagdHideSegment(myText = cagdAddText(p, ""));
			SetWindowText(cagdGetWindow(), polyText[0]);
			cagdSetHelpText(polyText[1]);
			cagdShowHelp();
			cagdRegisterCallback(CAGD_LBUTTONDOWN, myPolyLeftDown, NULL);
			break;
		
	}
	cagdRedraw();
}

int main(int argc, char *argv[])
{
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
	cagdRegisterCallback(CAGD_MENU, myCommand, (PVOID)hMenu);



	// New


	cagdRegisterCallback(CAGD_LOADFILE, myRead, NULL);
	cagdRegisterCallback(CAGD_SAVEFILE, mySave, NULL);

	//cagdAppendMenu(myPopup, "New");


	// Frenet menu
	HMENU fMenu = CreatePopupMenu();
	AppendMenu(fMenu, MF_STRING | MF_CHECKED, FRENET_CURVE, "Curve");
	AppendMenu(fMenu, MF_STRING | MF_CHECKED, FRENET_POINTS, "Points");
	AppendMenu(fMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(fMenu, MF_STRING , FRENET_TRIHEDRON, "Trihedron");
	AppendMenu(fMenu, MF_STRING , FRENET_CURVATURE, "Curvature (Osculating Circle)");
	AppendMenu(fMenu, MF_STRING , FRENET_TORSION, "Torsion");
	AppendMenu(fMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(fMenu, MF_STRING , FRENET_EVOLUTE, "Evolute");
	AppendMenu(fMenu, MF_STRING , FRENET_OFFSET, "Offset");
	AppendMenu(fMenu, MF_STRING , FRENET_SPHERE, "Sphere");

	AppendMenu(fMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(fMenu, MF_STRING , FRENET_AXIS, "Axis");
	AppendMenu(fMenu, MF_STRING , FRENET_ANIMATION, "Animation");
	AppendMenu(fMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(fMenu, MF_STRING, M_PROPERTIES, "Properties...");
	cagdRegisterCallback(CAGD_MENU, myFrenet, (PVOID)fMenu);

	cagdAppendMenu(fMenu, "Frenet");


	// end
	//cagdShowHelp();
	cagdMainLoop();
	return 0;
}
