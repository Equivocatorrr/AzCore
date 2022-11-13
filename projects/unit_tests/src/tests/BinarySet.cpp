/*
	File: BinarySet.cpp
	Author: Philip Haynes
	Unit test to verify the behavior of AzCore::BinarySet
*/

#include "../UnitTests.hpp"
#include "AzCore/Memory/BinarySet.hpp"
#include "AzCore/Memory/Array.hpp"
#include "AzCore/math.hpp"
#include "AzCore/IO/Log.hpp"

namespace BinarySetTestNamespace {

void BinarySetTest();
UT::Register binarySet("BinarySet", BinarySetTest);

using Node = az::BinarySet<i32>::Node;

i32 Depth(Node *node, i32 maxDepth=100) {
	if (maxDepth <= 0) return 10000;
	if (nullptr == node) return 0;
	i32 depth = max(Depth(node->left, maxDepth-1), Depth(node->right, maxDepth-1)) + 1;
	return depth;
}

#define CHECK_DEPTH(node, expectedDepth) \
	{ \
		i32 depth = Depth(node); \
		UTAssert(depth < 10000, "There's probably a circular chain here"); \
		UTExpect(depth == expectedDepth, "Expected depth to be ", expectedDepth, " but it was ", depth); \
	}

#define CHECK_NODE_KEY(node, expectedKey) \
	{ \
		UTAssert(nullptr != (node)); \
		UTExpect((node)->key == expectedKey, "Expected key to be ", expectedKey, " but it was ", (node)->key); \
	}

void BinarySetTest() {
	UT::TestInfo &test = *UT::currentTestInfo;

	az::BinarySet<i32> set;
	UTExpect(nullptr == set.base, "A newly-constructed set should have no nodes.");

	set.Emplace(0);
	set.Emplace(2);
	set.Emplace(1);
	/* Set should rotate nodes like so:
	    0         1
	      2  to  0 2
	     1
	*/
	CHECK_DEPTH(set.base, 2);
	CHECK_NODE_KEY(set.base, 1);
	CHECK_NODE_KEY(set.base->left, 0);
	CHECK_NODE_KEY(set.base->right, 2);
	UTExpectEquals(set.base->depthDiff, 0);
	UTExpectEquals(set.base->left->depthDiff, 0);
	UTExpectEquals(set.base->right->depthDiff, 0);
	// TODO: Write the same tests the other way around

	set.Clear();

	set.Emplace(1);
	set.Emplace(0);
	set.Emplace(4);
	set.Emplace(2);
	set.Emplace(5);
	set.Emplace(3);
	/* Set should rotate nodes like so:
	      1               2
	   0     4    to    1   4
	       2   5       0   3 5
	        3
	*/
	CHECK_DEPTH(set.base, 3);
	CHECK_NODE_KEY(set.base, 2);
	UTExpectEquals(set.base->depthDiff, 0);
	CHECK_NODE_KEY(set.base->left, 1);
	UTExpectEquals(set.base->left->depthDiff, -1);
	CHECK_NODE_KEY(set.base->left->left, 0);
	UTExpectEquals(set.base->left->left->depthDiff, 0);
	CHECK_NODE_KEY(set.base->right, 4);
	UTExpectEquals(set.base->right->depthDiff, 0);
	CHECK_NODE_KEY(set.base->right->left, 3);
	UTExpectEquals(set.base->right->left->depthDiff, 0);
	CHECK_NODE_KEY(set.base->right->right, 5);
	UTExpectEquals(set.base->right->right->depthDiff, 0);

	set.Clear();

	set.Emplace(0);
	UTAssert(set.base != nullptr);
	UTExpect(set.Exists(0));
	set.Emplace(1);
	set.Emplace(2);
	/* Set should rotate nodes like so:
		0         1
		 1   to  0 2
		  2
	*/
	CHECK_DEPTH(set.base, 2);
	CHECK_NODE_KEY(set.base, 1);
	CHECK_NODE_KEY(set.base->left, 0);
	CHECK_NODE_KEY(set.base->right, 2);
	set.Emplace(3);
	set.Emplace(4);
	/* Set should rotate nodes like so:
		 1          1
		0 2   to  0   3
		   3         2 4
		    4
	*/
	CHECK_DEPTH(set.base, 3);
	CHECK_NODE_KEY(set.base, 1);
	CHECK_NODE_KEY(set.base->left, 0);
	CHECK_NODE_KEY(set.base->right, 3);
	CHECK_NODE_KEY(set.base->right->left, 2);
	CHECK_NODE_KEY(set.base->right->right, 4);
	set.Emplace(-2);
	set.Emplace(-1);
	/* Set should rotate nodes like so:
	      1            1
	    0   3   to  -1   3
	  -2   2 4     -2 0 2 4
	   -1
	*/
	CHECK_DEPTH(set.base, 3);
	CHECK_NODE_KEY(set.base->left, -1);
	CHECK_NODE_KEY(set.base->left->left, -2);
	CHECK_NODE_KEY(set.base->left->right, 0);
	set.Emplace(6);
	set.Emplace(5);
	/* Set should rotate nodes like so:
	        1                1
	    -1     3   to    -1     3
	  -2   0 2   4     -2   0 2   5
	               6             4 6
	              5
	*/
	CHECK_DEPTH(set.base, 4);
	CHECK_NODE_KEY(set.base, 1);
	CHECK_NODE_KEY(set.base->left, -1);
	CHECK_NODE_KEY(set.base->left->left, -2);
	CHECK_NODE_KEY(set.base->left->right, 0);
	CHECK_NODE_KEY(set.base->right, 3);
	CHECK_NODE_KEY(set.base->right->left, 2);
	CHECK_NODE_KEY(set.base->right->right, 5);
	CHECK_NODE_KEY(set.base->right->right->left, 4);
	CHECK_NODE_KEY(set.base->right->right->right, 6);
	set.Emplace(-3);
	set.Emplace(-4);
	/* Set should rotate nodes like so:
	            1                      1
	        -1     3             -1        3
	      -2   0 2   5  to    -3     0   2   5
	    -3          4 6     -4  -2          4 6
	  -4
	*/
	CHECK_DEPTH(set.base, 4);
	CHECK_NODE_KEY(set.base, 1);
	CHECK_NODE_KEY(set.base->left, -1);
	CHECK_NODE_KEY(set.base->left->left, -3);
	CHECK_NODE_KEY(set.base->left->left->left, -4);
	CHECK_NODE_KEY(set.base->left->left->right, -2);
	CHECK_NODE_KEY(set.base->left->right, 0);
	CHECK_NODE_KEY(set.base->right, 3);
	CHECK_NODE_KEY(set.base->right->left, 2);
	CHECK_NODE_KEY(set.base->right->right, 5);
	CHECK_NODE_KEY(set.base->right->right->left, 4);
	CHECK_NODE_KEY(set.base->right->right->right, 6);
}

} // namespace BinarySetTestNamespace