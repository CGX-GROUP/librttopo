%{

/* WKT Parser */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rtin_wkt.h"
#include "rtin_wkt_parse.h"
#include "rtgeom_log.h"


/* Prototypes to quiet the compiler */
int wkt_yyparse(void);
void wkt_yyerror(const char *str);
int wkt_yylex(void);


/* Declare the global parser variable */
RTGEOM_PARSER_RESULT global_parser_result;

/* Turn on/off verbose parsing (turn off for production) */
int wkt_yydebug = 0;

/* 
* Error handler called by the bison parser. Mostly we will be 
* catching our own errors and filling out the message and errlocation
* from RTWKT_ERROR in the grammar, but we keep this one 
* around just in case.
*/
void wkt_yyerror(const char *str)
{
	/* If we haven't already set a message and location, let's set one now. */
	if ( ! global_parser_result.message ) 
	{
		global_parser_result.message = parser_error_messages[PARSER_ERROR_OTHER];
		global_parser_result.errcode = PARSER_ERROR_OTHER;
		global_parser_result.errlocation = wkt_yylloc.last_column;
	}
	RTDEBUGF(4,"%s", str);
}

/**
* Parse a WKT geometry string into an RTGEOM structure. Note that this
* process uses globals and is not re-entrant, so don't call it within itself
* (eg, from within other functions in rtin_wkt.c) or from a threaded program.
* Note that parser_result.wkinput picks up a reference to wktstr.
*/
int rtgeom_parse_wkt(RTGEOM_PARSER_RESULT *parser_result, char *wktstr, int parser_check_flags)
{
	int parse_rv = 0;

	/* Clean up our global parser result. */
	rtgeom_parser_result_init(&global_parser_result);
	/* Work-around possible bug in GNU Bison 3.0.2 resulting in wkt_yylloc
	 * members not being initialized on yyparse() as documented here:
	 * https://www.gnu.org/software/bison/manual/html_node/Location-Type.html
	 * See discussion here:
	 * http://lists.osgeo.org/pipermail/postgis-devel/2014-September/024506.html
	 */
	wkt_yylloc.last_column = wkt_yylloc.last_line = \
	wkt_yylloc.first_column = wkt_yylloc.first_line = 1;

	/* Set the input text string, and parse checks. */
	global_parser_result.wkinput = wktstr;
	global_parser_result.parser_check_flags = parser_check_flags;
		
	wkt_lexer_init(wktstr); /* Lexer ready */
	parse_rv = wkt_yyparse(); /* Run the parse */
	RTDEBUGF(4,"wkt_yyparse returned %d", parse_rv);
	wkt_lexer_close(); /* Clean up lexer */
	
	/* A non-zero parser return is an error. */
	if ( parse_rv != 0 ) 
	{
		if( ! global_parser_result.errcode )
		{
			global_parser_result.errcode = PARSER_ERROR_OTHER;
			global_parser_result.message = parser_error_messages[PARSER_ERROR_OTHER];
			global_parser_result.errlocation = wkt_yylloc.last_column;
		}

		RTDEBUGF(5, "error returned by wkt_yyparse() @ %d: [%d] '%s'", 
		            global_parser_result.errlocation, 
		            global_parser_result.errcode, 
		            global_parser_result.message);
		
		/* Copy the global values into the return pointer */
		*parser_result = global_parser_result;
                wkt_yylex_destroy();
		return RT_FAILURE;
	}
	
	/* Copy the global value into the return pointer */
	*parser_result = global_parser_result;
        wkt_yylex_destroy();
	return RT_SUCCESS;
}

#define RTWKT_ERROR() { if ( global_parser_result.errcode != 0 ) { YYERROR; } }


%}

%locations
%error-verbose
%name-prefix "wkt_yy"

%union {
	int integervalue;
	double doublevalue;
	char *stringvalue;
	RTGEOM *geometryvalue;
	POINT coordinatevalue;
	POINTARRAY *ptarrayvalue;
}

%token POINT_TOK LINESTRING_TOK POLYGON_TOK 
%token MPOINT_TOK MLINESTRING_TOK MPOLYGON_TOK 
%token MSURFACE_TOK MCURVE_TOK CURVEPOLYGON_TOK COMPOUNDCURVE_TOK CIRCULARSTRING_TOK
%token COLLECTION_TOK 
%token RBRACKET_TOK LBRACKET_TOK COMMA_TOK EMPTY_TOK
%token SEMICOLON_TOK
%token TRIANGLE_TOK TIN_TOK
%token POLYHEDRALSURFACE_TOK

