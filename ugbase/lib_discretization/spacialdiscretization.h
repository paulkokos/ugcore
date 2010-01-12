/*
 * spacialdiscretization.h
 *
 *  Created on: 22.10.2009
 *      Author: andreasvogel
 */

#ifndef __H__LIBDISCRETIZATION__SPACIALDISCRETIZATION__
#define __H__LIBDISCRETIZATION__SPACIALDISCRETIZATION__

#include "subsetdiscretization.h"
#include "numericalsolution.h"
#include "lib_algebra/lib_algebra.h"
#include <vector>
#include <string>
#include "assemble.h"

namespace ug{

template <int dim>
class SpacialDiscretization : public IAssemble<dim> {

	public:
		SpacialDiscretization();
		SpacialDiscretization(std::string name);

		void set_name(std::string name);
		std::string name();

		void add_SubsetDiscretization(SubsetDiscretization<dim>& psd);
		void delete_SubsetDiscretization(int nr);
		void clear_SubsetDiscretizations();
		int numberOfSubsetDiscretizations();

		void print_info();

		IAssembleReturn assemble_defect(Vector& d, NumericalSolution<dim>& u, number time, number s_m, number s_a, uint level = 0)
		{
			for(uint i=0; i < m_SubsetDiscretization.size(); ++i)
			{
				if(m_SubsetDiscretization[i]->assemble_defect(d, u, time, s_m, s_a) == false) return IAssemble_ERROR;
			}
			return IAssemble_OK;
		}

		IAssembleReturn assemble_jacobian(Matrix& J, NumericalSolution<dim>& u, number time, number s_m, number s_a, uint level = 0)
		{
			for(uint i=0; i < m_SubsetDiscretization.size(); ++i)
			{
				if(m_SubsetDiscretization[i]->assemble_jacobian(J, u, time, s_m, s_a)==false) return IAssemble_ERROR;
			}
			return IAssemble_OK;
		}

		IAssembleReturn assemble_jacobian_defect(Matrix& J, Vector& d, NumericalSolution<dim>& u, uint level = 0)
		{
			return IAssemble_ERROR;
		}

		IAssembleReturn assemble_defect(Vector& vec, NumericalSolution<dim>& u, uint level = 0)
		{
			for(uint i=0; i < m_SubsetDiscretization.size(); ++i)
			{
				if(m_SubsetDiscretization[i]->assemble_defect(vec, u)==false) return IAssemble_ERROR;
			}

			DirichletValues<dim> dirVal;
			if(get_dirichlet_values(u, dirVal) == false) return IAssemble_ERROR;
			dirVal.set_zero_values(vec);
			return IAssemble_OK;
		}

		IAssembleReturn assemble_jacobian(Matrix& J,  NumericalSolution<dim>& u, uint level = 0)
		{
			for(uint i=0; i < m_SubsetDiscretization.size(); ++i)
			{
				if(m_SubsetDiscretization[i]->assemble_jacobian(J, u)==false) return IAssemble_ERROR;
			}

			DirichletValues<dim> dirVal;
			if(get_dirichlet_values(u, dirVal) == false) return IAssemble_ERROR;
			dirVal.set_rows(J);
			return IAssemble_OK;
		}

		IAssembleReturn assemble_linear(Matrix& A, Vector& b, uint level = 0)
		{
			return IAssemble_ERROR;

			/*DirichletValues<dim> dirVal;
			bool control = true;

			for(uint i=0; i < m_SubsetDiscretization.size(); ++i)
			{
				//control = control && (m_SubsetDiscretization[i]->assemble_linear(A, b));
			}

			//if(get_dirichlet_values(u, dirVal) == false) return false;
			dirVal.set_values(b);
			dirVal.set_rows(A);

			return control;*/
		}

		IAssembleReturn assemble_solution(NumericalSolution<dim>& u, uint level = 0)
		{
			DirichletValues<dim> dirVal;

			if(get_dirichlet_values(u, dirVal) == false) return IAssemble_ERROR;
			dirVal.set_values(*u.GridVector());
			return IAssemble_OK;
		}

		bool get_dirichlet_values(NumericalSolution<dim>& u, DirichletValues<dim>& dirVal)
		{
			bool b = true;

			for(uint i=0; i < m_SubsetDiscretization.size(); ++i)
			{
				b = b && (m_SubsetDiscretization[i]->get_dirichlet_values(u, dirVal));
			}
			return b;
		}

	protected:
		typedef std::vector<SubsetDiscretization<dim>*> SubsetDiscretizationContainer;

	protected:
		std::string m_name;
		SubsetDiscretizationContainer m_SubsetDiscretization;

};






}



#endif /* __H__LIBDISCRETIZATION__SPACIALDISCRETIZATION__ */
