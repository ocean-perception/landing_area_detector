// Computing normal of all facets in CGAL::Polyhedron_3
// https://saurabhg.com/programming/computing-normal-facets-cgalpolyhedron_3/

// Author(s) : Pierre Alliez
#include <iostream>
#include <fstream>
#include <boost/optional/optional_io.hpp>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/locate.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>

#include <CGAL/draw_surface_mesh.h>
#include <CGAL/draw_polyhedron.h>
#include <CGAL/draw_point_set_3.h>

#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>

typedef CGAL::Simple_cartesian<double>  K;        // kernel
typedef K::FT                           FT;       // ft
typedef K::Point_3                      Point;    // point
typedef K::Ray_3                        Ray;      // ray (for vertical intersection)
typedef K::Segment_3                    Segment;  // alternative to ray
typedef CGAL::Polyhedron_3<K>           Polyhedron;
typedef CGAL::Surface_mesh<Point>       Surface_mesh;

typedef Surface_mesh::Vertex_iterator   Vertex_iterator;
typedef Surface_mesh::Vertex_index      Vertex_descriptor;

// typedef CGAL::AABB_face_graph_triangle_primitive<Surface_mesh>    Primitive;
// typedef CGAL::AABB_traits<K, Primitive>                         Traits;
// typedef CGAL::AABB_tree<Traits>                                 Tree;
// typedef Tree::Point_and_primitive_id                            Point_and_primitive_id;

// typedef boost::graph_traits<Surface_mesh>::face_descriptor              face_descriptor;
// typedef boost::optional<Tree::Intersection_and_primitive_id<Ray>::Type> Ray_intersection;

/*struct Skip { // structure to preprocess face descriptors
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
};//*/

namespace PMP=CGAL::Polygon_mesh_processing;
typedef   PMP::Face_location<Surface_mesh, FT>      Face_location;

using namespace std;

int main(int argc, char* argv[])
{

  // Step 1: Load geometry from file
  Polyhedron polyhedron;
  std::ifstream in1((argc>1)?argv[1]:"test.off");
  // in1 >> polyhedron;
  Surface_mesh sm;      // <- Go after MESH
  in1 >> sm;  // import input file into surface mesh structure

  // Step 2: Dump geometry information 
  cout << "Input file: " << argv[1] << endl;
  cout << "# edges: " << sm.num_edges() << endl;
  cout << "# faces: " << sm.num_faces() << endl;
  cout << "# vertices: " << sm.num_vertices() << endl;

  // Step 3: Draw raw geometry
  CGAL::draw(sm);

  // Step 4: Extract ConvexHull from geometry

  // Step 4.1:  Pull/convert mesh point to vector<Point> (compatible with LAD pipeline)
  // vector<Points> is already available from Surface_mesh.points()
  Surface_mesh convex_mesh;

  CGAL::convex_hull_3(sm.points().begin(), sm.points().end(), convex_mesh);

  cout << "Resulting convex hull: " << endl;
  cout << "# edges: "     << convex_mesh.num_edges() << endl;
  cout << "# faces: "     << convex_mesh.num_faces() << endl;
  cout << "# vertices: "  << convex_mesh.num_vertices() << endl;

  CGAL::draw(convex_mesh);


  return 0;

  // Step 5: Dump CH information

  // Step 6: Draw CH

  // Step 7: Construct AABB tree from CH

  // Step 8: Construct CoG Ray for intersection

  // Step 9: Locate intersected face (if any) and intersection point

  // Step 10: Dump Face information

  // Step 11: Draw Face



  // OBJECT CONSTRUCTION  *************************************************************************************************
    Point p(1.0, 0.0, 1.0);
    Point q(0.0, 1.0, 1.0);
    Point r(0.0, 0.0, 1.0);
    Point s(0.0, 0.0, 0.0);
    // Polyhedron polyhedron;
    // polyhedron.make_tetrahedron(p, q, r, s);

    // OBJECT INFO DUMP     *************************************************************************************************
    CGAL::set_ascii_mode( std::cout);
    // for ( mVertex_iterator v = tm.vertices_begin(); v != tm.vertices_end(); ++v)
    //     std::cout << v->point() << std::endl;

    // OBJECT VISUALIZATION *************************************************************************************************
    // CGAL::draw(po`lyhedron);

    // AABB TREE CREATION   *************************************************************************************************
    // constructs AABB tree and computes internal KD-tree
    // data structure to accelerate distance queries
    // Tree tree(faces(polyhedron).first, faces(polyhedron).second, polyhedron);
    typedef CGAL::AABB_face_graph_triangle_primitive<Surface_mesh>                AABB_face_graph_primitive;
    typedef CGAL::AABB_traits<K, AABB_face_graph_primitive>               AABB_face_graph_traits;
    CGAL::AABB_tree<AABB_face_graph_traits> tree;
  
    // PMP::build_AABB_tree(sm, tree);

    // OBJECT INTERSECTION  *************************************************************************************************
    // Ray creation
    // Intersection query
    // query point
    Point pointA(0.10, 0.10, 3.0);
    Point pointB(0.10, 0.10, -3.0);
    //*****************//
    Ray ray(pointA, pointB);

    // INTERSECTED FACET IDENTIFICATION
    // std::cout << tree.number_of_intersected_primitives(ray)
    //     << " intersections(s) with ray query" << std::endl;

/*    Face_location ray_location = PMP::locate_with_AABB_tree(ray, tree, tm);

    std::cout << "Intersection of the 3D ray and the mesh is in face " << ray_location.first
              << " with barycentric coordinates ["  << ray_location.second[0] << " "
                                                    << ray_location.second[1] << " "
                                                    << ray_location.second[2] << "]\n";
    std::cout << "It corresponds to point (" << PMP::construct_point(ray_location, tm) << ")\n";
    std::cout << "Is it on the face's border? " << (PMP::is_on_face_border(ray_location, tm) ? "Yes" : "No") << "\n\n";
*/
  //***************************************************************************************//
  //***************************************************************************************//
  //***************************************************************************************//
  //***************************************************************************************//
    // CGAL::draw(ray_location.first);

    std::cout << "---------------------------" << std::endl;
    // We are looking for those facets that are intersected by Ray

    return EXIT_SUCCESS;
}