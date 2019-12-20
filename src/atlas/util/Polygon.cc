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
#include <cmath>
#include <iostream>
#include <limits>

#include "eckit/types/FloatCompare.h"

#include "atlas/array.h"
#include "atlas/domain/Domain.h"
#include "atlas/field/Field.h"
#include "atlas/runtime/Exception.h"
#include "atlas/util/CoordinateEnums.h"
#include "atlas/util/Polygon.h"

namespace atlas {
namespace util {

namespace {

//------------------------------------------------------------------------------------------------------

double cross_product_analog( const Point2& A, const Point2& B, const Point2& C ) {
    return ( A[XX] - C[XX] ) * ( B[YY] - C[YY] ) - ( A[YY] - C[YY] ) * ( B[XX] - C[XX] );
}

}  // namespace

//------------------------------------------------------------------------------------------------------

Polygon::Polygon() = default;

Polygon::Polygon( const Polygon::edge_set_t& edges ) {
    setup( edges );
}

void Polygon::setup( const Polygon::edge_set_t& edges ) {
    // get external edges by attempting to remove reversed edges, if any
    edge_set_t extEdges;
    for ( const edge_t& e : edges ) {
        if ( !extEdges.erase( e.reverse() ) ) {
            extEdges.insert( e );
        }
    }
    ATLAS_ASSERT( extEdges.size() >= 2 );

    // set one polygon cycle, by picking next edge with first node same as second
    // node of last edge
    clear();
    reserve( extEdges.size() + 1 );

    emplace_back( extEdges.begin()->first );
    for ( edge_set_t::iterator e = extEdges.begin(); e != extEdges.end() && e->first == back();
          e                      = extEdges.lower_bound( edge_t( back(), std::numeric_limits<idx_t>::min() ) ) ) {
        emplace_back( e->second );
        extEdges.erase( *e );
    }
    ATLAS_ASSERT( front() == back() );

    // exhaust remaining edges, should represent additional cycles, if any
    while ( !extEdges.empty() ) {
        operator+=( Polygon( extEdges ) );
    }
}

Polygon::operator bool() const {
    return !Polygon::empty();
}

Polygon& Polygon::operator+=( const Polygon& other ) {
    if ( empty() ) {
        return operator=( other );
    }

    // polygon can have multiple cycles, but must be connected graphs
    // Note: a 'cycle' is handled by repeating the indices, excluding (repeated)
    // last index
    ATLAS_ASSERT( other.front() == other.back() );
    const difference_type N = difference_type( other.size() ) - 1;

    container_t cycle( 2 * size_t( N ) );
    std::copy( other.begin(), other.begin() + N, cycle.begin() );
    std::copy( other.begin(), other.begin() + N, cycle.begin() + N );

    for ( const_iterator c = cycle.begin(); c != cycle.begin() + N; ++c ) {
        iterator here = std::find( begin(), end(), *c );
        if ( here != end() ) {
            insert( here, c, c + N );
            return *this;
        }
    }

    throw_AssertionFailed( "Polygon: could not merge polygons, they are not connected", Here() );
}

void Polygon::print( std::ostream& s ) const {
    char z = '{';
    for ( auto n : static_cast<const container_t&>( *this ) ) {
        s << z << n;
        z = ',';
    }
    s << '}';
}

const RectangularDomain& PartitionPolygon::inscribedDomain() const {
    static RectangularDomain inscribed;
    return inscribed;
}

//------------------------------------------------------------------------------------------------------


PolygonCoordinates::PolygonCoordinates( const Polygon& poly, const atlas::Field& coordinates,
                                        bool removeAlignedPoints ) {
    ATLAS_ASSERT( poly.size() > 2 );
    ATLAS_ASSERT( poly.front() == poly.back() );

    // Point coordinates
    // - use a bounding box to quickly discard points,
    // - except when that is above/below bounding box but poles should be included

    coordinates_.clear();
    coordinates_.reserve( poly.size() );

    auto coord      = array::make_view<double, 2>( coordinates );
    coordinatesMin_ = Point2( coord( poly[0], XX ), coord( poly[0], YY ) );
    coordinatesMax_ = coordinatesMin_;

    size_t nb_removed_points_due_to_alignment = 0;

    for ( size_t i = 0; i < poly.size(); ++i ) {
        Point2 A( coord( poly[i], XX ), coord( poly[i], YY ) );
        coordinatesMin_ = Point2::componentsMin( coordinatesMin_, A );
        coordinatesMax_ = Point2::componentsMax( coordinatesMax_, A );

        // if new point is aligned with existing edge (cross product ~= 0) make the
        // edge longer
        if ( ( coordinates_.size() >= 2 ) && removeAlignedPoints ) {
            const Point2& B = coordinates_.back();
            const Point2& C = coordinates_[coordinates_.size() - 2];
            if ( eckit::types::is_approximately_equal( 0., cross_product_analog( A, B, C ) ) ) {
                coordinates_.back() = A;
                ++nb_removed_points_due_to_alignment;
                continue;
            }
        }

        coordinates_.emplace_back( A );
    }

    ATLAS_ASSERT( coordinates_.size() == poly.size() - nb_removed_points_due_to_alignment );
}


template <typename PointContainer>
PolygonCoordinates::PolygonCoordinates( const PointContainer& points ) {
    coordinates_.assign( points.begin(), points.end() );
    ATLAS_ASSERT( coordinates_.size() > 2 );
    ATLAS_ASSERT( eckit::geometry::points_equal( coordinates_.front(), coordinates_.back() ) );

    coordinatesMin_ = coordinates_.front();
    coordinatesMax_ = coordinatesMin_;
    for ( const Point2& P : coordinates_ ) {
        coordinatesMin_ = Point2::componentsMin( coordinatesMin_, P );
        coordinatesMax_ = Point2::componentsMax( coordinatesMax_, P );
    }
}

template <typename PointContainer>
PolygonCoordinates::PolygonCoordinates( const PointContainer& points, bool removeAlignedPoints ) {
    coordinates_.clear();
    coordinates_.reserve( points.size() );

    coordinatesMin_ = Point2( std::numeric_limits<double>::max(), std::numeric_limits<double>::max() );
    coordinatesMax_ = Point2( std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest() );

    size_t nb_removed_points_due_to_alignment = 0;

    for ( size_t i = 0; i < points.size(); ++i ) {
        const Point2& A = points[i];
        coordinatesMin_ = Point2::componentsMin( coordinatesMin_, A );
        coordinatesMax_ = Point2::componentsMax( coordinatesMax_, A );

        // if new point is aligned with existing edge (cross product ~= 0) make the
        // edge longer
        if ( ( coordinates_.size() >= 2 ) && removeAlignedPoints ) {
            const Point2& B = coordinates_.back();
            const Point2& C = coordinates_[coordinates_.size() - 2];
            if ( eckit::types::is_approximately_equal( 0., cross_product_analog( A, B, C ) ) ) {
                coordinates_.back() = A;
                ++nb_removed_points_due_to_alignment;
                continue;
            }
        }
        coordinates_.emplace_back( A );
    }

    ATLAS_ASSERT( coordinates_.size() > 2 );
    ATLAS_ASSERT( eckit::geometry::points_equal( coordinates_.front(), coordinates_.back() ) );
}

template PolygonCoordinates::PolygonCoordinates( const std::vector<Point2>& );
template PolygonCoordinates::PolygonCoordinates( const std::vector<PointXY>& );
template PolygonCoordinates::PolygonCoordinates( const std::vector<PointLonLat>& );

template PolygonCoordinates::PolygonCoordinates( const std::vector<Point2>&, bool );
template PolygonCoordinates::PolygonCoordinates( const std::vector<PointXY>&, bool );
template PolygonCoordinates::PolygonCoordinates( const std::vector<PointLonLat>&, bool );


PolygonCoordinates::~PolygonCoordinates() = default;

const Point2& PolygonCoordinates::coordinatesMax() const {
    return coordinatesMax_;
}

const Point2& PolygonCoordinates::coordinatesMin() const {
    return coordinatesMin_;
}

//------------------------------------------------------------------------------------------------------

}  // namespace util
}  // namespace atlas
