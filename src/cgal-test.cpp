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

// #include <CGAL/Path_on_surface.h>
// #include <CGAL/Curves_on_surface_topology.h>

typedef CGAL::Simple_cartesian<double>  K;        // kernel
typedef K::FT                           FT;       // ft
typedef K::Point_3                      Point;    // point
typedef K::Ray_3                        Ray;      // ray (for vertical intersection)
typedef K::Segment_3                    Segment;  // alternative to ray
typedef CGAL::Polyhedron_3<K>           Polyhedron;
typedef CGAL::Surface_mesh<Point>       Surface_mesh;

typedef Surface_mesh::Vertex_iterator   Vertex_iterator;
typedef Surface_mesh::Vertex_index      Vertex_descriptor;

typedef CGAL::AABB_face_graph_triangle_primitive<Surface_mesh>    Primitive;
typedef CGAL::AABB_traits<K, Primitive>                           Traits;
typedef CGAL::AABB_tree<Traits>                                   Tree;
typedef Tree::Point_and_primitive_id                              Point_and_primitive_id;

typedef boost::graph_traits<Surface_mesh>::face_descriptor              face_descriptor;
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
};//*/

namespace PMP = CGAL::Polygon_mesh_processing;
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
  // CGAL::draw(sm);

  // Step 4: Extract ConvexHull from geometry
  // Step 4.1:  Pull/convert mesh point to vector<Point> (compatible with LAD pipeline)
  // vector<Points> is already available from Surface_mesh.points()
  Surface_mesh convex_mesh;
  CGAL::convex_hull_3(sm.points().begin(), sm.points().end(), convex_mesh);

  // Step 5: Dump CH information
  cout << "Resulting convex hull: " << endl;
  cout << "# edges: "     << convex_mesh.num_edges() << endl;
  cout << "# faces: "     << convex_mesh.num_faces() << endl;
  cout << "# vertices: "  << convex_mesh.num_vertices() << endl;

  // Step 6: Draw CH
  // CGAL::draw(convex_mesh);

  // Step 7: Export CH as PLY/OFF
  CGAL::set_ascii_mode( std::cout);
  std::ofstream ofs;
  ofs.open ("convex.off", std::ofstream::out);
  CGAL::write_off(ofs, convex_mesh);
  ofs.close();

  // Step 8: Construct AABB tree from CH
  // AABB TREE CREATION   *************************************************************************************************
  // constructs AABB tree and computes internal KD-tree
  // data structure to accelerate distance queries
  // Tree tree(faces(polyhedron).first, faces(polyhedron).second, polyhedron);
  typedef CGAL::AABB_face_graph_triangle_primitive<Surface_mesh>  AABB_face_graph_primitive;
  typedef CGAL::AABB_traits<K, AABB_face_graph_primitive>         AABB_face_graph_traits;
  CGAL::AABB_tree<AABB_face_graph_traits> tree;

  PMP::build_AABB_tree(convex_mesh, tree); // build AABB tree from triangulated surface mesh

  // Step 9: Create Ray for interesection
  Point pointA(4.0, 4.0, 300.0);
  Point pointB(4.0, 4.0, -300.0);
  Ray ray(pointA, pointB);

 // INTERSECTED FACET IDENTIFICATION
  int n_int = tree.number_of_intersected_primitives(ray);
  std::cout << n_int << " intersections(s) with ray query" << std::endl;

  if (n_int < 1){
    // we failed, cancel
    cout << "No intersection found! Stopping..." << endl;
    return -1;
  }

  // Step 10: Locate intersected face (if any) and intersection point
  // check: https://doc.cgal.org/latest/Polygon_mesh_processing/Polygon_mesh_processing_2locate_example_8cpp-example.html#a11
  Face_location ray_location = PMP::locate_with_AABB_tree(ray, tree, convex_mesh);
  std::cout << "Intersection of the 3D ray and the mesh is in face " << ray_location.first
            << " with barycentric coordinates [" << ray_location.second[0] << " "
                                                 << ray_location.second[1] << " "
                                                 << ray_location.second[2] << "]\n";
  Point intersection = PMP::construct_point(ray_location, convex_mesh);
  std::cout << "It corresponds to point (" << intersection << ")\n";
  std::cout << "Is it on the face's border? " << (PMP::is_on_face_border(ray_location, convex_mesh) ? "Yes" : "No") << "\n\n";

  auto fid = ray_location.first;

  CGAL::SM_Halfedge_index hid;
  hid = convex_mesh.halfedge(fid); // gets a halfedge of face f
  cout << "Half edge data: " << hid << endl; 

  int inc = convex_mesh.degree(fid);
  cout << "Degree of face: " << inc <<endl;

  CGAL::Vertex_around_face_circulator<Surface_mesh> vcirc(convex_mesh.halfedge(fid), convex_mesh), done(vcirc); 
  do{
    cout << "Point for " << *vcirc++ << " || ";
    cout << convex_mesh.point(*vcirc) << endl;
  }while (vcirc != done);

  Polyhedron p_ch;
  p_ch.make_triangle(convex_mesh.point(*vcirc++), convex_mesh.point(*vcirc++), convex_mesh.point(*vcirc++));

  // CGAL::draw(p_ch);

  // Step 7: Export CH as PLY/OFF
  CGAL::set_ascii_mode( std::cout);
  // std::ofstream ofs;
  ofs.open ("incident.off", std::ofstream::out);
  CGAL::write_off(ofs, p_ch);
  ofs.close();

  // std::ofstream ofs;
  Polyhedron xray;
  // xray.make_triangle(ray.point())

  xray.make_triangle(pointA, pointB, pointA);
  ofs.open ("ray.off", std::ofstream::out);
  CGAL::write_off(ofs, xray);
  ofs.close();

  // CGAL::draw(xray);

  cout << endl;


  return 0;

    // CGAL::draw(ray_location.first);

    std::cout << "---------------------------" << std::endl;
    // We are looking for those facets that are intersected by Ray

    return EXIT_SUCCESS;
}