#include <cagd.h>
#include <stdio.h>
#include "resource.h"
#include <expr2tree.h>


#if defined(_WIN32)
    #if _MSC_VER >= 1900
	#pragma comment(lib, "legacy_stdio_definitions.lib")
    #endif
#endif

#define SPHERE_POINT_NUM 40

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



#define SHOW_FRENET_CURVE (1 << (FRENET_CURVE - FRENET_CURVE))
#define SHOW_FRENET_POINTS (1 << (FRENET_POINTS- FRENET_CURVE))
#define SHOW_FRENET_TRIHEDRON (1 << (FRENET_TRIHEDRON- FRENET_CURVE))
#define SHOW_FRENET_CURVATURE (1 << (FRENET_CURVATURE- FRENET_CURVE))
#define SHOW_FRENET_TORSION (1 << (FRENET_TORSION- FRENET_CURVE))
#define SHOW_FRENET_EVOLUTE (1 << (FRENET_EVOLUTE- FRENET_CURVE))
#define SHOW_FRENET_OFFSET (1 << (FRENET_OFFSET- FRENET_CURVE))
#define SHOW_FRENET_SPHERE (1 << (FRENET_SPHERE- FRENET_CURVE))


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
double offsetSize = 0.5;
double stepSize = 0.1;
double sleep_time = 1.0;
int numOfPoints;
UINT *id_pts, id_curve, *id_curvature, *id_torsion, *id_trihedron_T, *id_trihedron_N, *id_trihedron_B, id_evolute, id_offset, *id_sphere, id_axis[3];
int activePointIndex;
char my_state_mask = 0;

void hideAllPointwiseId() {
	for (int index = 0; index < numOfPoints; index++) {
		cagdHideSegment(id_curvature[index]);
	}
	for (int index = 0; index < numOfPoints; index++) {
		cagdHideSegment(id_trihedron_T[index]);
		cagdHideSegment(id_trihedron_N[index]);
		cagdHideSegment(id_trihedron_B[index]);
	}
	for (int index = 0; index < numOfPoints; index++) {
		cagdHideSegment(id_torsion[index]);
	}
	for (int index = 0; index < numOfPoints; index++) {
		cagdHideSegment(id_sphere[index]);
	}
}

void hideAllCurvewiseId() {
	if ((my_state_mask & SHOW_FRENET_CURVE) == 0)
		cagdHideSegment(id_curvature);
	if ((my_state_mask & SHOW_FRENET_EVOLUTE) == 0)
		cagdHideSegment(id_evolute);
	if ((my_state_mask & SHOW_FRENET_OFFSET) == 0)
		cagdHideSegment(id_offset);
}

void my_display() {
	hideAllPointwiseId();
	hideAllCurvewiseId();
	cagdRedraw();
}