%token <doublevalue> DOUBLE_TOK
%token <stringvalue> DIMENSIONALITY_TOK
%token <integervalue> SRID_TOK

%type <ptarrayvalue> ring
%type <ptarrayvalue> patchring
%type <ptarrayvalue> ptarray
%type <coordinatevalue> coordinate
%type <geometryvalue> circularstring
%type <geometryvalue> compoundcurve
%type <geometryvalue> compound_list
%type <geometryvalue> curve_list
%type <geometryvalue> curvepolygon
%type <geometryvalue> curvering
%type <geometryvalue> curvering_list
%type <geometryvalue> geometry
%type <geometryvalue> geometry_no_srid
%type <geometryvalue> geometry_list
%type <geometryvalue> geometrycollection
%type <geometryvalue> linestring
%type <geometryvalue> linestring_list
%type <geometryvalue> linestring_untagged
%type <geometryvalue> multicurve
%type <geometryvalue> multilinestring
%type <geometryvalue> multipoint
%type <geometryvalue> multipolygon
%type <geometryvalue> multisurface
%type <geometryvalue> patch
%type <geometryvalue> patch_list
%type <geometryvalue> patchring_list
%type <geometryvalue> point
%type <geometryvalue> point_list
%type <geometryvalue> point_untagged
%type <geometryvalue> polygon
%type <geometryvalue> polygon_list
%type <geometryvalue> polygon_untagged
%type <geometryvalue> polyhedralsurface
%type <geometryvalue> ring_list
%type <geometryvalue> surface_list
%type <geometryvalue> tin
%type <geometryvalue> triangle
%type <geometryvalue> triangle_list
%type <geometryvalue> triangle_untagged


/* These clean up memory on errors and parser aborts. */ 
%destructor { ptarray_free($$); } ptarray 
%destructor { ptarray_free($$); } ring
%destructor { ptarray_free($$); } patchring
%destructor { rtgeom_free($$); } curvering_list
%destructor { rtgeom_free($$); } triangle_list
%destructor { rtgeom_free($$); } surface_list
%destructor { rtgeom_free($$); } polygon_list
%destructor { rtgeom_free($$); } patch_list
%destructor { rtgeom_free($$); } point_list
%destructor { rtgeom_free($$); } linestring_list
%destructor { rtgeom_free($$); } curve_list
%destructor { rtgeom_free($$); } compound_list
%destructor { rtgeom_free($$); } ring_list
%destructor { rtgeom_free($$); } patchring_list
%destructor { rtgeom_free($$); } circularstring
%destructor { rtgeom_free($$); } compoundcurve
%destructor { rtgeom_free($$); } curvepolygon
%destructor { rtgeom_free($$); } curvering
%destructor { rtgeom_free($$); } geometry_no_srid
%destructor { rtgeom_free($$); } geometrycollection
%destructor { rtgeom_free($$); } linestring
%destructor { rtgeom_free($$); } linestring_untagged
%destructor { rtgeom_free($$); } multicurve
%destructor { rtgeom_free($$); } multilinestring
%destructor { rtgeom_free($$); } multipoint
%destructor { rtgeom_free($$); } multipolygon
%destructor { rtgeom_free($$); } multisurface
%destructor { rtgeom_free($$); } point
%destructor { rtgeom_free($$); } point_untagged
%destructor { rtgeom_free($$); } polygon
%destructor { rtgeom_free($$); } patch
%destructor { rtgeom_free($$); } polygon_untagged
%destructor { rtgeom_free($$); } polyhedralsurface
%destructor { rtgeom_free($$); } tin
%destructor { rtgeom_free($$); } triangle
%destructor { rtgeom_free($$); } triangle_untagged

%%

geometry:
	geometry_no_srid 
		{ wkt_parser_geometry_new($1, SRID_UNKNOWN); RTWKT_ERROR(); } |
	SRID_TOK SEMICOLON_TOK geometry_no_srid 
		{ wkt_parser_geometry_new($3, $1); RTWKT_ERROR(); } ;

