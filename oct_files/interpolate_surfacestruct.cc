/*
Copyright (C) 2017 Schinzilord <schinzilord@octarisk.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.
*/

#include <octave/oct.h>
#include <octave/unwind-prot.h>
#include <cmath>
#include <octave/parse.h>
#include <octave/ov-struct.h>

void
my_err_handler (const char *fmt, ...)
{
  // Do nothing!!
}

void
my_err_with_id_handler (const char *id, const char *fmt, ...)
{
  // Do nothing!!
}

static bool any_bad_argument(const octave_value_list& args);
static double previous_neighbour(NDArray axis_values, double coord);
static double next_neighbour(NDArray axis_values, double coord);
static octave_idx_type get_indexvalue(NDArray axis_values, double coord);


DEFUN_DLD (interpolate_surfacestruct, args, nargout, "-*- texinfo -*-\n\
@deftypefn{Loadable Function} {@var{retvec}} = interpolate_surfacestruct(@var{struct},@var{xx}, @var{yy})\n\
\n\
Interpolate surface values from all elements of an structure array of\n\
all surface fieldnames.\n\
\n\
This function should be called from  getValue method of Surface class\n\
which handles all input and ouput data.\n\
Please note that axis values needs to be sorted.\n\
\n\
Input and output variables:\n\
@itemize @bullet\n\
@item @var{struct}: structure with the following fields (struct)\n\
@itemize @bullet\n\
@item @var{id}: scenario ID (string)\n\
@item @var{surface}: volatility surface (one volatility value per x,y,z coordinate) (NDArray)\n\
@item @var{x_axis}: sorted vector of x-axis values (NDArray)\n\
@item @var{y_axis}: sorted vector of y-axis values (NDArray)\n\
@end itemize\n\
@item @var{xx}: x coordinate used for interpolation of surface value\n\
@item @var{yy}: y coordinate used for interpolation of surface value (scalar or vector)\n\
@item @var{retvec}: return vector of scenario dependent interpolated volatility values (NDArray) \n\
@end itemize\n\
@end deftypefn")
{
  
  // Input parameter checks
	if (any_bad_argument(args))
	  return octave_value_list();
	  
  octave_map input_struct = args(0).map_value ();
  double xx            = args(1).double_value ();  // xx coordinates to interpolate
  NDArray yy           = args(2).array_value ();  // yy coordinates to interpolate

  // get fieldnames of provided struct
    std::string fieldname_cube = "cube";
    std::string fieldname_id = "id";
	std::string fieldname_xx = "axis_x";
	std::string fieldname_yy = "axis_y";
  
  // get length of input struct
    int len = input_struct.contents (fieldname_id).numel ();
    // std::cout << "Length of structure array:" << len << "\n";
  
  // initialize scenario dependent output:
	dim_vector dim_scen (len, 1);
	NDArray retvec (dim_scen);
	retvec.fill(0.0);
	octave_value vola_surface;
	NDArray vola_array;
	octave_value x_axis;
	NDArray xx_values;
	octave_value y_axis;
	NDArray yy_values;
  
  // check for yy: vector or scalar? check length
    NDArray yy_vec(dim_scen);
    if (yy.numel () == 1) {
		for (octave_idx_type kk = 0; kk < len; kk++) 
		{
		 	yy_vec(kk) = yy(1); 
		}
		
	} else {
		yy_vec = yy;
    }	
	
  // iterate over all structure array entries 
  for (octave_idx_type ii = 0; ii < len; ii++) 
	{
		// catch ctrl + c
            OCTAVE_QUIT;
			
		// get vola_surface
		vola_surface = input_struct.contents (fieldname_cube)(ii);
		if (! vola_surface.is_defined ())
			error ("interpolate_surfacestruct: struct does not have a field named 'cube'\n");
		vola_array    = vola_surface.array_value ();
		
		// get x axis values
		x_axis = input_struct.contents (fieldname_xx)(ii);
		if (! x_axis.is_defined ())
			error ("interpolate_surfacestruct: struct does not have a field named '%s'\n", fieldname_xx.c_str ());
		xx_values    = x_axis.array_value ();
		
		// get y axis values
		y_axis = input_struct.contents (fieldname_yy)(ii);
		if (! y_axis.is_defined ())
			error ("interpolate_surfacestruct: struct does not have a field named '%s'\n", fieldname_yy.c_str ());
		yy_values    = y_axis.array_value ();
		
		// interpolate vola_surface and return  interpolated values
		
		// get nearest values for xx, yyfrom axis values
			double x0 = previous_neighbour(xx_values, xx);
			double x1 = next_neighbour(xx_values, xx);
			double y0 = previous_neighbour(yy_values, yy_vec(ii));
			double y1 = next_neighbour(yy_values, yy_vec(ii));
		// get differences
			double xd, yd;
			if ( x0 == x1) { 
				xd = 0;
			} else {
				xd = (xx - x0) / (x1 - x0);
			}
			if ( y0 == y1) { 
				yd = 0;
			} else {
				yd = (yy_vec(ii) - y0) / (y1 - y0);
			}
		// extract volatility value
			octave_idx_type index_x0 = get_indexvalue(xx_values, x0);
			octave_idx_type index_y0 = get_indexvalue(yy_values, y0);
			octave_idx_type index_x1 = get_indexvalue(xx_values, x1);
			octave_idx_type index_y1 = get_indexvalue(yy_values, y1);
			double V_x0y0 = vola_array(index_y0,index_x0);
			double V_x0y1 = vola_array(index_y1,index_x0);
			double V_x1y0 = vola_array(index_y0,index_x1);
			double V_x1y1 = vola_array(index_y1,index_x1);
		// interpolate along x axis
			double c0 = V_x0y0 * (1 - xd ) + V_x1y0 * xd;
			double c1 = V_x0y1 * (1 - xd ) + V_x1y1 * xd;
		// interpolate along y axis and return final value "y"
			double retval = c0 * (1 - yd ) + c1 * yd;     
					
		// save result to return vector
		retvec(ii) = retval;
	}	// end iteration over all structure entries
  
  // return interpolated value vector
	octave_value_list option_outargs;
	option_outargs(0) = retvec;
	
   return octave_value (option_outargs);
} // end of DEFUN_DLD

