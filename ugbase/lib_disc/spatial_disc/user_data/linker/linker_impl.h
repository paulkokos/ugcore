/*
 * data_linker_impl.h
 *
 *  Created on: 04.07.2011
 *      Author: andreasvogel
 */

#ifndef __H__UG__LIB_DISC__SPATIAL_DISC__DATA_LINKER_IMPL__
#define __H__UG__LIB_DISC__SPATIAL_DISC__DATA_LINKER_IMPL__

#include "linker.h"

namespace ug{

////////////////////////////////////////////////////////////////////////////////
//	Data Linker
////////////////////////////////////////////////////////////////////////////////

template <typename TImpl, typename TData, int dim>
void StdDataLinker<TImpl,TData,dim>::
operator() (TData& value,
            const MathVector<dim>& globIP,
            number time, int si) const
{
	getImpl().evaluate(value,globIP,time,si);
}

template <typename TImpl, typename TData, int dim>
void StdDataLinker<TImpl,TData,dim>::
operator()(TData vValue[],
           const MathVector<dim> vGlobIP[],
           number time, int si, const size_t nip) const
{
	for(size_t ip = 0; ip < nip; ++ip)
		getImpl().evaluate(vValue[ip],vGlobIP[ip],time,si);
}

template <typename TImpl, typename TData, int dim>
template <int refDim>
void StdDataLinker<TImpl,TData,dim>::
evaluate(TData vValue[],
					 const MathVector<dim> vGlobIP[],
					 number time, int si,
					 GeometricObject* elem,
					 const MathVector<dim> vCornerCoords[],
					 const MathVector<refDim> vLocIP[],
					 const size_t nip,
					 LocalVector* u,
					 const MathMatrix<refDim, dim>* vJT) const
{
	getImpl().template evaluate<refDim>(vValue,vGlobIP,time,si,elem,
	                                    vCornerCoords,vLocIP,nip,u,vJT);
}

template <typename TImpl, typename TData, int dim>
template <int refDim>
void StdDataLinker<TImpl,TData,dim>::eval_deriv(LocalVector* u, GeometricObject* elem,
                const MathVector<dim> vCornerCoords[], bool bDeriv){

	const number t = this->time();
	const int si = this->subset();

	std::vector<std::vector<TData> >* vvvDeriv = NULL;

	for(size_t s = 0; s < this->num_series(); ++s){

		if(bDeriv && this->m_vvvvDeriv[s].size() > 0)
			vvvDeriv = &this->m_vvvvDeriv[s][0];
		else
			vvvDeriv = NULL;

		getImpl().template eval_and_deriv<refDim>(this->values(s), this->ips(s), t, si,
		                                 elem, vCornerCoords,
		                                 this->template local_ips<refDim>(s), this->num_ip(s),
		                                 u, bDeriv, s, vvvDeriv);
	}
}

template <typename TImpl, typename TData, int dim>
void StdDataLinker<TImpl,TData,dim>::
compute(LocalVector* u, GeometricObject* elem,
        const MathVector<dim> vCornerCoords[], bool bDeriv){

	UG_ASSERT(elem->base_object_id() == this->dim_local_ips(),
	          "local ip dimension and reference element dimension mismatch.");

	switch(this->dim_local_ips()){
		case 1: eval_deriv<1>(u,elem,vCornerCoords,bDeriv); break;
		case 2: eval_deriv<2>(u,elem,vCornerCoords,bDeriv); break;
		case 3: eval_deriv<3>(u,elem,vCornerCoords,bDeriv); break;
		default: UG_THROW("StdDataLinker: Dimension not supported.");
	}
}


template <typename TImpl, typename TData, int dim>
bool StdDataLinker<TImpl,TData,dim>::requires_grid_fct() const
{
	for(size_t i = 0; i < this->m_vspICplUserData.size(); ++i)
		if(this->m_vspUserDataInfo[i]->requires_grid_fct())
			return true;
	return false;
}

template <typename TImpl, typename TData, int dim>
bool StdDataLinker<TImpl,TData,dim>::continuous() const
{
	bool bRet = true;
	for(size_t i = 0; i < this->m_vspICplUserData.size(); ++i)
		bRet &= this->m_vspUserDataInfo[i]->continuous();
	return bRet;
}

template <typename TImpl, typename TData, int dim>
bool StdDataLinker<TImpl,TData,dim>::zero_derivative() const
{
	bool bRet = true;
	for(size_t i = 0; i < m_vspICplUserData.size(); ++i)
		bRet &= m_vspICplUserData[i]->zero_derivative();
	return bRet;
}

template <typename TImpl, typename TData, int dim>
void StdDataLinker<TImpl,TData,dim>::check_setup() const
{
//	check, that all inputs are set
	for(size_t i = 0; i < num_input(); ++i)
		if(!m_vspICplUserData[i].valid())
			UG_THROW("StdDataLinker::check_setup: Input number "<<i<<" missing.");
}

template <typename TImpl, typename TData, int dim>
void StdDataLinker<TImpl,TData,dim>::
set_function_pattern(const FunctionPattern& fctPatt)
{
//	set function pattern in dependent data and collect all function groups
	std::vector<const FunctionGroup*> vFctGrp(num_input(), NULL);
	for(size_t i = 0; i < m_vspICplUserData.size(); ++i){
		if(m_vspICplUserData[i].valid()){
			m_vspUserDataInfo[i]->set_function_pattern(fctPatt);
			vFctGrp[i] = &(m_vspUserDataInfo[i]->function_group());
		}
	}

//	All data this linker depends on has now an updated function group. We can
//	now setup the map of this data. Therefore, we create a union of all function
//	this linker depends on and compute maps between this common function group
//	and the the function needed by the data.

//	create union of all function groups
	try{
		this->m_fctGrp.set_function_pattern(fctPatt);
		CreateUnionOfFunctionGroups(this->m_fctGrp, vFctGrp, true);
	}UG_CATCH_THROW("'StdDataLinker::set_function_pattern': Cannot create"
					" common function group.");

	try{
		CreateFunctionIndexMapping(this->m_map, this->m_fctGrp, *this->m_fctGrp.function_pattern());
	}UG_CATCH_THROW("'StdDataLinker::set_function_pattern':"
					"Cannot create Function Index Mapping for Common Functions.");

//	create FunctionIndexMapping for each Disc
	m_vMap.resize(vFctGrp.size());
	for(size_t i = 0; i < vFctGrp.size(); ++i)
	{
		if(vFctGrp[i] != NULL)
		{
			try{
				CreateFunctionIndexMapping(m_vMap[i], *vFctGrp[i], this->m_fctGrp);
			}UG_CATCH_THROW("'StdDataLinker::set_function_pattern':"
							"Cannot create Function Index Mapping for input "<<i<<".");
		}
	}
}

template <typename TImpl, typename TData, int dim>
void StdDataLinker<TImpl,TData,dim>::
local_ip_series_added(const size_t seriesID)
{
	const size_t s = seriesID;

//	 we need a series id for all inputs
	m_vvSeriesID.resize(m_vspICplUserData.size());

//	loop inputs
	for(size_t i = 0; i < m_vspICplUserData.size(); ++i)
	{
	//	check unset data
		UG_ASSERT(m_vspICplUserData[i].valid(), "No Input set, but requested.");

	//	resize series ids
		m_vvSeriesID[i].resize(s+1);

	//	request local ips for series at input data
		switch(this->dim_local_ips())
		{
			case 1:
				m_vvSeriesID[i][s] =
						m_vspICplUserData[i]->template register_local_ip_series<1>
								(this->template local_ips<1>(s), this->num_ip(s),
								 this->m_vMayChange[s]);
				break;
			case 2:
				m_vvSeriesID[i][s] =
						m_vspICplUserData[i]->template register_local_ip_series<2>
								(this->template local_ips<2>(s), this->num_ip(s),
								 this->m_vMayChange[s]);
				break;
			case 3:
				m_vvSeriesID[i][s] =
						m_vspICplUserData[i]->template register_local_ip_series<3>
								(this->template local_ips<3>(s), this->num_ip(s),
								 this->m_vMayChange[s]);
				break;
			default: UG_THROW("Dimension not supported."); break;
		}
	}

//	resize data fields
	DependentUserData<TData, dim>::local_ip_series_added(seriesID);
}


template <typename TImpl, typename TData, int dim>
void StdDataLinker<TImpl,TData,dim>::
local_ips_changed(const size_t seriesID, const size_t newNumIP)
{
	const size_t s = seriesID;

//	loop inputs
	for(size_t i = 0; i < m_vspICplUserData.size(); ++i)
	{
	//	skip unset data
		UG_ASSERT(m_vspICplUserData[i].valid(), "No Input set, but requested.");

		switch(this->dim_local_ips())
		{
			case 1: m_vspICplUserData[i]->template set_local_ips<1>
					(m_vvSeriesID[i][s], this->template local_ips<1>(s), this->num_ip(s));
				break;
			case 2: m_vspICplUserData[i]->template set_local_ips<2>
					(m_vvSeriesID[i][s], this->template local_ips<2>(s), this->num_ip(s));
				break;
			case 3: m_vspICplUserData[i]->template set_local_ips<3>
					(m_vvSeriesID[i][s], this->template local_ips<3>(s), this->num_ip(s));
				break;
			default: UG_THROW("Dimension not supported."); break;
		}
	}

//	resize data fields
	DependentUserData<TData, dim>::local_ips_changed(seriesID, newNumIP);
}

template <typename TImpl, typename TData, int dim>
void StdDataLinker<TImpl,TData,dim>::
global_ips_changed(const size_t seriesID, const MathVector<dim>* vPos, const size_t numIP)
{
//	loop inputs
	for(size_t i = 0; i < m_vspICplUserData.size(); ++i)
	{
	//	skip unset data
		UG_ASSERT(m_vspICplUserData[i].valid(), "No Input set, but requested.");

	//	adjust global ids of imported data
		m_vspICplUserData[i]->set_global_ips(m_vvSeriesID[i][seriesID], vPos, numIP);
	}
}

} // end namespace ug

#endif /* __H__UG__LIB_DISC__SPATIAL_DISC__DATA_LINKER_IMPL__ */