geometry_no_srid : 
	point { $$ = $1; } | 
	linestring { $$ = $1; } | 
	circularstring { $$ = $1; } | 
	compoundcurve { $$ = $1; } | 
	polygon { $$ = $1; } | 
	curvepolygon { $$ = $1; } | 
	multipoint { $$ = $1; } |
	multilinestring { $$ = $1; } | 
	multipolygon { $$ = $1; } |
	multisurface { $$ = $1; } |
	multicurve { $$ = $1; } |
	tin { $$ = $1; } |
	polyhedralsurface { $$ = $1; } |
	triangle { $$ = $1; } |
	geometrycollection { $$ = $1; } ;
	
geometrycollection :
	COLLECTION_TOK LBRACKET_TOK geometry_list RBRACKET_TOK 
		{ $$ = wkt_parser_collection_finalize(RTCOLLECTIONTYPE, $3, NULL); RTWKT_ERROR(); } |
	COLLECTION_TOK DIMENSIONALITY_TOK LBRACKET_TOK geometry_list RBRACKET_TOK 
		{ $$ = wkt_parser_collection_finalize(RTCOLLECTIONTYPE, $4, $2); RTWKT_ERROR(); } |
	COLLECTION_TOK DIMENSIONALITY_TOK EMPTY_TOK 
		{ $$ = wkt_parser_collection_finalize(RTCOLLECTIONTYPE, NULL, $2); RTWKT_ERROR(); } |
	COLLECTION_TOK EMPTY_TOK 
		{ $$ = wkt_parser_collection_finalize(RTCOLLECTIONTYPE, NULL, NULL); RTWKT_ERROR(); } ;
	
geometry_list :
	geometry_list COMMA_TOK geometry_no_srid 
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	geometry_no_srid 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } ;

multisurface :
	MSURFACE_TOK LBRACKET_TOK surface_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTISURFACETYPE, $3, NULL); RTWKT_ERROR(); } |
	MSURFACE_TOK DIMENSIONALITY_TOK LBRACKET_TOK surface_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTISURFACETYPE, $4, $2); RTWKT_ERROR(); } |
	MSURFACE_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTISURFACETYPE, NULL, $2); RTWKT_ERROR(); } |
	MSURFACE_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTISURFACETYPE, NULL, NULL); RTWKT_ERROR(); } ;
	
surface_list :
	surface_list COMMA_TOK polygon
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	surface_list COMMA_TOK curvepolygon
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	surface_list COMMA_TOK polygon_untagged
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	polygon 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } |
	curvepolygon 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } |
	polygon_untagged 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } ;

tin :
	TIN_TOK LBRACKET_TOK triangle_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTTINTYPE, $3, NULL); RTWKT_ERROR(); } |
	TIN_TOK DIMENSIONALITY_TOK LBRACKET_TOK triangle_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTTINTYPE, $4, $2); RTWKT_ERROR(); } |
	TIN_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTTINTYPE, NULL, $2); RTWKT_ERROR(); } |
	TIN_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTTINTYPE, NULL, NULL); RTWKT_ERROR(); } ;

polyhedralsurface :
	POLYHEDRALSURFACE_TOK LBRACKET_TOK patch_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTPOLYHEDRALSURFACETYPE, $3, NULL); RTWKT_ERROR(); } |
	POLYHEDRALSURFACE_TOK DIMENSIONALITY_TOK LBRACKET_TOK patch_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTPOLYHEDRALSURFACETYPE, $4, $2); RTWKT_ERROR(); } |
	POLYHEDRALSURFACE_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTPOLYHEDRALSURFACETYPE, NULL, $2); RTWKT_ERROR(); } |
	POLYHEDRALSURFACE_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTPOLYHEDRALSURFACETYPE, NULL, NULL); RTWKT_ERROR(); } ;

multipolygon :
	MPOLYGON_TOK LBRACKET_TOK polygon_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTIPOLYGONTYPE, $3, NULL); RTWKT_ERROR(); } |
	MPOLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK polygon_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTIPOLYGONTYPE, $4, $2); RTWKT_ERROR(); } |
	MPOLYGON_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTIPOLYGONTYPE, NULL, $2); RTWKT_ERROR(); } |
	MPOLYGON_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTIPOLYGONTYPE, NULL, NULL); RTWKT_ERROR(); } ;

