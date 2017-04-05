#pragma once

#include <iostream>
#include <array>
#include "atlas/grid/detail/domain/Domain.h"

namespace atlas {
namespace grid {
namespace domain {

class RectangularDomain: public Domain {

public:

    using Interval = std::array<double,2>;

public:

static bool is_global( const Interval& x, const Interval& y, const std::string& units = "degrees" );
static bool is_zonal_band( const Interval& x, const std::string& units = "degrees" );

public:

     // constructor
    RectangularDomain( const eckit::Parametrisation& );
    RectangularDomain( const Interval& x, const Interval& y, const std::string& units );

    static std::string static_type() {return "rectangular";}
    virtual std::string type() const override { return static_type(); }

    /// Checks if the point is contained in the domain
    virtual bool contains(double x, double y) const override;

    /// Checks if the x-value is contained in the domain
    bool contains_x(double x) const { return ( xmin_tol_ <= x && x <= xmax_tol_ ); }

    /// Checks if the y-value is contained in the domain
    bool contains_y(double y) const { return ( ymin_tol_ <= y && y <= ymax_tol_ ); }

    virtual bool global() const override { return global_; }
    virtual bool empty() const  override { return (xmin_ == xmax_) or (ymin_ == ymax_); }

    virtual eckit::Properties spec() const override;

    virtual void print(std::ostream&) const override;

    virtual std::string units() const override { return units_; }

    double xmin() const { return xmin_; }
    double xmax() const { return xmax_; }
    double ymin() const { return ymin_; }
    double ymax() const { return ymax_; }

private:

    double xmin_, xmax_, ymin_, ymax_;
    double xmin_tol_, xmax_tol_, ymin_tol_, ymax_tol_;
    bool global_;
    std::string units_;

};

}  // namespace domain
}  // namespace grid
}  // namespace atlas
