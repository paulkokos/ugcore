/*
 * Copyright (c) 2011-2015:  G-CSC, Goethe University Frankfurt
 * Author: Martin Rupp
 * 
 * This file is part of UG4.
 * 
 * UG4 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (2) The following notice must be displayed at a prominent place in the
 * terminal output of covered works: "Based on UG4 (www.ug4.org/license)".
 * 
 * (3) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S., Vogel, A., Heppner, I., Rupp, M., and Wittum, G. A massively
 *   parallel geometric multigrid solver on hierarchically distributed grids.
 *   Computing and visualization in science 16, 4 (2013), 151-164"
 * "Vogel, A., Reiter, S., Rupp, M., Nägel, A., and Wittum, G. UG4 -- a novel
 *   flexible software system for simulating pde based models on high performance
 *   computers. Computing and visualization in science 16, 4 (2013), 165-179"
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#ifndef __H__UG__LIB_ALGEBRA__ALGEBRA_TYPE__
#define __H__UG__LIB_ALGEBRA__ALGEBRA_TYPE__

#include <ostream>

namespace ug{

/// \addtogroup lib_algebra lib_algebra
/// \{

////////////////////////////////////////////////////////////////////////////////
//   Algebra Types
////////////////////////////////////////////////////////////////////////////////

/// class describing the type of an algebra
class AlgebraType
{
	public:
	///	types of algebra
		enum Type
		{
			CPU = 0,
			GPU = 1
		};

	///	indicating variable block size
		enum {VariableBlockSize = -1};

	public:
	///	empty constructor
		AlgebraType();

	///	constructor for fix blocksize
		AlgebraType(Type type, int blockSize);

	///	constructor for fix blocksize
		AlgebraType(const char* type, int blockSize);

	///	constructor for variable block size
		AlgebraType(const char* type);

	///	returns the type
		int type() const {return m_type;}

	///	returns the blocksize
		int blocksize() const {return m_blockSize;}

	protected:
		int m_type;
		int m_blockSize;
};

/// writes the Identifier to the output stream
std::ostream& operator<<(std::ostream& out,	const AlgebraType& v);


/// Singleton, providing the current default algebra.
class DefaultAlgebra
{
	public:
		static AlgebraType get() {return m_default;}
		static void set(const AlgebraType& defaultType) {m_default = defaultType;}

	protected:
		static AlgebraType m_default;
};

// end group lib_algebra
/// \}

} // end namespace ug

#endif /* __H__UG__LIB_ALGEBRA__ALGEBRA_TYPE__ */