polygon_list :
	polygon_list COMMA_TOK polygon_untagged 
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	polygon_untagged 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } ;

patch_list :
	patch_list COMMA_TOK patch 
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	patch 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } ;

polygon : 
	POLYGON_TOK LBRACKET_TOK ring_list RBRACKET_TOK 
		{ $$ = wkt_parser_polygon_finalize($3, NULL); RTWKT_ERROR(); } |
	POLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK ring_list RBRACKET_TOK 
		{ $$ = wkt_parser_polygon_finalize($4, $2); RTWKT_ERROR(); } |
	POLYGON_TOK DIMENSIONALITY_TOK EMPTY_TOK 
		{ $$ = wkt_parser_polygon_finalize(NULL, $2); RTWKT_ERROR(); } |
	POLYGON_TOK EMPTY_TOK 
		{ $$ = wkt_parser_polygon_finalize(NULL, NULL); RTWKT_ERROR(); } ;

polygon_untagged : 
	LBRACKET_TOK ring_list RBRACKET_TOK 
		{ $$ = $2; } |
	EMPTY_TOK
		{ $$ = wkt_parser_polygon_finalize(NULL, NULL); RTWKT_ERROR(); };

patch : 
	LBRACKET_TOK patchring_list RBRACKET_TOK { $$ = $2; } ;

curvepolygon :
	CURVEPOLYGON_TOK LBRACKET_TOK curvering_list RBRACKET_TOK
		{ $$ = wkt_parser_curvepolygon_finalize($3, NULL); RTWKT_ERROR(); } |
	CURVEPOLYGON_TOK DIMENSIONALITY_TOK LBRACKET_TOK curvering_list RBRACKET_TOK
		{ $$ = wkt_parser_curvepolygon_finalize($4, $2); RTWKT_ERROR(); } |
	CURVEPOLYGON_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_curvepolygon_finalize(NULL, $2); RTWKT_ERROR(); } |
	CURVEPOLYGON_TOK EMPTY_TOK
		{ $$ = wkt_parser_curvepolygon_finalize(NULL, NULL); RTWKT_ERROR(); } ;

curvering_list :
	curvering_list COMMA_TOK curvering 
		{ $$ = wkt_parser_curvepolygon_add_ring($1,$3); RTWKT_ERROR(); } |
	curvering 
		{ $$ = wkt_parser_curvepolygon_new($1); RTWKT_ERROR(); } ;

curvering :
	linestring_untagged { $$ = $1; } |
	linestring { $$ = $1; } |
	compoundcurve { $$ = $1; } |
	circularstring { $$ = $1; } ;

patchring_list :
	patchring_list COMMA_TOK patchring 
		{ $$ = wkt_parser_polygon_add_ring($1,$3,'Z'); RTWKT_ERROR(); } |
	patchring 
		{ $$ = wkt_parser_polygon_new($1,'Z'); RTWKT_ERROR(); } ;

ring_list :
	ring_list COMMA_TOK ring 
		{ $$ = wkt_parser_polygon_add_ring($1,$3,'2'); RTWKT_ERROR(); } |
	ring 
		{ $$ = wkt_parser_polygon_new($1,'2'); RTWKT_ERROR(); } ;

patchring :
	LBRACKET_TOK ptarray RBRACKET_TOK { $$ = $2; } ;

ring :
	LBRACKET_TOK ptarray RBRACKET_TOK { $$ = $2; } ;

compoundcurve :
	COMPOUNDCURVE_TOK LBRACKET_TOK compound_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTCOMPOUNDTYPE, $3, NULL); RTWKT_ERROR(); } |
	COMPOUNDCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK compound_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTCOMPOUNDTYPE, $4, $2); RTWKT_ERROR(); } |
	COMPOUNDCURVE_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTCOMPOUNDTYPE, NULL, $2); RTWKT_ERROR(); } |
	COMPOUNDCURVE_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTCOMPOUNDTYPE, NULL, NULL); RTWKT_ERROR(); } ;

