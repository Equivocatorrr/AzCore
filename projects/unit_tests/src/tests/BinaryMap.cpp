/*
	File: BinaryMap.cpp
	Author: Philip Haynes
	Unit test to verify the behavior of AzCore::BinaryMap,
	Copied from BinaryMap.cpp and modified for Map.
*/

#include "../UnitTests.hpp"
#include "AzCore/Memory/BinaryMap.hpp"
#include "AzCore/Memory/Array.hpp"
#include "AzCore/math.hpp"
#include "AzCore/IO/Log.hpp"

namespace BinaryMapTestNamespace {

void BinaryMapTest();
UT::Register binaryMap("BinaryMap", BinaryMapTest);

using Node = az::BinaryMap<i32, i32>::Node;

i32 Depth(Node *node, i32 maxDepth=100) {
	if (maxDepth <= 0) return 10000;
	if (nullptr == node) return 0;
	i32 depth = max(Depth((Node*)node->left, maxDepth-1), Depth((Node*)node->right, maxDepth-1)) + 1;
	return depth;
}

#define CHECK_DEPTH(node, expectedDepth) \
	{ \
		i32 depth1 = Depth(map1.node); \
		i32 depth2 = Depth(map2.node); \
		UTAssert(depth1 < 10000, "There's probably a circular chain here"); \
		UTExpect(depth1 == expectedDepth, "Expected depth to be ", expectedDepth, " but it was ", depth2); \
		UTAssert(depth2 < 10000, "There's probably a circular chain here"); \
		UTExpect(depth2 == expectedDepth, "Expected depth to be ", expectedDepth, " but it was ", depth2); \
	}

#define CHECK_NODE_KEY(node, expectedKey) \
	{ \
		UTAssert(nullptr != (map1.node)); \
		UTExpect((map1.node)->key == expectedKey, "Expected key to be ", expectedKey, " but it was ", (map1.node)->key); \
		UTAssert(nullptr != (map2.node)); \
		UTExpect((map2.node)->key == expectedKey, "Expected key to be ", expectedKey, " but it was ", (map2.node)->key); \
	}

#define EMPLACE(key, value) \
	map1.Emplace(key, value); \
	map2.ValueOf(key) = value;

void BinaryMapTest() {
	az::BinaryMap<i32, i32> map1, map2;
	UTExpect(nullptr == map1.base, "A newly-constructed map should have no nodes.");
	UTExpect(nullptr == map2.base, "A newly-constructed map should have no nodes.");

	EMPLACE(0, 1); // EMPLACE(0, 1);
	EMPLACE(2, 2); // EMPLACE(2, 2);
	EMPLACE(1, 3); // EMPLACE(1, 3);
	UTExpect(map1.Exists(0));
	UTExpect(map1.Exists(1));
	UTExpect(map1.Exists(2));
	UTExpect(!map1.Exists(-1));
	UTExpect(!map1.Exists(3));
	UTExpectEquals(map1[0], 1);
	UTExpectEquals(map1[1], 3);
	UTExpectEquals(map1[2], 2);
	/* Tree should rotate nodes like so:
	    0         1
	      2  to  0 2
	     1
	*/
	CHECK_DEPTH(base, 2);
	CHECK_NODE_KEY(base, 1);
	CHECK_NODE_KEY(base->left, 0);
	CHECK_NODE_KEY(base->right, 2);
	UTExpectEquals(map1.base->depthDiff, 0);
	UTExpectEquals(map1.base->depthDiff, 0);
	UTExpectEquals(map1.base->left->depthDiff, 0);
	UTExpectEquals(map2.base->left->depthDiff, 0);
	UTExpectEquals(map2.base->right->depthDiff, 0);
	UTExpectEquals(map2.base->right->depthDiff, 0);
	// TODO: Write the same tests the other way around

	map1.Clear();
	map2.Clear();

	EMPLACE(1, 1);
	EMPLACE(0, 2);
	EMPLACE(4, 3);
	EMPLACE(2, 4);
	EMPLACE(5, 5);
	EMPLACE(3, 6);
	UTExpect(map1.Exists(0));
	UTExpect(map1.Exists(1));
	UTExpect(map1.Exists(2));
	UTExpect(map1.Exists(3));
	UTExpect(map1.Exists(4));
	UTExpect(map1.Exists(5));
	UTExpectEquals(map1[0], 2);
	UTExpectEquals(map1[1], 1);
	UTExpectEquals(map1[2], 4);
	UTExpectEquals(map1[3], 6);
	UTExpectEquals(map1[4], 3);
	UTExpectEquals(map1[5], 5);
	UTExpect(!map1.Exists(-1));
	UTExpect(!map1.Exists(6));
	/* Tree should rotate nodes like so:
	      1               2
	   0     4    to    1   4
	       2   5       0   3 5
	        3
	*/
	CHECK_DEPTH(base, 3);
	CHECK_NODE_KEY(base, 2);
	UTExpectEquals(map1.base->depthDiff, 0);
	UTExpectEquals(map2.base->depthDiff, 0);
	CHECK_NODE_KEY(base->left, 1);
	UTExpectEquals(map1.base->left->depthDiff, -1);
	UTExpectEquals(map2.base->left->depthDiff, -1);
	CHECK_NODE_KEY(base->left->left, 0);
	UTExpectEquals(map1.base->left->left->depthDiff, 0);
	UTExpectEquals(map2.base->left->left->depthDiff, 0);
	CHECK_NODE_KEY(base->right, 4);
	UTExpectEquals(map1.base->right->depthDiff, 0);
	UTExpectEquals(map2.base->right->depthDiff, 0);
	CHECK_NODE_KEY(base->right->left, 3);
	UTExpectEquals(map1.base->right->left->depthDiff, 0);
	UTExpectEquals(map2.base->right->left->depthDiff, 0);
	CHECK_NODE_KEY(base->right->right, 5);
	UTExpectEquals(map1.base->right->right->depthDiff, 0);
	UTExpectEquals(map2.base->right->right->depthDiff, 0);

	map1.Clear();
	map2.Clear();


	EMPLACE(0, 1);
	UTAssert(map1.base != nullptr);
	UTExpect(map1.Exists(0));
	UTAssert(map2.base != nullptr);
	UTExpect(map2.Exists(0));
	EMPLACE(1, 2);
	EMPLACE(2, 3);
	/* Tree should rotate nodes like so:
		0         1
		 1   to  0 2
		  2
	*/
	CHECK_DEPTH(base, 2);
	CHECK_NODE_KEY(base, 1);
	CHECK_NODE_KEY(base->left, 0);
	CHECK_NODE_KEY(base->right, 2);
	EMPLACE(3, 4);
	EMPLACE(4, 5);
	/* Tree should rotate nodes like so:
		 1          1
		0 2   to  0   3
		   3         2 4
		    4
	*/
	CHECK_DEPTH(base, 3);
	CHECK_NODE_KEY(base, 1);
	CHECK_NODE_KEY(base->left, 0);
	CHECK_NODE_KEY(base->right, 3);
	CHECK_NODE_KEY(base->right->left, 2);
	CHECK_NODE_KEY(base->right->right, 4);
	EMPLACE(-2, 6);
	EMPLACE(-1, 7);
	/* Tree should rotate nodes like so:
	      1            1
	    0   3   to  -1   3
	  -2   2 4     -2 0 2 4
	   -1
	*/
	CHECK_DEPTH(base, 3);
	CHECK_NODE_KEY(base->left, -1);
	CHECK_NODE_KEY(base->left->left, -2);
	CHECK_NODE_KEY(base->left->right, 0);
	EMPLACE(6, 8);
	EMPLACE(5, 9);
	/* Tree should rotate nodes like so:
	        1                1
	    -1     3   to    -1     3
	  -2   0 2   4     -2   0 2   5
	               6             4 6
	              5
	*/
	CHECK_DEPTH(base, 4);
	CHECK_NODE_KEY(base, 1);
	CHECK_NODE_KEY(base->left, -1);
	CHECK_NODE_KEY(base->left->left, -2);
	CHECK_NODE_KEY(base->left->right, 0);
	CHECK_NODE_KEY(base->right, 3);
	CHECK_NODE_KEY(base->right->left, 2);
	CHECK_NODE_KEY(base->right->right, 5);
	CHECK_NODE_KEY(base->right->right->left, 4);
	CHECK_NODE_KEY(base->right->right->right, 6);
	EMPLACE(-3, 10);
	EMPLACE(-4, 11);
	/* Tree should rotate nodes like so:
	            1                      1
	        -1     3             -1        3
	      -2   0 2   5  to    -3     0   2   5
	    -3          4 6     -4  -2          4 6
	  -4
	*/
	CHECK_DEPTH(base, 4);
	CHECK_NODE_KEY(base, 1);
	CHECK_NODE_KEY(base->left, -1);
	CHECK_NODE_KEY(base->left->left, -3);
	CHECK_NODE_KEY(base->left->left->left, -4);
	CHECK_NODE_KEY(base->left->left->right, -2);
	CHECK_NODE_KEY(base->left->right, 0);
	CHECK_NODE_KEY(base->right, 3);
	CHECK_NODE_KEY(base->right->left, 2);
	CHECK_NODE_KEY(base->right->right, 5);
	CHECK_NODE_KEY(base->right->right->left, 4);
	CHECK_NODE_KEY(base->right->right->right, 6);
}

} // namespace BinaryMapTestNamespace