# Task List

## Basics

1. Display curve by stepping and sampling on the domain

2. Display Frenet frame (animated and moving)

3. Display osculating circle

4. Display line of torsion length

5. Click interation on curve -> display 2,3,4

6. Interactive editing of the curve's equation form

7. Modify parameter domain and step size for frenet frame

8. Modify fineness (domain boundary and steps) of the curve itself

9. Display curves from infix notation .dat files

## Advanced

1. Draw offset curves and evolute curves

2. Draw osculating sphere

3. Reparameterize the curves.

4. Custom (maybe heat? maybe speed with arc length and different params?)


# expr2tree stuff

For the first lab, you will need the files in this directory.  They will help you parse the expressions in the data files. Below is a short description.

The expr2tree module will convert infix expressions into binary trees,
derive the trees  with respect to a given  parameter, and evaluate them
at given parameter values. It supports:

(^+ ,- ,* ,/ ,) with the right priority and the following basic functions: sin, cos, tan, arcsin, arccos, arctan, sqrt, sqr, ln, log (base 10), exp.


Usage
-----
You must include the file expr2tree.h in your C file.  You must also compile and link in the expr2tree.c file found in this directory.  You may want to take a look in expr2tree.c - it has more information about error messages and other helpful stuff in its header.

e2t_expr_node *tree, *tree_dx;
double value;

To read an expression and store it in a tree data structures do the following:

	tree = e2t_expr2tree("X^2 + Y^2");

To evaluate the expression at X=1 and Y=2:

	e2t_setparamvalue(1.0, E2T_PARAM_X);        /* See expr2tree.h */
	e2t_setparamvalue(2.0, E2T_PARAM_Y;(
	value = e2t_evaltree(tree);

Note that E2T_PARAM_X and E2T_PARAM_Y are not variables.  They are constants which tell the evaluation procedure which parameter you mean to set.

To get partial derivative of the expression with respect to X:

	tree_dx = e2t_derivtree(tree, E2T_PARAM_X);

And 'tree_dx' can be used much like 'tree.'


Here is a summary of the expression manipulation routines you can use:

Main routines: 
1.	e2t_expr_node *e2t_expr2tree(s)   - main routine to perform the conversion.
2.	int e2t_parserror()      	      - return error number (see expr2tree.h for error codes).
3.	e2t_printtree(root, 0)    	      - routine to print an infix form content of the tree.
4.	e2t_expr_node *e2t_copytree(root) - returns a new copy of root pointed tree.
5.	double e2t_evaltree(root)         - evaluate expression for a given param.
6.	e2t_expr_node *e2t_derivtree(root, prm) - returns new tree, rep. its derivative according to parameter prm.
7.	int e2t_deriverror()              - return error number (see expr2tree.h for error codes.
8.	e2t_setparamvalue(Value, Number) - set a parameter value.

In addition:

9.	e2t_cmptree(root1, root2)     - symbolically compare two trees.
10.	e2t_paramintree(root, parameter) - return TRUE if parameter in tree.
11.	e2t_freetree(root) 	       - release memory allocated for tree root.