compound_list :
	compound_list COMMA_TOK circularstring
		{ $$ = wkt_parser_compound_add_geom($1,$3); RTWKT_ERROR(); } |
	compound_list COMMA_TOK linestring
		{ $$ = wkt_parser_compound_add_geom($1,$3); RTWKT_ERROR(); } |
	compound_list COMMA_TOK linestring_untagged
		{ $$ = wkt_parser_compound_add_geom($1,$3); RTWKT_ERROR(); } |
	circularstring
		{ $$ = wkt_parser_compound_new($1); RTWKT_ERROR(); } |
	linestring
		{ $$ = wkt_parser_compound_new($1); RTWKT_ERROR(); } |
	linestring_untagged
		{ $$ = wkt_parser_compound_new($1); RTWKT_ERROR(); } ;

multicurve :
	MCURVE_TOK LBRACKET_TOK curve_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTICURVETYPE, $3, NULL); RTWKT_ERROR(); } |
	MCURVE_TOK DIMENSIONALITY_TOK LBRACKET_TOK curve_list RBRACKET_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTICURVETYPE, $4, $2); RTWKT_ERROR(); } |
	MCURVE_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTICURVETYPE, NULL, $2); RTWKT_ERROR(); } |
	MCURVE_TOK EMPTY_TOK
		{ $$ = wkt_parser_collection_finalize(RTMULTICURVETYPE, NULL, NULL); RTWKT_ERROR(); } ;

curve_list :
	curve_list COMMA_TOK circularstring
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	curve_list COMMA_TOK compoundcurve
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	curve_list COMMA_TOK linestring
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	curve_list COMMA_TOK linestring_untagged
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	circularstring
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } |
	compoundcurve
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } |
	linestring
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } |
	linestring_untagged
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } ;

multilinestring :
	MLINESTRING_TOK LBRACKET_TOK linestring_list RBRACKET_TOK 
		{ $$ = wkt_parser_collection_finalize(RTMULTILINETYPE, $3, NULL); RTWKT_ERROR(); } |
	MLINESTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK linestring_list RBRACKET_TOK 
		{ $$ = wkt_parser_collection_finalize(RTMULTILINETYPE, $4, $2); RTWKT_ERROR(); } |
	MLINESTRING_TOK DIMENSIONALITY_TOK EMPTY_TOK 
		{ $$ = wkt_parser_collection_finalize(RTMULTILINETYPE, NULL, $2); RTWKT_ERROR(); } |
	MLINESTRING_TOK EMPTY_TOK 
		{ $$ = wkt_parser_collection_finalize(RTMULTILINETYPE, NULL, NULL); RTWKT_ERROR(); } ;

linestring_list :
	linestring_list COMMA_TOK linestring_untagged 
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	linestring_untagged 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } ;

circularstring : 
	CIRCULARSTRING_TOK LBRACKET_TOK ptarray RBRACKET_TOK 
		{ $$ = wkt_parser_circularstring_new($3, NULL); RTWKT_ERROR(); } |
	CIRCULARSTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK 
		{ $$ = wkt_parser_circularstring_new($4, $2); RTWKT_ERROR(); } |
	CIRCULARSTRING_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_circularstring_new(NULL, $2); RTWKT_ERROR(); } |
	CIRCULARSTRING_TOK EMPTY_TOK 
		{ $$ = wkt_parser_circularstring_new(NULL, NULL); RTWKT_ERROR(); } ;

linestring : 
	LINESTRING_TOK LBRACKET_TOK ptarray RBRACKET_TOK 
		{ $$ = wkt_parser_linestring_new($3, NULL); RTWKT_ERROR(); } | 
	LINESTRING_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK 
		{ $$ = wkt_parser_linestring_new($4, $2); RTWKT_ERROR(); } |
	LINESTRING_TOK DIMENSIONALITY_TOK EMPTY_TOK 
		{ $$ = wkt_parser_linestring_new(NULL, $2); RTWKT_ERROR(); } |
	LINESTRING_TOK EMPTY_TOK 
		{ $$ = wkt_parser_linestring_new(NULL, NULL); RTWKT_ERROR(); } ;

linestring_untagged :
	LBRACKET_TOK ptarray RBRACKET_TOK 
		{ $$ = wkt_parser_linestring_new($2, NULL); RTWKT_ERROR(); } |
	EMPTY_TOK
		{ $$ = wkt_parser_linestring_new(NULL, NULL); RTWKT_ERROR(); };

triangle_list :
	triangle_list COMMA_TOK triangle_untagged 
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	triangle_untagged 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } ;

