#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <iostream>
typedef CGAL::Simple_cartesian<double>     Kernel;
typedef Kernel::Point_3                    Point_3;
typedef Kernel::Plane_3                    Plane_3;
typedef CGAL::Polyhedron_3<Kernel>         Polyhedron;
typedef Polyhedron::Vertex_iterator        Vertex_iterator;

typedef Polyhedron::Facet_iterator         Facet_iterator;

// functor to be called at creation /typecasting time. Will populate the plane containers 
struct Plane_equation {
    template <class Facet>
    typename Facet::Plane_3 operator()( Facet& f) {
        typename Facet::Halfedge_handle h = f.halfedge();
        typedef typename Facet::Plane_3  Plane;
        return Plane( h->vertex()->point(),
                      h->next()->vertex()->point(),
                      h->next()->next()->vertex()->point());
    }
};


int main() {
    Point_3 p( 1.0, 0.0, 0.0);
    Point_3 q( 0.0, 1.0, 0.0);
    Point_3 r( 0.0, 0.0, 1.0);
    Point_3 s( 0.0, 0.0, 0.0);
    Polyhedron P;
    P.make_tetrahedron( p, q, r, s);
    CGAL::set_ascii_mode( std::cout);
    // force plane-creator functor call
    std::transform( P.facets_begin(), P.facets_end(), P.planes_begin(),
                    Plane_equation());

    // dump vertex data, meh
    for ( Vertex_iterator v = P.vertices_begin(); v != P.vertices_end(); ++v)
        std::cout << v->point() << std::endl;

    std::cout << "-------- now facets" << std::endl;

    // print (beauty) plane info, before copy.... meh
    for ( Facet_iterator f = P.facets_begin(); f != P.facets_end(); ++f){
        std::cout << f->plane() << std::endl;
        // f->
    }

    // print (beauty) plane info, after copy.... yay
    CGAL::set_pretty_mode( std::cout);
    std::copy( P.planes_begin(), P.planes_end(),
               std::ostream_iterator<Plane_3>( std::cout, "\n"));

    return 0;

}