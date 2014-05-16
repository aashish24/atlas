/*
 * (C) Copyright 1996-2014 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>

#include "atlas/atlas_config.h"

#include "atlas/mesh/Mesh.hpp"
#include "atlas/meshgen/RGG.hpp"
#include "atlas/io/Gmsh.hpp"

BOOST_AUTO_TEST_SUITE( TestMeshGen3D )

BOOST_AUTO_TEST_CASE( test_create_mesh )
{
    using namespace atlas;
    using namespace atlas::meshgen;

    Mesh* m;

    RGGMeshGenerator generate;

    // generate.options.set("nb_parts",1); // default = 1
    // generate.options.set("part",    0); // default = 0

    generate.options.set("three_dimensional", true); ///< creates links along date-line
    generate.options.set("include_pole", true);      ///< triangulate the pole point

    m = generate( T159() );

    Gmsh::write(*m,"out.msh");

//    atlas::actions::BuildXYZ(m);

    delete m;
}

BOOST_AUTO_TEST_SUITE_END()