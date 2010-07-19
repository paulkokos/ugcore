/*
 * convergence_check.h
 *
 *  Created on: 18.07.2010
 *      Author: andreasvogel
 *
 *  Comment: Based on an idea by Arne Naegel
 */

#ifndef __H__LIB_DISCRETIZATION__OPERATOR__CONVERGENCE_CHECK__
#define __H__LIB_DISCRETIZATION__OPERATOR__CONVERGENCE_CHECK__

#include <ostream>
#include <string>

#include "common/common.h"

namespace ug{

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Convergence check
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

/** ConvergenceCheck
 *
 * This is the base class for a convergence checking object. An instance is
 * passed to an iterative solver to control the convergence.
 *
 */
template <typename TFunction>
class ConvergenceCheck
{
	public:
		// function type this convergence check can be used for
		typedef TFunction function_type;

	public:
		///////////////////
		// defect control
		///////////////////

		/// starts iteration with the given defect
		virtual void start(function_type& d) = 0;

		/// update for the current defect
		virtual void update(function_type& d) = 0;

		/** iteration_ended
		 *
		 *	Checks if the iteration must be ended.
		 *	This can be due to convergence or divergence.
		 *
		 * \return 	true 		if iteration ended
		 * 			false 		if iteration can and must be continued until convergence
		 */
		virtual bool iteration_ended() = 0;

		/** post
		 *
		 * post-processes the iteration. Some informative outputs of the status of
		 * the iteration after finishing the iteration can be placed here
		 *
		 * \return 	true 		if iteration was successful
		 * 			false 		if iteration did not lead to a satisfying result
		 */
		virtual bool post() = 0;

		/////////////////////////////////////
		// informations about current status
		/////////////////////////////////////

		/// returns the current defect
		virtual number defect() = 0;

		/// returns the current number of steps
		virtual int step() = 0;

		// returns the current relative reduction
		virtual number reduction() = 0;

		////////////////
		// output style
		////////////////

		/// sets the number of spaces printed before output information
		virtual void set_offset(int offset) = 0;

		/// sets the symbol used for output
		virtual void set_symbol(char symbol) = 0;

		/// sets the name of the iteration
		virtual void set_name(std::string name) = 0;


		/// virtual destructor
		virtual ~ConvergenceCheck() {};
};


///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Standard convergence check
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

/** StdConvCheck
 *
 * This is a standard implementation of the convergence check.
 * The function_type must provide the following function:
 *  - number two_norm()  --- giving the two norm of the function
 */
template <typename TFunction>
class StandardConvCheck : public ConvergenceCheck<TFunction>
{
	public:
		typedef TFunction function_type;

	public:
		StandardConvCheck(int maxSteps, number minDefect, number relReduction, bool verbose)
		 :	 m_initialDefect(0.0), m_currentDefect(0.0), m_lastDefect(0.0), m_currentStep(0),
			 m_maxSteps(maxSteps), m_minDefect(minDefect), m_relReduction(relReduction),
			 m_verbose(verbose), m_offset(0), m_symbol('%'), m_name("Iteration")
			 {};

		void start(function_type& d)
		{
			m_initialDefect = d.two_norm();
			m_currentDefect = m_initialDefect;
			m_currentStep = 0;

			if(m_verbose)
			{
				UG_LOG("\n");
				print_offset(); for(int i = 0; i < 10; ++i) UG_LOG(m_symbol);
				UG_LOG("  "<< m_name << "  ");
				for(int i = 0; i < 10; ++i) UG_LOG(m_symbol); UG_LOG("\n");
				print_offset(); UG_LOG("  Iter      Defect         Rate \n");
				print_offset(); UG_LOG(std::setw(4) << step() << ":    " << std::scientific << defect() <<  "      -------\n");
			}
		}

		void update(function_type& d)
		{
			m_lastDefect = m_currentDefect;
			m_currentDefect = d.two_norm();
			m_currentStep++;

			if(m_verbose)
			{
				print_offset(); UG_LOG(std::setw(4) << step() << ":    " << std::scientific << defect() <<
									"    " << defect()/m_lastDefect << "\n");
			}
		}

		bool iteration_ended()
		{
			if(!is_valid_number(m_currentDefect)) return true;
			if(step() >= m_maxSteps) return true;
			if(defect() < m_minDefect) return true;
			if(reduction() < m_relReduction) return true;
			return false;
		}

		bool post()
		{
			bool success = false;

			if(defect() < m_minDefect)
			{
				if(m_verbose)
				{
					print_offset(); UG_LOG("Absolute reduction " << m_minDefect << " reached after " << step() << " steps.\n");
				}
				success = true;
			};

			if(reduction() < m_relReduction)
			{
				if(m_verbose)
				{
					print_offset(); UG_LOG("Relative reduction " << m_relReduction << " reached after " << step() << " steps.\n");
				}
				success = true;
			};

			if(!success)
			{
				if (!is_valid_number(m_currentDefect))
					if(m_verbose)
					{
						print_offset(); UG_LOG("Current defect << " << m_currentDefect << " is not a valid number.\n");
					}

				if(step() >= m_maxSteps)
					if(m_verbose)
					{
						print_offset(); UG_LOG("Maximum numbers of "<< m_maxSteps << " iterations reached without convergence.\n");
					};
			}

			if(m_verbose)
			{
				print_offset(); for(int i = 0; i < 5; ++i) UG_LOG(m_symbol);
				if(success) {UG_LOG("  Iteration converged  ");}
				else {UG_LOG("  Iteration not successful  ");}
				for(int i = 0; i < 5; ++i) UG_LOG(m_symbol); UG_LOG("\n\n");
			}
			return success;
		}

		number reduction() {return m_currentDefect/m_initialDefect;};
		number defect() {return m_currentDefect;};
		int step() {return m_currentStep;}

		void set_offset(int offset){m_offset = offset;}
		void set_symbol(char symbol){m_symbol = symbol;}
		void set_name(std::string name) {m_name = name;}

	protected:
		void print_offset()
		{
			// step 1: whitespace
			for(int i = 0; i < m_offset; ++i)	UG_LOG(" ");

			// step 2: print style character
			UG_LOG(m_symbol << " ");
		}

		bool is_valid_number(number value)
		{
			// (value >= std::numeric_limits<number>::min() ) == true if value > -infty
			// (value <= std::numeric_limits<number>::max() ) == true if value < infty
			// (value == value                         ) == true if value != NaN

			if (value == 0.0) return true;
			else return (value >= std::numeric_limits<number>::min()
						&& value <= std::numeric_limits<number>::max()
						&& value == value && value >= 0.0);
		}

	protected:
		// start defect
		number m_initialDefect;

		// current defect
		number m_currentDefect;

		// defect of the previous step
		number m_lastDefect;

		// current step
		int m_currentStep;

	protected:
		// maximum number of steps to be performed
		int m_maxSteps;

		// absolute reduction to be reached for convergence
		number m_minDefect;

		// relative reduction to be reached for convergence
		number m_relReduction;

		// verbose level
		bool m_verbose;

		// number of spaces inserted before output
		int m_offset;

		// symbol for output appearance
		char m_symbol;

		// name of iteration
		std::string m_name;
};


} // end namespace ug

#endif /* __H__LIB_DISCRETIZATION__OPERATOR__CONVERGENCE_CHECK__ */