void freeGlobals() {
	if (id_pts) {
		free(id_pts);
		id_pts = NULL;
	}
	if (id_trihedron_B) {
		free(id_trihedron_B);
		id_trihedron_B = NULL;
	}
	if (id_trihedron_N) {
		free(id_trihedron_N);
		id_trihedron_N = NULL;
	}
	if (id_trihedron_T) {
		free(id_trihedron_T);
		id_trihedron_T = NULL;
	}
	if (id_curvature) {
		free(id_curvature);
		id_curvature = NULL;
	}
	if (id_sphere) {
		free(id_sphere);
		id_sphere = NULL;
	}
	if (id_torsion) {
		free(id_torsion);
		id_torsion = NULL;
	}
}

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
	CAGD_POINT poly_tmp_pts[2];
	double asserter;
	double scale = 0.2;
	int index = 0;
	boolean isPlanar = FALSE;

	CAGD_POINT* my_pts, tmp_pt;
	double x_pt, y_pt, z_pt;
	e2t_expr_node* x_tree, *y_tree, *z_tree;

	CAGD_POINT* Tmy_pts;
	double Tx_pt, Ty_pt, Tz_pt;
	double* vector_size;
	e2t_expr_node* Tx_tree, *Ty_tree, *Tz_tree;


	CAGD_POINT* Nmy_pts;
	CAGD_POINT* Evolute_pts, *Offset_pts;
	double Nx_pt, Ny_pt, Nz_pt;
	double* kappa;
	double* kappa_prime;
	e2t_expr_node* Nx_tree, *Ny_tree, *Nz_tree;

	CAGD_POINT* Bmy_pts;
	double Bx_pt, By_pt, Bz_pt;
	double Bsize;
	double* tau;
	e2t_expr_node* Nprimex_tree, *Nprimey_tree, *Nprimez_tree;
	double Nprimex_pt, Nprimey_pt, Nprimez_pt;
	double scalar;
	

	numOfPoints = floor((domain_max - domain_min) / stepSize);
	asserter = stepSize * numOfPoints + domain_min;
	if (domain_max < asserter) {
		printf("too many points \n");
	}
	freeGlobals();


	my_pts = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * (numOfPoints));
	Tmy_pts = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * (numOfPoints));
	Nmy_pts = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * (numOfPoints));
	Bmy_pts = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * (numOfPoints));
	Evolute_pts = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * (numOfPoints));
	Offset_pts = (CAGD_POINT *)malloc(sizeof(CAGD_POINT) * (numOfPoints));

	vector_size = (double*)malloc(sizeof(double) * (numOfPoints));
	kappa = (double*)malloc(sizeof(double) * (numOfPoints));
	kappa_prime = (double*)malloc(sizeof(double) * (numOfPoints));
	tau = (double*)malloc(sizeof(double) * (numOfPoints));


	id_pts = (UINT*)malloc(sizeof(UINT) * (numOfPoints));
	id_trihedron_T = (UINT*)malloc(sizeof(UINT) * (numOfPoints));
	id_trihedron_N = (UINT*)malloc(sizeof(UINT) * (numOfPoints));
	id_trihedron_B = (UINT*)malloc(sizeof(UINT) * (numOfPoints));
	id_curvature = (UINT*)malloc(sizeof(UINT) * (numOfPoints));
	id_sphere = (UINT*)malloc(sizeof(UINT) * (numOfPoints));
	id_torsion = (UINT*)malloc(sizeof(UINT) * (numOfPoints));


	x_tree = buildTreeFromLine(x_line);
	y_tree = buildTreeFromLine(y_line);
	z_tree = buildTreeFromLine(z_line);

	Tx_tree = e2t_derivtree(x_tree, E2T_PARAM_T);
	Ty_tree = e2t_derivtree(y_tree, E2T_PARAM_T);
	Tz_tree = e2t_derivtree(z_tree, E2T_PARAM_T);
	char tmp[BUFSIZ];
	e2t_printtree(Tx_tree, tmp);
	printf("%s\n", tmp);
	e2t_printtree(Ty_tree, tmp);
	printf("%s\n", tmp);
	e2t_printtree(Tz_tree, tmp);
	printf("%s\n", tmp);
	Nx_tree = e2t_derivtree(Tx_tree, E2T_PARAM_T);
	Ny_tree = e2t_derivtree(Ty_tree, E2T_PARAM_T);
	Nz_tree = e2t_derivtree(Tz_tree, E2T_PARAM_T);
	char printtmp[BUFSIZ];
	e2t_printtree(Nx_tree, tmp);
	printf("%s\n", tmp);
	e2t_printtree(Ny_tree, tmp);
	printf("%s\n", tmp);
	e2t_printtree(Nz_tree, tmp);
	printf("%s\n", tmp);
	Nprimex_tree = e2t_derivtree(Nx_tree, E2T_PARAM_T);
	Nprimey_tree = e2t_derivtree(Ny_tree, E2T_PARAM_T);
	Nprimez_tree = e2t_derivtree(Nz_tree, E2T_PARAM_T);

	for (index = 0; index < numOfPoints; index++) {
		e2t_setparamvalue(stepSize * index + domain_min, E2T_PARAM_T);
		x_pt = e2t_evaltree(x_tree);
		y_pt = e2t_evaltree(y_tree);
		z_pt = e2t_evaltree(z_tree);

		tmp_pt = (CAGD_POINT) { .x = x_pt, .y = y_pt, .z = z_pt };
		my_pts[index] = tmp_pt;
		id_pts[index] = cagdAddPoint(&tmp_pt);

	}
	cagdSetColor(255, 0, 0);
	for (index = 0; index < numOfPoints; index++) {
		e2t_setparamvalue(stepSize * index + domain_min, E2T_PARAM_T);

		Tx_pt = e2t_evaltree(Tx_tree);
		Ty_pt = e2t_evaltree(Ty_tree);
		Tz_pt = e2t_evaltree(Tz_tree);

		tmp_pt = (CAGD_POINT) { .x = Tx_pt, .y = Ty_pt, .z = Tz_pt };
		Tmy_pts[index] = tmp_pt;

		vector_size[index] = sqrt(Tx_pt*Tx_pt + Ty_pt * Ty_pt + Tz_pt * Tz_pt);
		Tx_pt = Tx_pt / vector_size[index];
		Ty_pt = Ty_pt / vector_size[index];
		Tz_pt = Tz_pt / vector_size[index];

		poly_tmp_pts[0] = my_pts[index];
		poly_tmp_pts[1] = (CAGD_POINT) {
			.x = my_pts[index].x + Tx_pt * scale,
				.y = my_pts[index].y + Ty_pt * scale,
				.z = my_pts[index].z + Tz_pt * scale,
		};

		id_trihedron_T[index] = cagdAddPolyline(poly_tmp_pts, 2);
	}
	cagdSetColor(0, 255, 0);
	for (index = 0; index < numOfPoints; index++) {
		e2t_setparamvalue(stepSize * index + domain_min, E2T_PARAM_T);

		Nx_pt = e2t_evaltree(Nx_tree);
		Ny_pt = e2t_evaltree(Ny_tree);
		Nz_pt = e2t_evaltree(Nz_tree);

		kappa[index] = sqrt(Nx_pt*Nx_pt + Ny_pt * Ny_pt + Nz_pt * Nz_pt);
		Nx_pt = Nx_pt / kappa[index];
		Ny_pt = Ny_pt / kappa[index];
		Nz_pt = Nz_pt / kappa[index];
		tmp_pt = (CAGD_POINT) { .x = Nx_pt, .y = Ny_pt, .z = Nz_pt };
		Nmy_pts[index] = tmp_pt;

		
		Offset_pts[index].x = my_pts[index].x + (Nx_pt*offsetSize );
		Offset_pts[index].y = my_pts[index].y + (Ny_pt *offsetSize);
		Offset_pts[index].z = my_pts[index].z + (Nz_pt * offsetSize);


		poly_tmp_pts[0] = my_pts[index];
		poly_tmp_pts[1] = (CAGD_POINT) {
			.x = my_pts[index].x + Nx_pt * scale,
				.y = my_pts[index].y + Ny_pt * scale,
				.z = my_pts[index].z + Nz_pt * scale,
		};
		 
		id_trihedron_N[index] = cagdAddPolyline(poly_tmp_pts, 2);

		double R = 1.0f / kappa[index];  // Radius of curvature
		CAGD_POINT center = {
			.x = my_pts[index].x + R * Nx_pt,
			.y = my_pts[index].y + R * Ny_pt,
			.z = my_pts[index].z + R * Nz_pt
		};

		Evolute_pts[index]= center; // the evolute is the curve of all the ceners of the osculating circles
		
		double stepDegree = 2 * 3.14159265358979323846 / 32;
		CAGD_POINT circlePoints[32];
	
		for (int circlestep = 0; circlestep < 32; circlestep++) {
			double theta = circlestep * stepDegree;
			double cos_theta = cosf(theta);
			double sin_theta = sinf(theta);

			// Circle point in T-N plane
			circlePoints[circlestep] = (CAGD_POINT) {
				.x = center.x + R * (cos_theta * Nmy_pts[index].x /kappa[index] + sin_theta * Tmy_pts[index].x /vector_size[index]),
					.y = center.y + R * (cos_theta * Nmy_pts[index].y / kappa[index] + sin_theta * Tmy_pts[index].y / vector_size[index]),
					.z = center.z + R * (cos_theta * Nmy_pts[index].z / kappa[index] + sin_theta * Tmy_pts[index].z / vector_size[index])
			};
		}

		cagdSetColor(255, 255, 0);
		
		
	id_curvature[index] = cagdAddPolyline(circlePoints, 32);
	
		cagdSetColor(0, 255, 0);
	}

	cagdSetColor(100, 100, 100);
	id_evolute = cagdAddPolyline(Evolute_pts, numOfPoints);
	id_offset = cagdAddPolyline(Offset_pts, numOfPoints);
	cagdSetColor(0, 0, 255);
	for (index = 0; index < numOfPoints; index++) {
		e2t_setparamvalue(stepSize * index + domain_min, E2T_PARAM_T);

		Bx_pt = (Tmy_pts[index].y) * (Nmy_pts[index].z) - (Tmy_pts[index].z) * (Nmy_pts[index].y);
		By_pt = (Tmy_pts[index].z) * (Nmy_pts[index].x) - (Tmy_pts[index].x) * (Nmy_pts[index].z);
		Bz_pt = (Tmy_pts[index].x) * (Nmy_pts[index].y) - (Tmy_pts[index].y) * (Nmy_pts[index].x);

		Bsize = sqrt(Bx_pt*Bx_pt + By_pt * By_pt + Bz_pt * Bz_pt);

		Nprimex_pt = e2t_evaltree(Nprimex_tree);
		Nprimey_pt = e2t_evaltree(Nprimey_tree);
		Nprimez_pt = e2t_evaltree(Nprimez_tree);
		
		kappa_prime[index] = (Nmy_pts[index].x * Nprimex_pt) + (Nmy_pts[index].y  * Nprimey_pt) + (Nmy_pts[index].z  *Nprimez_pt) / kappa[index];

		scalar = Bx_pt * Nprimex_pt + By_pt * Nprimey_pt + Bz_pt * Nprimez_pt;

		tau[index] = scalar / (Bsize*Bsize);

		tmp_pt = (CAGD_POINT) { .x = Bx_pt, .y = By_pt, .z = Bz_pt};
		Bmy_pts[index] = tmp_pt;

		Bx_pt = Bx_pt / Bsize;
		By_pt = By_pt / Bsize;
		Bz_pt = Bz_pt / Bsize;

		poly_tmp_pts[0] = my_pts[index];
		poly_tmp_pts[1] = (CAGD_POINT) {
			.x = my_pts[index].x + Bx_pt * scale ,
				.y = my_pts[index].y + By_pt * scale,
				.z = my_pts[index].z + Bz_pt * scale ,
		};

		id_trihedron_B[index] = cagdAddPolyline(poly_tmp_pts, 2);

		Tx_pt = Tmy_pts[index].x / vector_size[index];
		Ty_pt = Tmy_pts[index].y / vector_size[index];
		Tz_pt = Tmy_pts[index].z / vector_size[index];

		Nx_pt = Nmy_pts[index].x / kappa[index];
		Ny_pt = Nmy_pts[index].y / kappa[index];
		Nz_pt = Nmy_pts[index].z / kappa[index];


		
		double stepDegree = 2 * 3.14159265358979323846 / 32;
		CAGD_POINT helixPoints[32];

		double helixR = 1.0f / tau[index]; 
		double helixHeight = helixR; // height of helix is the same as it's radius
		printf("|T| = %.4lf\t|N| = %.4lf\t|B| = %.4lf\n",
			sqrt(Tx_pt*Tx_pt + Ty_pt * Ty_pt + Tz_pt * Tz_pt),
			sqrt(Nx_pt*Nx_pt + Ny_pt * Ny_pt + Nz_pt * Nz_pt),
			sqrt(Bx_pt*Bx_pt + By_pt * By_pt + Bz_pt * Bz_pt));

		printf("T•N = %.4lf\tT•B = %.4lf\tN•B = %.4lf\n",
			Tx_pt*Nx_pt + Ty_pt * Ny_pt + Tz_pt * Nz_pt,
			Tx_pt*Bx_pt + Ty_pt * By_pt + Tz_pt * Bz_pt,
			Nx_pt*Bx_pt + Ny_pt * By_pt + Nz_pt * Bz_pt);

		// helix height
		for (int helixstep = 0; helixstep < 32; helixstep++) {
			double t = helixstep * stepDegree;
			double cos_t = cosf(t);
			double sin_t = sinf(t);
			// helix with (0,0,0) as the starting point
			//	x = R (cos(t) - 1)
			//	y = R sin(t)
			//  z = Ht
			
			// bruh im so bad at linear algebra
			helixPoints[helixstep] = (CAGD_POINT) {
				.x = my_pts[index].x
					+ Tx_pt * helixR * (cos_t - 1)
					+ Nx_pt * helixR * sin_t
					+ Bx_pt * helixHeight * t,

					.y = my_pts[index].y
					+ Ty_pt * helixR * (cos_t - 1)
					+ Ny_pt * helixR * sin_t
					+ By_pt * helixHeight * t,

					.z = my_pts[index].z
					+ Tz_pt * helixR * (cos_t - 1)
					+ Nz_pt * helixR * sin_t
					+ Bz_pt * helixHeight * t,
			};

		}
		
		cagdSetColor(255, 200, 230);

		CAGD_POINT sphereCenter =
		{ .x = my_pts[index].x + Nx_pt / kappa[index] - (Bx_pt * kappa_prime[index]) / (kappa[index] * kappa[index] * tau[index]),
		.y = my_pts[index].y + (Ny_pt / kappa[index]) - (kappa_prime[index] / (kappa[index] * kappa[index] * tau[index])) * By_pt,
		.z = my_pts[index].z + (Nz_pt / kappa[index]) - (kappa_prime[index] / (kappa[index] * kappa[index] * tau[index])) * Bz_pt };

		double sphereR = sqrt((1 / kappa[index])*(1 / kappa[index]) +
			(kappa_prime[index] / (kappa[index] * kappa[index] * tau[index]))*(kappa_prime[index] / (kappa[index] * kappa[index] * tau[index])));

		CAGD_POINT sphereSpiralPoints[SPHERE_POINT_NUM * SPHERE_POINT_NUM];
		int i = 0;
		stepDegree = 2 * 3.14159265358979323846f / SPHERE_POINT_NUM;
		for (int spherestep_theta = 0; spherestep_theta < SPHERE_POINT_NUM; spherestep_theta++) {
			double theta = spherestep_theta * stepDegree;
			double cos_theta = cosf(theta);
			double sin_theta = sinf(theta);
			for (int spherestep_phi = 0; spherestep_phi < SPHERE_POINT_NUM; spherestep_phi++) {
				double phi = spherestep_phi * stepDegree;
				double cos_phi = cosf(phi);
				double sin_phi = sinf(phi);

				CAGD_POINT tmp = { .x =sphereCenter.x + sphereR * sin_theta * cos_phi,
									.y = sphereCenter.y + sphereR * sin_theta * sin_phi,
									.z = sphereCenter.z + sphereR * cos_theta,
				};

				sphereSpiralPoints[i] = tmp;
				i++;
			}
			
		}
		id_sphere[index] = cagdAddPolyline(sphereSpiralPoints, SPHERE_POINT_NUM*SPHERE_POINT_NUM);
		//printf("%d\n", id_sphere[index]);
		cagdSetColor(255, 150, 0);
		
		id_torsion[index] = cagdAddPolyline(helixPoints, 32);
		cagdSetColor(0, 0, 255);

	}


	
	cagdSetColor(255, 255, 255);

	id_curve = cagdAddPolyline(my_pts, numOfPoints );


	free(my_pts);
	free(Tmy_pts);
	free(Nmy_pts);
	free(Bmy_pts);
	free(Evolute_pts);
	free(Offset_pts);
	free(vector_size);
	free(kappa);
	free(kappa_prime);
	free(tau);

	e2t_freetree(x_tree);
	e2t_freetree(y_tree);
	e2t_freetree(z_tree);
	e2t_freetree(Tx_tree);
	e2t_freetree(Ty_tree);
	e2t_freetree(Tz_tree);
	e2t_freetree(Nx_tree);
	e2t_freetree(Ny_tree);
	e2t_freetree(Nz_tree);
	e2t_freetree(Nprimex_tree);
	e2t_freetree(Nprimey_tree);
	e2t_freetree(Nprimez_tree);

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
	char x_buff[BUFSIZ], y_buff[BUFSIZ], z_buff[BUFSIZ], min_buff[32], max_buff[32], ssize_buff[32], offset_buff[32], sleep_buff[32];
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
		sprintf(offset_buff, " %lf ", offsetSize);
		SetDlgItemText(hwnd, IDC_EDIT_OFFSET, offset_buff);
		sprintf(sleep_buff, " %lf ", sleep_time);
		SetDlgItemText(hwnd, IDC_EDIT_OFFSET, sleep_buff);
		break;
	case WM_COMMAND:
		// If OK button is pressed (IDOK)
		if (LOWORD(wParam) == IDOK) {

			// Retrieve text input from the three edit controls (Red, Green, Blue)
			GetDlgItemText(hwnd, IDC_EDIT_X, x_buff, sizeof(x_buff)); 
			GetDlgItemText(hwnd, IDC_EDIT_Y, y_buff, sizeof(y_buff)); 
			GetDlgItemText(hwnd, IDC_EDIT_Z, z_buff, sizeof(z_buff)); 
			GetDlgItemText(hwnd, IDC_EDIT_MIN, min_buff, sizeof(min_buff));
			GetDlgItemText(hwnd, IDC_EDIT_MAX, max_buff, sizeof(max_buff));
			GetDlgItemText(hwnd, IDC_EDIT_SSIZE, ssize_buff, sizeof(ssize_buff));
			GetDlgItemText(hwnd, IDC_EDIT_OFFSET, offset_buff, sizeof(offset_buff));
			GetDlgItemText(hwnd, IDC_EDIT_OFFSET, sleep_buff, sizeof(sleep_buff));


			strncpy(x_line, x_buff, BUFSIZ);
			strncpy(y_line, y_buff, BUFSIZ);
			strncpy(z_line, z_buff, BUFSIZ);

			sscanf(min_buff, "%lf", &domain_min);
			sscanf(max_buff, "%lf", &domain_max);
			sscanf(ssize_buff, "%lf", &stepSize);
			sscanf(offset_buff, "%lf", &offsetSize);
			sscanf(sleep_buff, "%lf", &sleep_time);


			EndDialog(hwnd, IDOK);
			myCreateCAGD();
			my_display();
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

void display_point(int ptIndex) {
	// check flags and display
	printf("found this index %d\n", ptIndex);
	// depends on the flags, display the thing.
	if(my_state_mask & SHOW_FRENET_CURVATURE){
		cagdShowSegment(id_curvature[ptIndex]);
	}
	else {
		cagdHideSegment(id_curvature[ptIndex]);
	}
	if (my_state_mask & SHOW_FRENET_TRIHEDRON) {
		cagdShowSegment(id_trihedron_T[ptIndex]);
		cagdShowSegment(id_trihedron_N[ptIndex]);
		cagdShowSegment(id_trihedron_B[ptIndex]);

	}
	else {
		cagdHideSegment(id_trihedron_T[ptIndex]);
		cagdHideSegment(id_trihedron_N[ptIndex]);
		cagdHideSegment(id_trihedron_B[ptIndex]);

	}
	if (my_state_mask & SHOW_FRENET_TORSION) {
		cagdShowSegment(id_torsion[ptIndex]);
	}
	else {
		cagdHideSegment(id_torsion[ptIndex]);
	}
	if (my_state_mask & SHOW_FRENET_SPHERE) {
		cagdShowSegment(id_sphere[ptIndex]);
	}
	else {
		cagdHideSegment(id_sphere[ptIndex]);
	}
}

void hide_point(int ptIndex) {
	if (ptIndex >= 0) {
		cagdHideSegment(id_curvature[ptIndex]);

		cagdHideSegment(id_trihedron_T[ptIndex]);
		cagdHideSegment(id_trihedron_N[ptIndex]);
		cagdHideSegment(id_trihedron_B[ptIndex]);

		cagdHideSegment(id_torsion[ptIndex]);

		cagdHideSegment(id_sphere[ptIndex]);
	}
}

void animate() {
	
		hide_point((activePointIndex) % numOfPoints);
		display_point((activePointIndex+1) % numOfPoints);
		activePointIndex  = (activePointIndex + 1) % numOfPoints;
		
		cagdRedraw();
}

void myPointInteract(int x, int y, PVOID userData) {
	UINT id;
	int index = 0;
	boolean point_found = FALSE;
	cagdPick(x, y);

	while (id = cagdPickNext()) {
		if (cagdGetSegmentType(id) == CAGD_SEGMENT_POINT)
			break;
	}
	if (id != 0) {
		for (index = 0; index < numOfPoints; index++) {
			if (id == id_pts[index])
				break;
		}
		if (index != numOfPoints) {
			display_point(index);
			point_found = TRUE;
		}
	}
	hide_point(activePointIndex);

	if (point_found) {
		activePointIndex = index;
	}
	else {
		activePointIndex = -1;
	}
	cagdRedraw();
}

void myFrenet(int id, int unUsed, PVOID userData) {
	HMENU fMenu = (HMENU)userData;
	UINT state = GetMenuState(fMenu, id, MF_BYCOMMAND);
	UINT newState = state;
	if (id != M_PROPERTIES) {
		 newState = (state & MF_CHECKED) ? MF_UNCHECKED : MF_CHECKED;
	}
	cagdRegisterCallback(CAGD_LBUTTONDOWN, myPointInteract, NULL);

	CheckMenuItem(fMenu, id, MF_BYCOMMAND | newState);
	switch (id){
	case FRENET_CURVE:
		if (newState == MF_CHECKED) {
			if (!cagdShowSegment(id_curve)) {
				printf("ERROR: can't show curve!\n");
			}
			my_state_mask |= SHOW_FRENET_CURVE;
		}
		else {
			if (!cagdHideSegment(id_curve)) {
				printf("ERROR: can't hide curve!\n");
			}
			my_state_mask &= ~SHOW_FRENET_CURVE;
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
				my_state_mask |= SHOW_FRENET_POINTS;
			}
			else {
				for (int pointIndex = 0; pointIndex < numOfPoints; pointIndex++) {
					if (!cagdHideSegment(id_pts[pointIndex])) {
						printf("ERROR: can't hide point!\n");
					}
				}
				my_state_mask &= SHOW_FRENET_POINTS;

			}
		}
		
		break;
	case FRENET_TRIHEDRON:
		if (id_trihedron_T && id_trihedron_B && id_trihedron_N) {
			if (newState == MF_CHECKED) {

				my_state_mask |= SHOW_FRENET_TRIHEDRON;
			}
			else {
				my_state_mask &= ~SHOW_FRENET_TRIHEDRON;
			}
		}
		
		break;
	case FRENET_CURVATURE:
		if (id_curvature) {
			if (newState == MF_CHECKED) {

				my_state_mask |= SHOW_FRENET_CURVATURE;
			}
			else {
				my_state_mask &= SHOW_FRENET_CURVATURE;

			}
		}
		break;
	case FRENET_TORSION:
		if (id_torsion) {
			if (newState == MF_CHECKED) {
				
				my_state_mask |= SHOW_FRENET_TORSION;
			}
			else {
				
				my_state_mask &= SHOW_FRENET_TORSION;
			}
		}
		break;
	case FRENET_EVOLUTE:
		if (newState == MF_CHECKED) {
			if (!cagdShowSegment(id_evolute)) {
				printf("ERROR: can't show evolute!\n");
			}
			my_state_mask |= SHOW_FRENET_EVOLUTE;
		}
		else {
			if (!cagdHideSegment(id_evolute)) {
				printf("ERROR: can't hide evolute!\n");
			}
			my_state_mask &= ~SHOW_FRENET_EVOLUTE;
		}
		break;
	case FRENET_OFFSET:
		if (newState == MF_CHECKED) {
			if (!cagdShowSegment(id_offset)) {
				printf("ERROR: can't show offset!\n");
			}
			my_state_mask |= SHOW_FRENET_OFFSET;
		}
		else {
			if (!cagdHideSegment(id_offset)) {
				printf("ERROR: can't hide offset!\n");
			}
			my_state_mask &= ~SHOW_FRENET_OFFSET;
		}
		break;
	case FRENET_SPHERE:
		if (id_torsion) {
			if (newState == MF_CHECKED) {
				
				my_state_mask |= SHOW_FRENET_SPHERE;
			}
			else {
				
				my_state_mask &= SHOW_FRENET_SPHERE;
			}
		}
		break;
	case FRENET_AXIS:
		if (id_axis[0] && id_axis[1] && id_axis[2]) {
			if (newState == MF_CHECKED) {
				if (!cagdShowSegment(id_axis[0])) {
					printf("ERROR: can't show X!\n");
				}

				if (!cagdShowSegment(id_axis[1])) {
					printf("ERROR: can't show Y!\n");
				}

				if (!cagdShowSegment(id_axis[2])) {
					printf("ERROR: can't show Z!\n");
				}

			}
			else {
				if (!cagdHideSegment(id_axis[0])) {
					printf("ERROR: can't hide X!\n");
				}

				if (!cagdHideSegment(id_axis[1])) {
					printf("ERROR: can't hide Y!\n");
				}

				if (!cagdHideSegment(id_axis[2])) {
					printf("ERROR: can't hide Z!\n");
				}
			}

		}
		break;
	case FRENET_ANIMATION:
		hide_point(activePointIndex);
		activePointIndex = 0;
		if (newState == MF_CHECKED) {
			cagdRegisterCallback(CAGD_TIMER, animate, NULL);
			cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);

		}
		else {
			cagdRegisterCallback(CAGD_TIMER, NULL, NULL);
			cagdRegisterCallback(CAGD_LBUTTONDOWN, myPointInteract, NULL);


		}
		break;
	case M_PROPERTIES:
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
			
	}

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
	cagdRegisterCallback(CAGD_LBUTTONDOWN, myPointInteract, NULL);

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

