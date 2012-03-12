/*
 * cuthill_mckee.cpp
 *
 *  Created on: 21.03.2011
 *      Author: andreasvogel
 */

#include "common/common.h"
#include "cuthill_mckee.h"
#include <algorithm>
#include <vector>
#include <queue>

namespace ug{

/// help class to provide compare operator for indices based on their degree
/**
 * This class is used to provide an ordering for indices. The ordering relation
 * is based on the connectivity-degree, i.e. on the number of connections the
 * index has. The indices with less connections are ordered first.
 */
struct CompareDegree {
///	constructor, passing field with connections for each index
	CompareDegree(const std::vector<std::vector<size_t> >& vInfo) : m_vCon(vInfo) {}

///	comparison operator
	bool operator() (size_t i,size_t j)
	{
		UG_ASSERT(i < m_vCon.size(), "Invalid index.");
		UG_ASSERT(j < m_vCon.size(), "Invalid index.");
		return (m_vCon[i].size() < m_vCon[j].size());
	}

private:
///	storage field of connections of each index
	const std::vector<std::vector<size_t> >& m_vCon;
};

// computes ordering using Cuthill-McKee algorithm
void ComputeCuthillMcKeeOrder(std::vector<size_t>& vNewIndex,
                              std::vector<std::vector<size_t> >& vvConnection,
                              bool bReverse)
{
	PROFILE_FUNC();
//	list of sorted (will be filled later)
	std::vector<size_t> vNewOrder; vNewOrder.clear();

//	create flag list to remember already handled indices
	std::vector<bool> vHandled(vvConnection.size(), false);

//	number of indices, that must be sorted
	size_t numToSort = 0;

//	Sort neighbours by degree (i.e. by number of neighbours those have)
	CompareDegree myCompDegree(vvConnection);
	for(size_t i = 0; i < vvConnection.size(); ++i)
	{
	//	indices with no adjacent indices are marked as handled (and skipped)
		if(vvConnection[i].size() == 0)
		{
			vHandled[i] = true;
			continue;
		}

	//	sort adjacent index by degree
		std::sort(	vvConnection[i].begin(),
		          	vvConnection[i].end(), myCompDegree);

	//	this index must be sorted
		numToSort++;
	}

//	start with first index
	size_t start = 0;

//	handle all indices
	while(true)
	{
	//	find first unhandled index
		size_t i_start = start;
		for(i_start = start; i_start < vHandled.size(); ++i_start)
		{
			if(vHandled[i_start] == false) {start = i_start; break;}
		}

	//	check if one unhandled vertex left
		if(i_start == vHandled.size()) break;

	//	Find node with smallest degree for all remaining indices
		for(size_t i = start; i < vHandled.size(); ++i)
		{
			if(vHandled[i] == false &&
				vvConnection[i].size() < vvConnection[start].size())
				start = i;
		}

	//	Add start vertex to mapping
		vNewOrder.push_back(start);
		vHandled[start] = true;

	//	Create queue of adjacent vertices
		std::queue<size_t> qAdjacent;
		for(size_t i = 0; i < vvConnection[start].size(); ++i)
		{
			if(vvConnection[start][i] != start)
				qAdjacent.push(vvConnection[start][i]);
		}

	//	add adjacent vertices to mapping
		while(!qAdjacent.empty())
		{
		//	get next index
			const size_t front = qAdjacent.front();

		//	if not handled
			if(vHandled[front] == false)
			{
			//	Add to mapping
				vNewOrder.push_back(front);
				vHandled[front] = true;

			//	add adjacent to queue
				for(size_t i = 0; i < vvConnection[front].size(); ++i)
				{
					const size_t ind = vvConnection[front][i];

					if(vHandled[ind] == false && ind != front)
						qAdjacent.push(ind);
				}
			}

		//	pop index
			qAdjacent.pop();
		}
	}

//	number of sorted indices
	const size_t numSorted = vNewOrder.size();

//	check, that number of sorted indices is correct
	if(numSorted != numToSort)
		UG_THROW_FATAL("OrderCuthillMcKee: Must sort "<<numToSort<<" indices,"
				" but "<<numSorted<<" indices sorted.\n");

// 	Create list of mapping
	vNewIndex.clear(); vNewIndex.resize(vvConnection.size(), (size_t)-1);

//	write new indices into out array
	size_t cnt = 0;
	if(bReverse)
	{
		for(size_t oldInd = 0; oldInd < vvConnection.size(); ++oldInd)
		{
		//	skip non-sorted indices
			if(vvConnection[oldInd].size() == 0) continue;

		//	get old index
			const size_t newInd = vNewOrder[numSorted - 1 - cnt]; ++cnt;

		//	set new index to order
			vNewIndex[newInd] = oldInd;
		}
	}
	else
	{
		for(size_t oldInd = 0; oldInd < vvConnection.size(); ++oldInd)
		{
		//	skip non-sorted indices
			if(vvConnection[oldInd].size() == 0) continue;

		//	get old index
			const size_t newInd = vNewOrder[cnt++];

		//	set new index to order
			vNewIndex[newInd] = oldInd;
		}
	}

//	check if all ordered indices have been written
	if(cnt != vNewOrder.size())
		UG_THROW_FATAL("OrderCuthillMcKee: "
					   "Not all ordered indices written back.\n");

//	fill non-sorted indices
	for(size_t i = 1; i < vNewIndex.size(); ++i)
	{
		if(vNewIndex[i] == (size_t)-1) vNewIndex[i] = vNewIndex[i-1] + 1;
	}
}



}