triangle :
	TRIANGLE_TOK LBRACKET_TOK LBRACKET_TOK ptarray RBRACKET_TOK RBRACKET_TOK 
		{ $$ = wkt_parser_triangle_new($4, NULL); RTWKT_ERROR(); } | 
	TRIANGLE_TOK DIMENSIONALITY_TOK LBRACKET_TOK LBRACKET_TOK ptarray RBRACKET_TOK RBRACKET_TOK 
		{ $$ = wkt_parser_triangle_new($5, $2); RTWKT_ERROR(); } |
	TRIANGLE_TOK DIMENSIONALITY_TOK EMPTY_TOK
		{ $$ = wkt_parser_triangle_new(NULL, $2); RTWKT_ERROR(); } | 
	TRIANGLE_TOK EMPTY_TOK
		{ $$ = wkt_parser_triangle_new(NULL, NULL); RTWKT_ERROR(); } ;

triangle_untagged :
	LBRACKET_TOK LBRACKET_TOK ptarray RBRACKET_TOK RBRACKET_TOK
		{ $$ = wkt_parser_triangle_new($3, NULL); RTWKT_ERROR(); } ;

multipoint :
	MPOINT_TOK LBRACKET_TOK point_list RBRACKET_TOK 
		{ $$ = wkt_parser_collection_finalize(RTMULTIPOINTTYPE, $3, NULL); RTWKT_ERROR(); } |
	MPOINT_TOK DIMENSIONALITY_TOK LBRACKET_TOK point_list RBRACKET_TOK 
		{ $$ = wkt_parser_collection_finalize(RTMULTIPOINTTYPE, $4, $2); RTWKT_ERROR(); } |
	MPOINT_TOK DIMENSIONALITY_TOK EMPTY_TOK 
		{ $$ = wkt_parser_collection_finalize(RTMULTIPOINTTYPE, NULL, $2); RTWKT_ERROR(); } |
	MPOINT_TOK EMPTY_TOK 
		{ $$ = wkt_parser_collection_finalize(RTMULTIPOINTTYPE, NULL, NULL); RTWKT_ERROR(); } ;

point_list :
	point_list COMMA_TOK point_untagged 
		{ $$ = wkt_parser_collection_add_geom($1,$3); RTWKT_ERROR(); } |
	point_untagged 
		{ $$ = wkt_parser_collection_new($1); RTWKT_ERROR(); } ;

point_untagged :
	coordinate 
		{ $$ = wkt_parser_point_new(wkt_parser_ptarray_new($1),NULL); RTWKT_ERROR(); } |
	LBRACKET_TOK coordinate RBRACKET_TOK
		{ $$ = wkt_parser_point_new(wkt_parser_ptarray_new($2),NULL); RTWKT_ERROR(); } |
	EMPTY_TOK
		{ $$ = wkt_parser_point_new(NULL, NULL); RTWKT_ERROR(); };

point : 
	POINT_TOK LBRACKET_TOK ptarray RBRACKET_TOK 
		{ $$ = wkt_parser_point_new($3, NULL); RTWKT_ERROR(); } |
	POINT_TOK DIMENSIONALITY_TOK LBRACKET_TOK ptarray RBRACKET_TOK 
		{ $$ = wkt_parser_point_new($4, $2); RTWKT_ERROR(); } |
	POINT_TOK DIMENSIONALITY_TOK EMPTY_TOK 
		{ $$ = wkt_parser_point_new(NULL, $2); RTWKT_ERROR(); } |
	POINT_TOK EMPTY_TOK 
		{ $$ = wkt_parser_point_new(NULL,NULL); RTWKT_ERROR(); } ;

ptarray : 
	ptarray COMMA_TOK coordinate 
		{ $$ = wkt_parser_ptarray_add_coord($1, $3); RTWKT_ERROR(); } |
	coordinate 
		{ $$ = wkt_parser_ptarray_new($1); RTWKT_ERROR(); } ;

coordinate : 
	DOUBLE_TOK DOUBLE_TOK 
		{ $$ = wkt_parser_coord_2($1, $2); RTWKT_ERROR(); } | 
	DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK 
		{ $$ = wkt_parser_coord_3($1, $2, $3); RTWKT_ERROR(); } | 
	DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK DOUBLE_TOK 
		{ $$ = wkt_parser_coord_4($1, $2, $3, $4); RTWKT_ERROR(); } ;

%%

