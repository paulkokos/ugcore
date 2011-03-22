#include "./p1conform.h"
#include <vector>
#include <queue>
#include <algorithm>


namespace ug{

///////////////////////////////////////
// P1StorageManager
///////////////////////////////////////

void
P1StorageManager::
set_subset_handler(ISubsetHandler& sh)
{
	if(m_pSH != NULL) clear();

	m_pSH = &sh;
	m_pSH->enable_subset_attachments(true);
}

void
P1StorageManager::
clear_subset_handler()
{
	if(m_pSH != NULL) clear();
	m_pSH->enable_subset_attachments(false);
	m_pSH = NULL;
}

void
P1StorageManager::
clear()
{
	if(m_pSH == NULL) return;

	for(size_t si = 0; si < m_vSubsetInfo.size(); ++si)
	{
		m_pSH->detach_from<VertexBase>(m_vSubsetInfo[si].aDoF, si);
		m_vSubsetInfo[si].aaDoFVRT.invalidate();
	}
	m_vSubsetInfo.clear();
}

void
P1StorageManager::
update_attachments()
{
	if(m_pSH == NULL)
	{
		UG_LOG("WARNING: Updating indices, but no SubsetHandler set.\n");
		return;
	}

	size_t num_subsets =  m_pSH->num_subsets();

	// Create level dof distributors
	for(size_t si = m_vSubsetInfo.size(); si < num_subsets; ++si)
	{
		m_vSubsetInfo.push_back(SubsetInfo());
		m_pSH->attach_to<VertexBase>(m_vSubsetInfo[si].aDoF, si);
		m_vSubsetInfo[si].aaDoFVRT.access(*m_pSH, m_vSubsetInfo[si].aDoF, si);
	}
}



///////////////////////////////////////
// P1ConformDoFDistribution
///////////////////////////////////////

size_t
P1ConformDoFDistribution::
num_indices(ReferenceObjectID refID, int si, const FunctionGroup& funcGroup) const
{
	const ReferenceElement& refElem =
			ReferenceElementFactory::get_reference_element(refID);

	size_t numFct = 0;
	for(size_t fct = 0; fct < funcGroup.num_fct(); ++fct)
	{
		if(is_def_in_subset(funcGroup[fct], si))
			numFct++;
	}

	return numFct * refElem.num_obj(0);
}

bool
P1ConformDoFDistribution::
prepare_indices(ReferenceObjectID refID, int si,
                LocalIndices& ind, bool withHanging) const
{
	const ReferenceElement& refElem =
			ReferenceElementFactory::get_reference_element(refID);

	if(!withHanging)
	{
		ind.clear();
		size_t numInd = 0;
		size_t numFct = 0;
		for(size_t fct = 0; fct < ind.num_fct(); ++fct)
		{
			if(!is_def_in_subset(ind.unique_id(fct), si)) continue;
			for(size_t dof = 0; dof < refElem.num_obj(0); ++dof)
			{
				LocalIndices::multi_index_type dof_ind;
				dof_ind[0] = dof + numFct * refElem.num_obj(0);
				dof_ind[1] = 0;
				ind.add_dof(fct, dof_ind);
			}
			numInd += refElem.num_obj(0);
			numFct++;
		}
		ind.set_num_indices(numInd);
	}
	return true;
}


size_t
P1ConformDoFDistribution::
num_inner_indices(ReferenceObjectID refID, int si,
                  const FunctionGroup& funcGroup) const
{
	if(refID != ROID_VERTEX) return 0;
	else return num_indices(refID, si, funcGroup);
}

bool
P1ConformDoFDistribution::
prepare_inner_indices(ReferenceObjectID refID, int si, LocalIndices& ind) const
{
	ind.clear();
	if(refID != ROID_VERTEX) return true;
	else return prepare_indices(refID, si, ind);
}

///////////// creation /////////////////

bool
P1ConformDoFDistribution::
distribute_dofs()
{
	if(m_pStorageManager == NULL)
	{
		UG_LOG("In 'P1ConformDoFDistribution::distribute_dofs:"
				"Storage Manager not set. Aborting.\n");
		return false;
	}

	if(m_pFuncPattern == NULL)
	{
		UG_LOG("In 'P1ConformDoFDistribution::distribute_dofs:"
				"Function Pattern not set. Aborting.\n");
		return false;
	}

// 	Attach indices
	m_pStorageManager->update_attachments();

// 	iterators
	geometry_traits<VertexBase>::iterator iter, iterBegin, iterEnd;

// 	reset counter for all dofs
	m_numDoFs = 0;

// 	reset number of dofs
	m_vNumDoFs.clear(); m_vNumDoFs.resize(num_subsets(), 0);
	m_vvOffsets.clear(); m_vvOffsets.resize(num_subsets());

// 	loop subsets
	for(int si = 0; si < num_subsets(); ++si)
	{
	// 	create offsets
		size_t count = 0;
		m_vvOffsets[si].resize(num_fct());
		for(size_t fct = 0; fct < num_fct(); ++fct)
		{
			if(!is_def_in_subset(fct, si)) m_vvOffsets[si][fct] = (size_t) -1;
			else m_vvOffsets[si][fct] = count++;
		}

	// 	skip if no dofs to be distributed
		if(!(num_fct(si)>0)) continue;

		iterBegin = this->begin<VertexBase>(si);
		iterEnd =  this->end<VertexBase>(si);

	// 	remember number of functions
		const size_t numFct = num_fct(si);

	// 	loop Vertices
		m_vNumDoFs[si] = 0;
		for(iter = iterBegin; iter != iterEnd; ++iter)
		{
		// 	get vertex
			VertexBase* vrt = *iter;

		//	skip shadows
			if(m_pSurfaceView != NULL)
				if(m_pSurfaceView->is_shadow(vrt))
					continue;

		// 	write index
			first_index(vrt, si) = m_numDoFs;

		//	increase number of DoFs
			m_numDoFs += numFct;

		//	increase number of dofs on subset
			m_vNumDoFs[si] += numFct;
		}
	}

//	post process shadows.
	if(m_pSurfaceView != NULL)
	{
		for(int si = 0; si < num_subsets(); ++si)
		{

			iterBegin = this->begin<VertexBase>(si);
			iterEnd =  this->end<VertexBase>(si);
			for(iter = iterBegin; iter != iterEnd; ++iter)
			{
			//	get vertex
				VertexBase* vrt = *iter;

			//	skip non-shadows
				if(!m_pSurfaceView->is_shadow(vrt)) continue;

			//	get child
				VertexBase* vrtChild = m_pSurfaceView->get_child(vrt);

			//	get indices of child
				const size_t indexChild = first_index(vrtChild, si);

			//	set index of shadow to index of child
				first_index(vrt, si) = indexChild;
			}
		}
	}

//	we're done
	return true;
}

bool
P1ConformDoFDistribution::
swap_indices(const std::vector<size_t>& vIndNew)
{
//	check, that storage is initialized
	if(m_pStorageManager == NULL)
	{
		UG_LOG("ERROR in 'P1ConformDoFDistribution::swap_indices':"
				" No Storage Manager");
		return false;
	}
	if(m_pISubsetHandler == NULL)
	{
		UG_LOG("ERROR in 'P1ConformDoFDistribution::swap_indices':"
				" No Subset Handler");
		return false;
	}

//	check, that passed index fields have the same size
	if(this->num_dofs() != vIndNew.size())
	{
		UG_LOG("ERROR in 'P1ConformDoFDistribution::swap_indices': New index set"
				" must have same cardinality for swap indices.\n");
		return false;
	}

// 	loop subsets
	for(int si = 0; si < num_subsets(); ++si)
	{
	//	get iterators
		geometry_traits<VertexBase>::iterator iter, iterBegin, iterEnd;
		iterBegin = this->begin<VertexBase>(si);
		iterEnd =  this->end<VertexBase>(si);

	// 	loop Vertices
		for(iter = iterBegin; iter != iterEnd; ++iter)
		{
		// 	get vertex
			VertexBase* vrt = *iter;

		// 	get current (old) index
			const size_t oldIndex = first_index(vrt, si);

		//	replace old index by new one
			first_index(vrt, si) = vIndNew[oldIndex];
		}
	}

//	we're done
	return true;
}

bool
P1ConformDoFDistribution::
get_connections(std::vector<std::vector<size_t> >& vvConnection)
{
	UG_ASSERT(m_pStorageManager != NULL, "No Storage Manager");
	UG_ASSERT(m_pISubsetHandler != NULL, "No Subset Handler");

//	clear neighbours
	vvConnection.clear(); vvConnection.resize(m_numDoFs);

//	if no subset given, we're done
	if(num_subsets() == 0) return true;

//	check that in all subsets same number of functions and at least one
	size_t numFct = num_fct(0);
	if(numFct == 0) return true;
	for(int si = 0; si < num_subsets(); ++si)
	{
		if(numFct != num_fct(si))
		{
			UG_LOG("ERROR in 'P1ConformDoFDistribution::get_algebraic_neighbours':"
					" Currently only implemented iff same number of functions"
					" in all subsets.\n");
			return true;
		}
	}

//	Adjacent Edges
	std::vector<EdgeBase*> vEdges;

// 	Grid
	Grid* grid = m_pStorageManager->m_pSH->get_assigned_grid();

// 	Iterators
	geometry_traits<VertexBase>::iterator iter, iterBegin, iterEnd;

//	Loop vertices
	for(int si = 0; si < num_subsets(); ++si)
	{
		iterBegin = this->begin<VertexBase>(si);
		iterEnd =  this->end<VertexBase>(si);

		for(iter = iterBegin; iter != iterEnd; ++iter)
		{
		// 	Get vertex
			VertexBase* vrt = *iter;

		//	skip shadows
			if(m_pSurfaceView != NULL)
				if(m_pSurfaceView->is_shadow(vrt))
					continue;

		//	get index
			const size_t index = first_index(vrt, si);

		//	always connection with itself
			vvConnection[index].push_back(index);

		//	Get Edges
			CollectEdges(vEdges, *grid, vrt);

		//	Get connections via shadow
			if(m_pSurfaceView != NULL)
			{
			//	skip if not shadowing
				if(m_pSurfaceView->shadows(vrt))
				{
				//	get parent
					VertexBase* vrtParent =
						dynamic_cast<VertexBase*>(m_pSurfaceView->get_parent(vrt));

				//	Get Edges
					if(vrtParent != NULL)
						CollectEdges(vEdges, *grid, vrtParent, false);
				}
			}

		//	Get connected indices
			for(size_t ed = 0; ed < vEdges.size(); ++ed)
			{
				for(size_t i = 0; i < vEdges[ed]->num_vertices(); ++i)
				{
				//	Get Vertices of adjacent edges
					VertexBase* vrt1 = vEdges[ed]->vertex(i);

				//	skip own vertex
					if(vrt1 == vrt) continue;

				//	get index
					const int si1 = m_pISubsetHandler->get_subset_index(vrt1);

				//	skip iff in no subset
				//  This can happen, when no subset is set from the beginning or
				//  even when a surface grid is considered with vertical
				//	copy nodes.
					if(si1 < 0) continue;

				//	get adjacent index
					const size_t adjInd = first_index(vrt1, si1);

				//	search for index in adjacend indices
					std::vector<size_t>::iterator it;
					it = find(vvConnection[index].begin(), vvConnection[index].end(),
					          adjInd);

				//	Add vertex to list of vertices
					if(it == vvConnection[index].end())
						vvConnection[index].push_back(adjInd);
				}
			}
		}
	}

//	we're done
	return true;
}

///////////////////////////////////////
// GroupedP1ConformDoFDistribution
///////////////////////////////////////


size_t
GroupedP1ConformDoFDistribution::
num_indices(ReferenceObjectID refID, int si, const FunctionGroup& funcGroup) const
{
	const ReferenceElement& refElem =
			ReferenceElementFactory::get_reference_element(refID);

	for(size_t fct = 0; fct < funcGroup.num_fct(); ++fct)
	{
		if(is_def_in_subset(funcGroup[fct], si))
			return refElem.num_obj(0);
	}

	return 0;
}


size_t
GroupedP1ConformDoFDistribution::
num_inner_indices(ReferenceObjectID refID, int si, const FunctionGroup& funcGroup) const
{
	if(refID != ROID_VERTEX) return 0;
	else return num_indices(refID, si, funcGroup);
}

bool
GroupedP1ConformDoFDistribution::
prepare_indices(ReferenceObjectID refID, int si,
                LocalIndices& ind, bool withHanging) const
{
	const ReferenceElement& refElem =
			ReferenceElementFactory::get_reference_element(refID);

	if(withHanging) throw(UGError("Not implemented"));

	ind.clear();
	size_t numInd = 0;
	for(size_t fct = 0; fct < ind.num_fct(); ++fct)
	{
		if(!is_def_in_subset(ind.unique_id(fct), si)) continue;

		for(size_t dof = 0; dof < refElem.num_obj(0); ++dof)
		{
			LocalIndices::multi_index_type dof_ind;
			dof_ind[0] = dof;
			dof_ind[1] = ind.unique_id(fct);
			ind.add_dof(fct, dof_ind);
		}
		numInd = refElem.num_obj(0);
	}
	ind.set_num_indices(numInd);
	return true;
}

bool
GroupedP1ConformDoFDistribution::
prepare_inner_indices(ReferenceObjectID refID, int si, LocalIndices& ind) const
{
	ind.clear();
	if(refID != ROID_VERTEX) return 0;
	else return prepare_indices(refID, si, ind);
}

///////// Creation /////////////

bool
GroupedP1ConformDoFDistribution::
distribute_dofs()
{
	if(m_pStorageManager == NULL)
	{
		UG_LOG("In 'P1ConformDoFDistribution::distribute_dofs:"
				"Storage Manager not set. Aborting.\n");
		return false;
	}

	if(m_pFuncPattern == NULL)
	{
		UG_LOG("In 'P1ConformDoFDistribution::distribute_dofs:"
				"Function Pattern not set. Aborting.\n");
		return false;
	}

// 	Attach indices
	m_pStorageManager->update_attachments();

// 	iterators
	geometry_traits<VertexBase>::iterator iter, iterBegin, iterEnd;

//	counters
	size_t i = 0;
	size_t i_per_subset = 0;

// 	reset number of dofs
	m_vNumDoFs.clear(); m_vNumDoFs.resize(num_subsets(), 0);
	m_vvOffsets.clear(); m_vvOffsets.resize(num_subsets());

// 	loop subsets
	for(int si = 0; si < num_subsets(); ++si)
	{
	// 	create offsets
		size_t count = 0;
		m_vvOffsets[si].resize(num_fct());
		for(size_t fct = 0; fct < num_fct(); ++fct)
		{
			if(!is_def_in_subset(fct, si)) m_vvOffsets[si][fct] = (size_t) -1;
			else m_vvOffsets[si][fct] = count++;
		}

	// 	skip if no dofs to be distributed
		if(!(num_fct(si)>0)) continue;

		iterBegin = this->begin<VertexBase>(si);
		iterEnd =  this->end<VertexBase>(si);

	// 	loop Vertices
		i_per_subset = 0;
		for(iter = iterBegin; iter != iterEnd; ++iter)
		{
		// 	get vertex
			VertexBase* vrt = *iter;

		// 	write index
			m_pStorageManager->m_vSubsetInfo[si].aaDoFVRT[vrt] = i;
			i ++;
			i_per_subset ++;
		}

	//	remember number of dofs on subset
		m_vNumDoFs[si] = i_per_subset;
	}

//	remember total number of dofs
	m_numDoFs = i;

//	we're done
	return true;
}

bool
GroupedP1ConformDoFDistribution::
swap_indices(const std::vector<size_t>& vIndNew)
{
	return false;
}

bool
GroupedP1ConformDoFDistribution::
get_connections(std::vector<std::vector<size_t> >& vvConnection)
{
	return false;
}

} // end namespace ug