void createOriginAxis() {
	CAGD_POINT origin = { .x = 0,.y = 0,.z = 0 };
	CAGD_POINT v_d = { .x = 1,.y = 0,.z = 0 };
	CAGD_POINT tmp[] = { origin, v_d };
	cagdSetColor(255, 0, 0);
	id_axis[0] = cagdAddPolyline(tmp, 2);
	tmp[1] = (CAGD_POINT) { .x = 0, .y = 1, .z = 0 };
	cagdSetColor(0, 255, 0);
	id_axis[1] = cagdAddPolyline(tmp, 2);
	tmp[1] = (CAGD_POINT) { .x = 0, .y = 0, .z = 1 };
	cagdSetColor(0, 0, 255);
	id_axis[2] = cagdAddPolyline(tmp, 2);
	cagdSetColor(255, 255, 255);
	cagdHideSegment(id_axis[0]);
	cagdHideSegment(id_axis[1]);
	cagdHideSegment(id_axis[2]);

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
	//cagdAppendMenu(hMenu, "Demos");
	myPopup = CreatePopupMenu();
	AppendMenu(myPopup, MF_STRING | MF_DISABLED, 0, "Point");
	AppendMenu(myPopup, MF_SEPARATOR, 0, NULL);
	AppendMenu(myPopup, MF_STRING, MY_ADD, "Add");
	AppendMenu(myPopup, MF_SEPARATOR, 0, NULL);
	AppendMenu(myPopup, MF_STRING, MY_COLOR, "Change color...");
	AppendMenu(myPopup, MF_STRING, MY_REMOVE, "Remove");
	boolean res = cagdRegisterCallback(CAGD_MENU, myCommand, (PVOID)hMenu);



	// New


	cagdRegisterCallback(CAGD_LOADFILE, myRead, NULL);
	cagdRegisterCallback(CAGD_SAVEFILE, mySave, NULL);
	createOriginAxis();

	// Frenet menu
	HMENU fMenu = CreatePopupMenu();
	AppendMenu(fMenu, MF_STRING | MF_CHECKED, FRENET_CURVE, "Curve");
	my_state_mask = my_state_mask | SHOW_FRENET_CURVE;

	AppendMenu(fMenu, MF_STRING | MF_CHECKED, FRENET_POINTS, "Points");
	my_state_mask = my_state_mask | SHOW_FRENET_POINTS;

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
