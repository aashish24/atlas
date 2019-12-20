/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

/// @file Unstructured.h
/// @author Willem Deconinck
/// @author Tiago Quintino
/// @author Pedro Maciel
/// @date January 2015

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "atlas/grid/detail/grid/Grid.h"
#include "atlas/runtime/Exception.h"
#include "atlas/util/Point.h"

namespace atlas {
class Mesh;
}

namespace atlas {
namespace grid {
namespace detail {
namespace grid {

class Unstructured : public Grid {
public:
    template <typename Base, typename Derived>
    class UnstructuredIterator : public Base {
    public:
        UnstructuredIterator( const Unstructured& grid, bool begin = true ) :
            grid_( grid ),
            size_( static_cast<idx_t>( grid_.points_->size() ) ),
            n_( begin ? 0 : size_ ),
            derived_( static_cast<Derived&>( *this ) ) {}

        virtual bool next( typename Base::value_type& point ) {
            if ( n_ < size_ ) {
                derived_.compute_value( n_, point );
                ++n_;
                return true;
            }
            else {
                return false;
            }
        }

        virtual typename Base::reference operator*() const { return derived_.get_reference( n_ ); }

        virtual const Base& operator++() {
            ++n_;
            derived_.update_value( n_ );
            return *this;
        }


        virtual const Base& operator+=( typename Base::difference_type distance ) {
            n_ += distance;
            derived_.update_value( n_ );
            return *this;
        }

        virtual bool operator==( const Base& other ) const { return n_ == static_cast<const Derived&>( other ).n_; }

        virtual bool operator!=( const Base& other ) const { return n_ != static_cast<const Derived&>( other ).n_; }

        virtual typename Base::difference_type distance( const Base& other ) const {
            const auto& _other = static_cast<const Derived&>( other );
            return _other.n_ - n_;
        }

        virtual std::unique_ptr<Base> clone() const {
            auto result = new Derived( grid_, false );
            result->n_  = n_;
            result->update_value( n_ );
            return std::unique_ptr<Base>( result );
        }

    protected:
        const Unstructured& grid_;
        idx_t size_;
        idx_t n_;
        Derived& derived_;
    };

    class IteratorXY : public UnstructuredIterator<Grid::IteratorXY, IteratorXY> {
    public:
        using Base = UnstructuredIterator<Grid::IteratorXY, IteratorXY>;
        using Base::Base;
        void update_value( idx_t n ) {}
        void compute_value( idx_t n, value_type& point ) { grid_.xy( n, point.data() ); }
        reference get_reference( idx_t n ) { return grid_.xy( n ); }
    };

    class IteratorLonLat : public UnstructuredIterator<Grid::IteratorLonLat, IteratorLonLat> {
    public:
        using Base = UnstructuredIterator<Grid::IteratorLonLat, IteratorLonLat>;
        IteratorLonLat( const Unstructured& grid, bool begin = true ) : Base( grid, begin ) { update_value( n_ ); }

        void update_value( idx_t n ) {
            if ( n < size_ ) {
                grid_.lonlat( n, point_.data() );
            }
        }
        void compute_value( idx_t n, value_type& point ) { grid_.lonlat( n, point.data() ); }
        reference get_reference( idx_t n ) { return point_; }
        value_type point_;
    };

public:  // methods
    static std::string static_type() { return "unstructured"; }
    virtual std::string name() const override;
    virtual std::string type() const override { return static_type(); }

    /// Constructor converting any Grid with domain to an unstructured grid
    Unstructured( const Grid&, Domain );

    /// Constructor taking a list of parameters
    Unstructured( const Config& );

    /// Constructor taking a list of points (takes ownership)
    Unstructured( std::vector<PointXY>* pts );

    /// Constructor taking a list of points (takes ownership)
    Unstructured( std::vector<PointXY>&& pts );

    /// Constructor taking a list of points (makes copy)
    Unstructured( const std::vector<PointXY>& pts );

    /// Constructor taking a mesh
    Unstructured( const Mesh& m );

    /// Constructor from initializer list
    Unstructured( std::initializer_list<PointXY> );

    virtual ~Unstructured() override;

    virtual idx_t size() const override;

    virtual Spec spec() const override;

    const PointXY& xy( idx_t n ) const { return ( *points_ )[n]; }

    PointLonLat lonlat( idx_t n ) const { return projection_.lonlat( ( *points_ )[n] ); }

    void xy( idx_t n, double crd[] ) const {
        PointXY& p = ( *points_ )[n];
        crd[0]     = p[0];
        crd[1]     = p[1];
    }

    void lonlat( idx_t n, double crd[] ) const {
        xy( n, crd );
        projection_.xy2lonlat( crd );
    }

    virtual std::unique_ptr<Grid::IteratorXY> xy_begin() const override {
        return std::unique_ptr<Grid::IteratorXY>( new IteratorXY( *this ) );
    }
    virtual std::unique_ptr<Grid::IteratorXY> xy_end() const override {
        return std::unique_ptr<Grid::IteratorXY>( new IteratorXY( *this, false ) );
    }
    virtual std::unique_ptr<Grid::IteratorLonLat> lonlat_begin() const override {
        return std::unique_ptr<Grid::IteratorLonLat>( new IteratorLonLat( *this ) );
    }
    virtual std::unique_ptr<Grid::IteratorLonLat> lonlat_end() const override {
        return std::unique_ptr<Grid::IteratorLonLat>( new IteratorLonLat( *this, false ) );
    }

private:  // methods
    virtual void print( std::ostream& ) const override;

    /// Hash of the lonlat array
    virtual void hash( eckit::Hash& ) const override;

    /// @return parallel/meridian limits containing the grid
    virtual RectangularLonLatDomain lonlatBoundingBox() const override;

protected:
    /// Storage of coordinate points
    std::unique_ptr<std::vector<PointXY>> points_;

    /// Cache for the shortName
    mutable std::string shortName_;

    /// Cache for the spec since may be quite heavy to compute
    mutable std::unique_ptr<Grid::Spec> cached_spec_;
};

extern "C" {
const Unstructured* atlas__grid__Unstructured__points( const double lonlat[], int shapef[], int stridesf[] );
const Unstructured* atlas__grid__Unstructured__config( util::Config* conf );
}


}  // namespace grid
}  // namespace detail
}  // namespace grid
}  // namespace atlas
