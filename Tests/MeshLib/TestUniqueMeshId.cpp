/**
 * \copyright
 * Copyright (c) 2013, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/LICENSE.txt
 */

#include <ctime>
#include "gtest/gtest.h"

#include "Mesh.h"

using namespace MeshLib;

TEST(MeshLib, UniqueMeshId)
{
	// Create first mesh
	Mesh m0("first", std::vector<Node*>(), std::vector<Element*>());
	ASSERT_EQ(std::size_t(1), m0.getID());

	Mesh* m1 = new Mesh("second", std::vector<Node*>(), std::vector<Element*>());
	ASSERT_EQ(std::size_t(2), m1->getID());

	Mesh m2("third", std::vector<Node*>(), std::vector<Element*>());
	ASSERT_EQ(std::size_t(3), m2.getID());

	delete m1;
	ASSERT_EQ(std::size_t(3), m2.getID());

	Mesh m3("fourth", std::vector<Node*>(), std::vector<Element*>());
	ASSERT_EQ(std::size_t(4), m3.getID());

	// Copy mesh keeps also increments the counter.
	Mesh m4(m0);
	ASSERT_EQ(std::size_t(5), m4.getID());

}
