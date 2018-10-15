/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <algorithm>
#include "eckit/linalg/LinearAlgebra.h"
#include "eckit/linalg/Vector.h"
#include "eckit/memory/ScopedPtr.h"
#include "eckit/types/Types.h"

#include "atlas/array.h"
#include "atlas/array/ArrayView.h"
#include "atlas/array/MakeView.h"
#include "atlas/field/Field.h"
#include "atlas/functionspace/NodeColumns.h"
#include "atlas/functionspace/PointCloud.h"
#include "atlas/functionspace/StructuredColumns.h"
#include "atlas/grid/Grid.h"
#include "atlas/library/Library.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/meshgenerator/MeshGenerator.h"
#include "atlas/output/Gmsh.h"
#include "atlas/parallel/mpi/mpi.h"
#include "atlas/util/CoordinateEnums.h"
#include "atlas/util/MicroDeg.h"

#include "Stencil.h"
#include "tests/AtlasTestEnvironment.h"

using namespace eckit;
using namespace atlas::functionspace;
using namespace atlas::util;

namespace atlas {
namespace test {

//-----------------------------------------------------------------------------

std::vector<double> IFS_vertical_coordinates( idx_t nlev ) {
    std::vector<double> zcoord( nlev + 2 );
    zcoord[0]        = 0.;
    zcoord[nlev + 1] = 1.;
    double dzcoord   = 1. / double( nlev );
    for ( idx_t jlev = 1; jlev <= nlev; ++jlev ) {
        zcoord[jlev] = jlev * dzcoord - 0.5 * dzcoord;
    }
    return zcoord;
}

std::vector<double> zrange( idx_t nlev, double min, double max ) {
    std::vector<double> zcoord( nlev );
    double dzcoord = ( max - min ) / double( nlev - 1 );
    for ( idx_t jlev = 0; jlev < nlev; ++jlev ) {
        zcoord[jlev] = min + jlev * dzcoord;
    }
    return zcoord;
}

double cubic( double x, double min, double max ) {
    double x0   = min;
    double x1   = 0.5 * ( max + min );
    double x2   = max;
    double xmax = 0.5 * ( x0 + x1 );
    return ( x - x0 ) * ( x - x1 ) * ( x - x2 ) / ( ( xmax - x0 ) * ( xmax - x1 ) * ( xmax - x2 ) );
}

CASE( "test finding of North-West grid point" ) {
    std::string gridname = eckit::Resource<std::string>( "--grid", "O8" );

    grid::StructuredGrid grid( gridname );

    constexpr double tol = 0.5e-6;

    ComputeNorth compute_j_north( grid );
    ComputeWest compute_i_west( grid );

    struct IJ {
        idx_t i;
        idx_t j;
    };
    idx_t ny = grid.ny();

    if ( mpi::comm().size() == 1 ) {
        auto entries = {
            std::make_tuple( PointXY{0. + 0.5 * tol, grid.y( 0 ) + 0.5 * tol}, IJ{0, 0} ),
            std::make_tuple( PointXY{0. - 0.5 * tol, grid.y( 0 ) - 0.5 * tol}, IJ{0, 0} ),
            std::make_tuple( PointXY{0. + 2.0 * tol, grid.y( 0 ) + 2.0 * tol}, IJ{0, -1} ),
            std::make_tuple( PointXY{0. - 2.0 * tol, grid.y( 0 ) - 2.0 * tol}, IJ{-1, 0} ),
            std::make_tuple( PointXY{360. + 0.5 * tol, grid.y( ny - 1 ) + 0.5 * tol}, IJ{grid.nx( ny - 1 ), ny - 1} ),
            std::make_tuple( PointXY{360. - 0.5 * tol, grid.y( ny - 1 ) - 0.5 * tol}, IJ{grid.nx( ny - 1 ), ny - 1} ),
            std::make_tuple( PointXY{360. + 2.0 * tol, grid.y( ny - 1 ) + 2.0 * tol}, IJ{grid.nx( ny - 2 ), ny - 2} ),
            std::make_tuple( PointXY{360. - 2.0 * tol, grid.y( ny - 1 ) - 2.0 * tol},
                             IJ{grid.nx( ny - 1 ) - 1, ny - 1} ),
        };
        for ( auto entry : entries ) {
            auto p     = std::get<0>( entry );
            auto index = std::get<1>( entry );
            EXPECT( compute_j_north( p.y() ) == index.j );
            EXPECT( compute_i_west( p.x(), index.j ) == index.i );
        }
    }
}

CASE( "test horizontal stencil" ) {
    //if ( mpi::comm().size() == 1 ) {
    std::string gridname = eckit::Resource<std::string>( "--grid", "O8" );

    grid::StructuredGrid grid( gridname );
    int halo = eckit::Resource<int>( "--halo", 2 );
    util::Config config;
    config.set( "halo", halo );
    config.set( "levels", 9 );
    config.set( "periodic_points", true );
    functionspace::StructuredColumns fs( grid, grid::Partitioner( "equal_regions" ), config );

    double tol = 0.5e-6;

    constexpr int stencil_width = 4;
    HorizontalStencil<stencil_width> stencil;

    ComputeHorizontalStencil compute_stencil( grid, stencil.width() );

    auto departure_points = {
        PointXY( 0., 90. ),
        PointXY( 0., -90. ),
        PointXY( 0., 0. ),
        PointXY( 360., 0. ),
    };
    for ( auto p : departure_points ) {
        Log::info() << p << std::endl;
        compute_stencil( p.x(), p.y(), stencil );
        for ( idx_t j = 0; j < stencil.width(); ++j ) {
            Log::info() << stencil.i( j ) << " " << stencil.j( j ) << "   --   "
                        << "x,y = " << fs.compute_xy( stencil.i( j ), stencil.j( j ) ) << std::endl;
        }
        Log::info() << std::endl;
    }
}

CASE( "test vertical stencil" ) {
    idx_t nlev     = 10;
    auto zcoord    = IFS_vertical_coordinates( nlev );
    double dzcoord = 1. / double( nlev );

    ATLAS_DEBUG_VAR( zcoord );

    ComputeVertical compute_vertical( zcoord );

    const double eps = 1.e-14;

    for ( idx_t k = 1; k <= nlev; ++k ) {
        idx_t k_expected = std::max( 1, std::min( nlev - 1, k ) );
        EXPECT( compute_vertical( zcoord[k] ) == k_expected );
        EXPECT( compute_vertical( zcoord[k] - eps ) == k_expected );
        EXPECT( compute_vertical( zcoord[k] + eps ) == k_expected );
        EXPECT( compute_vertical( zcoord[k] + 0.5 * dzcoord ) == k_expected );
    }
    EXPECT( compute_vertical( zcoord[0] ) == 1 );
    EXPECT( compute_vertical( zcoord[nlev + 1] ) == nlev - 1 );

    ComputeVerticalStencil compute_vertical_stencil( zcoord, 4 );
    std::vector<double> departure_points{0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    for ( auto p : departure_points ) {
        VerticalStencil<4> stencil;
        compute_vertical_stencil( p, stencil );
        Log::info() << p << "   :    ";
        for ( idx_t k = 0; k < stencil.width(); ++k ) {
            Log::info() << stencil.k( k ) << " ";
        }
        Log::info() << std::endl;
    }
}

CASE( "test vertical cubic interpolation" ) {
    idx_t nlev    = 10;
    auto vertical = Vertical{nlev, IFS_vertical_coordinates( nlev - 2 ), util::Config( "boundaries", false )};

    CubicVerticalInterpolation interpolate( vertical );
    std::vector<double> departure_points{0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 0.3246};
    array::ArrayT<double> array( nlev + 2 );
    auto view = array::make_view<double, 1>( array );
    for ( idx_t k = 0; k <= nlev + 1; ++k ) {
        view( k ) = cubic( vertical( k ), 0., 1. );
    }
    //view(0) = -9999.;
    //view(nlev+1) = -9999.;

    for ( auto p : departure_points ) {
        Log::info() << p << "   :    " << interpolate( p, view ) << std::endl;
        EXPECT( eckit::types::is_approximately_equal( interpolate( p, view ), cubic( p, 0., 1. ) ) );
    }
}

//-----------------------------------------------------------------------------

CASE( "test horizontal cubic interpolation" ) {
    //if ( mpi::comm().size() == 1 ) {
    std::string gridname = eckit::Resource<std::string>( "--grid", "O8" );

    grid::StructuredGrid grid( gridname );
    int halo = eckit::Resource<int>( "--halo", 2 );
    util::Config config;
    config.set( "halo", halo );
    config.set( "levels", 9 );
    config.set( "periodic_points", true );
    functionspace::StructuredColumns fs( grid, grid::Partitioner( "equal_regions" ), config );

    Field field = fs.createField<double>( option::levels( 0 ) );
    auto f      = array::make_view<double, 1>( field );
    auto xy     = array::make_view<double, 2>( fs.xy() );

    auto fx  = []( double x ) { return cubic( x, 0., 360. ); };
    auto fy  = []( double y ) { return cubic( y, -90., 90. ); };
    auto fxy = [fx, fy]( double x, double y ) { return fx( x ) * fy( y ); };

    for ( idx_t j = 0; j < fs.size(); ++j ) {
        f( j ) = fxy( xy( j, XX ), xy( j, YY ) );
    }

    CubicStructuredInterpolation cubic_interpolation( fs );

    auto departure_points = {
        PointXY( 0.13257, 45.6397 ),
    };
    for ( auto p : departure_points ) {
        Log::info() << p << "  -->  " << cubic_interpolation( p.x(), p.y(), f ) << std::endl;
        EXPECT( eckit::types::is_approximately_equal( cubic_interpolation( p.x(), p.y(), f ), fxy( p.x(), p.y() ) ) );
    }
}

//-----------------------------------------------------------------------------

bool operator==( const eckit::linalg::Triplet& t1, const eckit::linalg::Triplet& t2 ) {
    if ( t1.row() != t2.row() ) return false;
    if ( t1.col() != t2.col() ) return false;
    if ( t1.value() != t2.value() ) return false;

    return true;
}
bool operator!=( const eckit::linalg::Triplet& t1, const eckit::linalg::Triplet& t2 ) {
    return !( t1 == t2 );
}
bool operator==( const std::vector<eckit::linalg::Triplet>& t1, const std::vector<eckit::linalg::Triplet>& t2 ) {
    if ( t1.size() != t2.size() ) return false;
    for ( idx_t i = 0; i < t1.size(); ++i ) {
        if ( t1[i] != t2[i] ) return false;
    }
    return true;
}
std::ostream& operator<<( std::ostream& out, const array::LocalView<double, 1>& array ) {
    out << "{ ";
    for ( idx_t i = 0; i < array.size(); ++i ) {
        out << array( i );
        if ( i < array.size() - 1 ) { out << ","; }
        out << " ";
    }
    out << "}";
    return out;
}

//-----------------------------------------------------------------------------

CASE( "test horizontal cubic interpolation triplets" ) {
    //if ( mpi::comm().size() == 1 ) {
    std::string gridname = eckit::Resource<std::string>( "--grid", "O8" );

    grid::StructuredGrid grid( gridname );
    int halo = eckit::Resource<int>( "--halo", 2 );
    util::Config config;
    config.set( "halo", halo );
    config.set( "levels", 9 );
    config.set( "periodic_points", true );
    functionspace::StructuredColumns fs( grid, grid::Partitioner( "equal_regions" ), config );

    Field field = fs.createField<double>( option::levels( 0 ) );
    auto f      = array::make_view<double, 1>( field );
    auto xy     = array::make_view<double, 2>( fs.xy() );

    for ( idx_t j = 0; j < fs.size(); ++j ) {
        f( j ) = xy( j, XX );
    }

    CubicStructuredInterpolation cubic_interpolation( fs );

    auto departure_points = functionspace::PointCloud{PointXY(),
                                                      {
                                                          {0.13257, 45.6397},
                                                          {360., -90.},
                                                      }};
    auto departure_lonlat = array::make_view<double, 2>( departure_points.lonlat() );

    CubicStructuredInterpolation::WorkSpace ws;
    CubicStructuredInterpolation::Triplets triplets;
    for ( idx_t row = 0; row < departure_points.size(); ++row ) {
        auto triplets_row =
            cubic_interpolation.compute_triplets( row, departure_lonlat( row, XX ), departure_lonlat( row, YY ), ws );
        Log::info() << departure_lonlat.slice( row, array::Range::all() ) << "  -->  {\n";
        for ( auto triplet : triplets_row ) {
            Log::info() << "    " << triplet << " ,\n";
        }
        Log::info() << " } " << std::endl;
        std::copy( triplets_row.begin(), triplets_row.end(), std::back_inserter( triplets ) );
    }

    auto triplets2 = cubic_interpolation.reserve_triplets( departure_points.size() );
    {
        idx_t row{0};
        for ( auto p : departure_points.iterate().xy() ) {
            cubic_interpolation.insert_triplets( row++, p.x(), p.y(), triplets2, ws );
        }
    }


    EXPECT( triplets2 == triplets );

    eckit::linalg::SparseMatrix matrix( departure_points.size(), fs.size(), triplets2 );
    Log::info() << matrix << std::endl;

    std::vector<double> tgt( departure_points.size() );
    eckit::linalg::Vector v_src( const_cast<double*>( f.data() ), f.size() );
    eckit::linalg::Vector v_tgt( tgt.data(), tgt.size() );
    eckit::linalg::LinearAlgebra::backend().spmv( matrix, v_src, v_tgt );
    Log::info() << "output = " << tgt << std::endl;
}

//-----------------------------------------------------------------------------

CASE( "ifs method to find nearest grid point" ) {
    // see satrad/module/gaussgrid.F90
    std::string gridname = eckit::Resource<std::string>( "--grid", "O8" );
    grid::StructuredGrid grid( gridname );

    auto p = PointXY{0., grid.y( 0 )};
    idx_t kgrib_lat, kgrib_lon;
    {
        double x = p.x();
        double y = p.y();
        std::vector<double> pdlat( grid.ny() );
        for ( idx_t j = 0; j < grid.ny(); ++j ) {
            pdlat[j] = std::abs( y - grid.y( j ) );
        }

        auto iterator = std::min_element( pdlat.begin(), pdlat.end() );
        kgrib_lat     = ( iterator - pdlat.begin() );

        double zfirstlon = grid.x( 0, kgrib_lat );
        double zdlon     = grid.x( 1, kgrib_lat ) - zfirstlon;
        double zsafelon  = std::fmod( x - zfirstlon + 720., 360. );
        kgrib_lon        = std::fmod( std::round( zsafelon / zdlon ), grid.nx( kgrib_lat ) );
    }
    EXPECT( kgrib_lon == 0 );
    EXPECT( kgrib_lat == 0 );
}

//-----------------------------------------------------------------------------

CASE( "test 3d cubic interpolation" ) {
    //if ( mpi::comm().size() == 1 ) {
    std::string gridname = eckit::Resource<std::string>( "--grid", "O8" );
    idx_t nlev           = 11;

    Vertical vertical( nlev, zrange( nlev, 0., 1. ), util::Config( "boundaries", false ) );
    Log::info() << zrange( nlev, 0., 1. ) << std::endl;
    ;

    grid::StructuredGrid grid( gridname );
    int halo = eckit::Resource<int>( "--halo", 2 );
    util::Config config;
    config.set( "halo", halo );
    config.set( "periodic_points", true );
    config.set( "levels", vertical.size() );
    functionspace::StructuredColumns fs( grid, grid::Partitioner( "equal_regions" ), config );

    Field field = fs.createField<double>();
    auto f      = array::make_view<double, 2>( field );
    auto xy     = array::make_view<double, 2>( fs.xy() );

    auto fx   = []( double x ) { return cubic( x, 0., 360. ); };
    auto fy   = []( double y ) { return cubic( y, -90., 90. ); };
    auto fz   = []( double z ) { return cubic( z, 0., 1. ); };
    auto fxyz = [fx, fy, fz]( double x, double y, double z ) { return fx( x ) * fy( y ) * fz( z ); };
    for ( idx_t n = 0; n < fs.size(); ++n ) {
        for ( idx_t k = vertical.k_begin(); k < vertical.k_end(); ++k ) {
            double x  = xy( n, XX );
            double y  = xy( n, YY );
            double z  = vertical( k );
            f( n, k ) = fxyz( x, y, z );
        }
    }

    Cubic3DInterpolation cubic_interpolation( fs, vertical );

    auto departure_points = functionspace::PointCloud(
        PointXYZ(), {
                        {90., -45., 0.25}, {0., -45., 0.25},  {180., -45., 0.25}, {360., -45., 0.25}, {90., -90., 0.25},
                        {90., 0., 0.25},   {90., 90., 0.25},  {10., -45., 0.25},  {20., -45., 0.25},  {30., -45., 0.25},
                        {40., -45., 0.25}, {50., -45., 0.25}, {60., -45., 0.25},  {70., -45., 0.25},  {80., -45., 0.25},
                        {90., -45., 0.25}, {10., -10., 0.25}, {20., -20., 0.25},  {30., -30., 0.25},  {40., -40., 0.25},
                        {50., -50., 0.25}, {60., -60., 0.25}, {70., -70., 0.25},  {80., -80., 0.25},  {90., -90., 0.25},
                        {60., -60., 0.6},  {90., -45., 0.16}, {90., -45., 0.6},   {90., -45., 1.},    {90., -45., 0.},
                        {90., -45., 0.1},
                    } );

    for ( auto p : departure_points.iterate().xyz() ) {
        double interpolated = cubic_interpolation( p.x(), p.y(), p.z(), f );
        double exact        = fxyz( p.x(), p.y(), p.z() );
        Log::info() << p << "  -->  " << interpolated << std::endl;
        EXPECT( eckit::types::is_approximately_equal( interpolated, exact ) );
    }
}

}  // namespace test
}  // namespace atlas

int main( int argc, char** argv ) {
    return atlas::test::run( argc, argv );
}
