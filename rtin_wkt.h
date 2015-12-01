#include "librtgeom_internal.h"

/*
* Coordinate object to hold information about last coordinate temporarily.
* We need to know how many dimensions there are at any given time.
*/
typedef struct
{
	uint8_t flags;
	double x;
	double y;
	double z;
	double m;
}	
POINT;

/*
* Global that holds the final output geometry for the WKT parser.
*/
extern RTGEOM_PARSER_RESULT global_parser_result;
extern const char *parser_error_messages[];

/*
* Prototypes for the lexer
*/
extern void wkt_lexer_init(char *str);
extern void wkt_lexer_close(void);
extern int wkt_yylex_destroy(void);


/*
* Functions called from within the bison parser to construct geometries.
*/
int wkt_lexer_read_srid(char *str);
POINT wkt_parser_coord_2(double c1, double c2);
POINT wkt_parser_coord_3(double c1, double c2, double c3);
POINT wkt_parser_coord_4(double c1, double c2, double c3, double c4);
POINTARRAY* wkt_parser_ptarray_add_coord(POINTARRAY *pa, POINT p);
POINTARRAY* wkt_parser_ptarray_new(POINT p);
RTGEOM* wkt_parser_point_new(POINTARRAY *pa, char *dimensionality);
RTGEOM* wkt_parser_linestring_new(POINTARRAY *pa, char *dimensionality);
RTGEOM* wkt_parser_circularstring_new(POINTARRAY *pa, char *dimensionality);
RTGEOM* wkt_parser_triangle_new(POINTARRAY *pa, char *dimensionality);
RTGEOM* wkt_parser_polygon_new(POINTARRAY *pa, char dimcheck);
RTGEOM* wkt_parser_polygon_add_ring(RTGEOM *poly, POINTARRAY *pa, char dimcheck);
RTGEOM* wkt_parser_polygon_finalize(RTGEOM *poly, char *dimensionality);
RTGEOM* wkt_parser_curvepolygon_new(RTGEOM *ring);
RTGEOM* wkt_parser_curvepolygon_add_ring(RTGEOM *poly, RTGEOM *ring);
RTGEOM* wkt_parser_curvepolygon_finalize(RTGEOM *poly, char *dimensionality);
RTGEOM* wkt_parser_compound_new(RTGEOM *element);
RTGEOM* wkt_parser_compound_add_geom(RTGEOM *col, RTGEOM *geom);
RTGEOM* wkt_parser_collection_new(RTGEOM *geom);
RTGEOM* wkt_parser_collection_add_geom(RTGEOM *col, RTGEOM *geom);
RTGEOM* wkt_parser_collection_finalize(int rttype, RTGEOM *col, char *dimensionality);
void    wkt_parser_geometry_new(RTGEOM *geom, int srid);

