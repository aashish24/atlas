/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */


#include "atlas/grid/Structured.h"

#include <algorithm>
#include <limits>
#include "atlas/runtime/ErrorHandling.h"
#include "eckit/geometry/Point3.h"

#include "atlas/internals/Debug.h"


namespace atlas {
namespace grid {


Structured* Structured::create(const util::Config& p) {
    Structured* grid = dynamic_cast<Structured*>(Grid::create(p));
    if (!grid)
        throw eckit::BadParameter("Grid is not a reduced grid", Here());
    return grid;

}


Structured* Structured::create(const std::string& uid) {
    Structured* grid = dynamic_cast<Structured*>( Grid::create(uid) );
    if (!grid)
        throw eckit::BadParameter("Grid "+uid+" is not a reduced grid",Here());
    return grid;
}


std::string Structured::className() {
    return "atlas.grid.Structured";
}


std::string Structured::grid_type_str() {
    return "structured";
}

std::string Structured::shortName() const {
  return "structured";

}

Structured::Structured() :
    Grid(),
    N_(0) {
}


Structured::~Structured() {
}


void Structured::setup(
    const size_t ny,
    const double y[],
    const long nx[],
    const double xmin[],
    const double xmax[] ) {
    ASSERT(ny>1);  // can't have a grid with just one latitude

    nx_  .assign(nx,   nx   + ny);
    y_   .assign(y,    y    + ny);
    xmin_.assign(xmin, xmin + ny);
    xmax_.assign(xmax, xmax + ny);
    npts_ = static_cast<size_t>(std::accumulate(nx_.begin(), nx_.end(), 0));

    dx_.resize(ny);
    nxmin_ = nxmax_ = static_cast<size_t>(nx_[0]);

    for (size_t j = 0; j < ny; ++j) {
        dx_[j] = (xmax_[j]-xmin_[j])/double(nx_[j]-1);
        nxmin_ = std::min(static_cast<size_t>(nx_[j]),nxmin_);
        nxmax_ = std::max(static_cast<size_t>(nx_[j]),nxmax_);
    }

    compute_true_periodicity();
}


void Structured::setup_cropped(const size_t ny, const double y[], const long nx[], const double xmin[], const double xmax[], const domain::Domain& dom ) {
    ASSERT(ny>0);

    std::vector<double> dom_y;    dom_y.  reserve(ny);
    std::vector<long>   dom_nx;   dom_nx.    reserve(ny);
    std::vector<double> dom_xmin; dom_xmin.reserve(ny);
    std::vector<double> dom_xmax; dom_xmax.reserve(ny);
    const double tol = 1.e-6;
    size_t dom_ny = 0;
    const double d_xmin = dom.xmin();
    const double d_xmax = dom.xmax();
    const double d_ymin = dom.ymin();
    const double d_ymax = dom.ymax();
    double dx;
    for( size_t j=0; j<ny; ++j )
    {
        if( y[j]-tol < d_ymax && y[j]+tol > d_ymin )
        {
            ++dom_ny;
            const double _y = y[j];
            double _xmin = xmin[j];
            double _xmax = xmax[j];
            if( isPeriodicX() ) {  // periodic:      nx = number of divisions
              dx = (d_xmax-d_xmin)/double(nx[j]);
            } else {            // not periodic:  nx = number of points
              if( _xmin < _xmax ) {
                dx = (_xmax-_xmin)/double(nx[j]-1l);
              } else {
                dx = (d_xmax-d_xmin)/double(nx[j]-1l);
              }
            }


// Flaw: Assumed that original grid has increments that start from xmin=0.0
//       xmin of original grid does not even have to be multiple of dx

            long _nx(0);
            for( long i=0; i<nx[j]; ++i )
            {
                const double x = dx*i;
                if( x+tol > d_xmin && x-tol < d_xmax )
                {
                    _xmin = std::min(_xmin,x);
                    _nx++;
                }
            }
            _xmax = _xmin+_nx*dx;
            dom_y     .push_back(y[j]);
            dom_nx    .push_back(_nx);
            dom_xmin.push_back(_xmin);
            dom_xmax.push_back(_xmax);
        }
    }
    setup(dom_ny,dom_y.data(),dom_nx.data(),dom_xmin.data(),dom_xmax.data());
}

void Structured::compute_true_periodicity() {
  using eckit::geometry::lonlat_to_3d;
  using eckit::geometry::Point3;
  using eckit::geometry::points_equal;

  double llxmin[] = { domain_->xmin(), 0.5*(domain_->ymin()+domain_->ymax()) };
  double llxmax[] = { domain_->xmax(), 0.5*(domain_->ymin()+domain_->ymax()) };
  double llymin[] = { 0.5*(domain_->xmin()+domain_->xmax()), domain_->ymin() };
  double llymax[] = { 0.5*(domain_->xmin()+domain_->xmax()), domain_->ymax() };

  projection_->coords2lonlat(llxmin);
  projection_->coords2lonlat(llxmax);
  projection_->coords2lonlat(llymin);
  projection_->coords2lonlat(llymax);

  double pxmin[3];
  double pxmax[3];
  double pymin[3];
  double pymax[3];

  double r=1., h=0.;
  lonlat_to_3d(llxmin,pxmin,r,h);
  lonlat_to_3d(llxmax,pxmax,r,h);
  lonlat_to_3d(llymin,pymin,r,h);
  lonlat_to_3d(llymax,pymax,r,h);

  enum{ XX=0, YY=1, ZZ=2 };
  Point3 Pxmin(pxmin[XX],pxmin[YY],pxmin[ZZ]);
  Point3 Pxmax(pxmax[XX],pxmax[YY],pxmax[ZZ]);
  Point3 Pymin(pymin[XX],pymin[YY],pymin[ZZ]);
  Point3 Pymax(pymax[XX],pymax[YY],pymax[ZZ]);

  periodic_x_ = points_equal( Pxmin, Pxmax );
  periodic_y_ = points_equal( Pymin, Pymax );
}

void Structured::lonlat( std::vector<Point>& pts ) const {
    pts.resize(npts());

    for(size_t jlat=0, c=0; jlat<nlat(); ++jlat) {
        const double y = lat(jlat);
        for(size_t jlon=0; jlon<nlon(jlat); ++jlon) {
            pts[c++].assign(lon(jlat,jlon),y);
        }
    }
}


size_t Structured::copyLonLatMemory(double* pts, size_t size) const {
    size_t sizePts = 2*npts();
    ASSERT(size >= sizePts);

    for(size_t jlat=0, c=0; jlat<nlat(); ++jlat ) {
        const double y = lat(jlat);
        for( size_t jlon=0; jlon<nlon(jlat); ++jlon ) {
            pts[c++] = lon(jlat,jlon);
            pts[c++] = y;
        }
    }
    return sizePts;
}


void Structured::print(std::ostream& os) const {
    os << "Structured(Name:" << shortName() << ")";
}


void Structured::hash(eckit::MD5& md5) const {
    // Through inheritance the grid_type_str() might differ while still being same grid
    //md5.add(grid_type_str());

    md5.add(latitudes().data(), sizeof(double)*latitudes().size());
    md5.add(pl().data(), sizeof(long)*nlat());

    // also add lonmin and lonmax
    md5.add(xmin_.data(), sizeof(double)*xmin_.size());
    md5.add(xmax_.data(), sizeof(double)*xmax_.size());

    // also add projection information
    eckit::Properties prop;
    std::ostringstream s;
    s << projection().spec();
    prop.set("projection",s.str());
    prop.hash(md5);
}

eckit::Properties Structured::spec() const {
    eckit::Properties grid_spec;

    // general specs
    grid_spec=Grid::spec();

    // specific specs
    grid_spec.set("nlat",nlat());
    grid_spec.set("latitudes",eckit::makeVectorValue(latitudes()));
    grid_spec.set("pl",eckit::makeVectorValue(pl()));
    grid_spec.set("lonmin",eckit::makeVectorValue(xmin_));
    grid_spec.set("lonmax",eckit::makeVectorValue(xmax_));

    return grid_spec;
}


// --------------------------------------------------------------------


extern "C" {


    size_t atlas__grid__Structured__N(Structured* This) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->N();
        );
        return 0;
    }


