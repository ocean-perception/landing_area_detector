// Computing normal of all facets in CGAL::Polyhedron_3
// https://saurabhg.com/programming/computing-normal-facets-cgalpolyhedron_3/

// Author(s) : Pierre Alliez
#include <iostream>
#include <fstream>
#include <boost/optional/optional_io.hpp>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/draw_surface_mesh.h>
#include <CGAL/draw_polyhedron.h>
#include <CGAL/draw_point_set_3.h>

#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>

typedef CGAL::Simple_cartesian<double>  K;
typedef K::FT                           FT;
typedef K::Point_3                      Point;
typedef K::Ray_3                        Ray;
typedef K::Segment_3                    Segment;
typedef CGAL::Polyhedron_3<K>           Polyhedron;
typedef Polyhedron::Vertex_iterator     Vertex_iterator;

typedef CGAL::AABB_face_graph_triangle_primitive<Polyhedron>    Primitive;
typedef CGAL::AABB_traits<K, Primitive>                         Traits;
typedef CGAL::AABB_tree<Traits>                                 Tree;
typedef Tree::Point_and_primitive_id                            Point_and_primitive_id;

typedef CGAL::Surface_mesh<Point>                               Mesh;
typedef boost::graph_traits<Mesh>::face_descriptor              face_descriptor;
typedef boost::optional<Tree::Intersection_and_primitive_id<Ray>::Type> Ray_intersection;

struct Skip { // structure to preprocess face descriptors
  face_descriptor fd;
  Skip(const face_descriptor fd)
    : fd(fd)
  {}
  bool operator()(const face_descriptor& t) const
  { if(t == fd){
      std::cerr << "ignore " << t  <<std::endl;
    };
    return(t == fd);
  }
};

int main(int argc, char* argv[])
{
    Polyhedron polyhedron;
    std::ifstream in1((argc>1)?argv[1]:"test.off");
    in1 >> polyhedron;

  // OBJECT CONSTRUCTION  *************************************************************************************************
    Point p(1.0, 0.0, 1.0);
    Point q(0.0, 1.0, 1.0);
    Point r(0.0, 0.0, 1.0);
    Point s(0.0, 0.0, 0.0);
    // Polyhedron polyhedron;
    // polyhedron.make_tetrahedron(p, q, r, s);

    // OBJECT INFO DUMP     *************************************************************************************************
    CGAL::set_ascii_mode( std::cout);
    for ( Vertex_iterator v = polyhedron.vertices_begin(); v != polyhedron.vertices_end(); ++v)
        std::cout << v->point() << std::endl;

    // OBJECT VISUALIZATION *************************************************************************************************
    CGAL::draw(polyhedron);

    // AABB TREE CREATION   *************************************************************************************************
    // constructs AABB tree and computes internal KD-tree
    // data structure to accelerate distance queries
    Tree tree(faces(polyhedron).first, faces(polyhedron).second, polyhedron);

    // OBJECT INTERSECTION  *************************************************************************************************
    // Ray creation
    // Intersection query
    // query point
    Point pointA(0.10, 0.10, 3.0);
    Point pointB(0.10, 0.10, -3.0);
    //*****************//
    Ray ray(pointA, pointB);

    // INTERSECTED FACET IDENTIFICATION
    std::cout << tree.number_of_intersected_primitives(ray)
        << " intersections(s) with ray query" << std::endl;
    // computes squared distance from query
    // FT sqd = tree.squared_distance(pointA);
    // std::cout << "squared distance: " << sqd << std::endl;

    // computes closest point
    Point closest = tree.closest_point(pointA);
    std::cout << "closest point [P]: " << closest << std::endl;

    // computes closest point and primitive id
    Point_and_primitive_id pp = tree.closest_point_and_primitive(pointA);
    Point closest_point = pp.first;
    Polyhedron::Face_handle f = pp.second; // closest primitive id >> this should give a pointer to the closest facet
    std::cout << "closest point [Pp]: " << closest_point << std::endl;
    std::cout << "closest triangle: ( "
              << f->halfedge()->vertex()->point() << " , "
              << f->halfedge()->next()->vertex()->point() << " , "
              << f->halfedge()->next()->next()->vertex()->point()
              << " )" << std::endl;

    std::cout << "---------------------------" << std::endl;
    // We are looking for those facets that are intersected by Ray

    return EXIT_SUCCESS;
}