//#########################    STATIC FUNCTIONS    #############################

// static function get index value of value in array
octave_idx_type get_indexvalue(NDArray axis_values, double coord)
    {
		octave_idx_type retval = 0;
		for (octave_idx_type ii = 0; ii < axis_values.numel(); ii++) 
		{
			if ( axis_values(ii) == coord)
				retval = ii;
		} 
		return retval;
    }

// static function getting axis value previous to input value
double previous_neighbour(NDArray axis_values, double coord)
    {
		if ( axis_values.numel() < 2 )
			return axis_values(0);
		
		double retval = 0.0;
		
		for (octave_idx_type ii = 0; ii < axis_values.numel(); ii++) 
		{
			if ( axis_values(ii) <= coord)
				retval =  axis_values(ii);
		}
		return retval;  
    }
	
// static function getting axis value next to input value
double next_neighbour(NDArray axis_values, double coord)
    {
		if ( axis_values.numel() < 2 )
			return axis_values(0);
		
		double retval = axis_values(axis_values.numel() - 1);
		
		for (octave_idx_type ii = axis_values.numel(); ii >= 0 ; ii--) 
		{
			if ( axis_values(ii) >= coord)
				retval =  axis_values(ii);
		}
		return retval;  
    }


// static function for input parameter checks
bool any_bad_argument(const octave_value_list& args)
{
    // octave_value_list:
    // struct, xx, yy
	
	if (args.length () != 3)
	{
		print_usage ();
		return true;
	}

	if (! args(0).is_map ())
	{
		error ("interpolate_surfacestruct: ARG0 must be a struct");
		return true;
    }

	if (! args(1).is_numeric_type())
	{
		error ("interpolate_surfacestruct: ARG1 must be numeric (Term)");
		return true;
    }

	if (! args(2).is_numeric_type())
	{
		error ("interpolate_surfacestruct: ARG2 must be numeric (Tenor)");
		return true;
    }
    
    return false;
}