    size_t atlas__grid__Structured__nlat(Structured* This) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->nlat();
        );
        return 0;
    }


    size_t atlas__grid__Structured__nlon(Structured* This, size_t jlat) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->nlon(jlat);
        );
        return 0;
    }


    void atlas__grid__Structured__pl(Structured* This, const long* &nlons, size_t &size) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            nlons = This->pl().data();
            size  = This->pl().size();
        );
    }


    size_t atlas__grid__Structured__nlonmax(Structured* This) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->nlonmax();
        );
        return 0;
    }


    size_t atlas__grid__Structured__nlonmin(Structured* This) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->nlonmin();
        );
        return 0;
    }


    size_t atlas__grid__Structured__npts(Structured* This) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->npts();
        );
        return 0;
    }


    double atlas__grid__Structured__lat(Structured* This,size_t jlat) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->lat(jlat);
        );
        return 0.;
    }


    double atlas__grid__Structured__lon(Structured* This,size_t jlat,size_t jlon) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->lon(jlat, jlon);
        );
        return 0.;
    }


    void atlas__grid__Structured__lonlat(Structured* This, size_t jlat, size_t jlon, double crd[]) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            This->lonlat(jlat, jlon, crd);
        );
    }


    void atlas__grid__Structured__latitudes(Structured* This, const double* &lat, size_t &size) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            lat  = This->latitudes().data();
            size = This->latitudes().size();
        );
    }


    int atlas__grid__Structured__reduced(Structured* This) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
            return This->reduced();
        );
        return 1;
    }


    Structured* atlas__grid__Structured(char* identifier) {
        ATLAS_ERROR_HANDLING(
            ASSERT( identifier );
            return Structured::create( std::string(identifier) );
        );
        return 0;
    }


    Structured* atlas__grid__Structured__config(util::Config* conf) {
        ATLAS_ERROR_HANDLING(
            ASSERT( conf );
            return Structured::create(*conf);
        );
        return 0;
    }


    void atlas__grid__Structured__delete(Structured* This) {
        ATLAS_ERROR_HANDLING(
            ASSERT( This );
        );
        delete This;
    }


}


}  // namespace grid
}  // namespace atlas

