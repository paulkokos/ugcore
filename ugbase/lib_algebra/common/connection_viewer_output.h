/*
 * connection_viewer_output.h
 *
 *  Created on: 03.08.2011
 *      Author: mrupp
 */

#ifndef CONNECTIONVIEWEROUTPUT_H_
#define CONNECTIONVIEWEROUTPUT_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#ifdef UG_PARALLEL
#include "pcl/pcl.h"
#endif

namespace ug
{

#define CONNECTION_VIEWER_VERSION 1

#ifdef UG_PARALLEL
/**
 * extends the filename (add p000X extension in parallel) and writes a parallel pvec/pmat "header" file
 */
std::string GetParallelName(std::string name, const pcl::ProcessCommunicator &pc, bool bWriteHeader=true);

inline std::string GetParallelName(std::string name)
{
	pcl::ProcessCommunicator pc;
	return GetParallelName(name, pc);

}
template<typename T>
inline std::string GetParallelName(T &t, std::string name)
{
	return GetParallelName(name, t.process_communicator());
}

#endif

// WriteMatrixToConnectionViewer
//--------------------------------------------------
/**
 * \brief writes to a file in somewhat SparseMatrix-market format (for connection viewer)
 * \param filename Filename to write matrix to
 * \param A SparseMatrix A.
 * \param positions Positions, there has to be one position for each i in (0, ..., max(A.num_rows(), A.num_cols())).
 * \param dimensions Dimension of positions
 */
template<typename Matrix_type, typename postype>
void WriteMatrixToConnectionViewer(std::string filename, const Matrix_type &A, postype *positions, int dimensions)
{
#ifdef UG_PARALLEL
	if(pcl::GetNumProcesses() > 1)
		filename = GetParallelName(A, filename);
#endif

	std::fstream file(filename.c_str(), std::ios::out);
	file << CONNECTION_VIEWER_VERSION << std::endl;
	file << dimensions << std::endl;

	size_t rows = A.num_rows();

	// write positions
	file << rows << std::endl;
	if(dimensions == 1)
			for(size_t i=0; i < rows; i++)
				file << positions[i][0] << " 0.0"  << std::endl;
	else if(dimensions == 2)
		for(size_t i=0; i < rows; i++)
			file << positions[i][0] << " " << positions[i][1] << std::endl;
	else
		for(size_t i=0; i < rows; i++)
		  file << positions[i][0] << " " << positions[i][1] << " " << positions[i][2] << std::endl;

	file << 1 << std::endl; // show all cons
	// write connections
	for(size_t i=0; i < rows; i++)
	{
		for(typename Matrix_type::const_row_iterator conn = A.begin_row(i); conn != A.end_row(i); ++conn)
			if(conn.value() != 0.0)
				file << i << " " << conn.index() << " " << conn.value() <<		std::endl;
			else
				file << i << " " << conn.index() << " 0" <<	std::endl;
	}
}

// WriteMatrixToConnectionViewer
//--------------------------------------------------
/**
 * this version can handle different from and to spaces
 */
template <typename Matrix_type, typename postype>
bool WriteMatrixToConnectionViewer(	std::string filename,
									const Matrix_type &A,
									std::vector<postype> &positionsFrom, std::vector<postype> &positionsTo, size_t dimensions)
{
#ifdef UG_PARALLEL
	if(pcl::GetNumProcesses() > 1)
		filename = GetParallelName(A, filename);
#endif

	/*const char * p = strstr(filename, ".mat");
	if(p == NULL)
	{
		UG_LOG("Currently only '.mat' format supported for domains.\n");
		return false;
	}*/

	if(positionsFrom.size() != A.num_cols())
	{
		UG_LOG("uFrom.size() != A.num_cols() !\n");
		return false;
	}
	if(positionsTo.size() != A.num_rows())
	{
		UG_LOG("uTo.size() != A.num_rows() !\n");
		return false;
	}

	std::vector<postype> positions;
	std::vector<size_t> mapFrom, mapTo;
	mapFrom.resize(positionsFrom.size());
	mapTo.resize(positionsTo.size());

	if(positionsFrom.size() > positionsTo.size())
	{
		positions.resize(positionsFrom.size());
		for(size_t i=0; i<positionsFrom.size(); i++)
		{
			positions[i] = positionsFrom[i];
			mapFrom[i] = i;
		}


		for(size_t i=0; i<positionsTo.size(); i++)
		{
			size_t j;
			for(j=0; j<positionsFrom.size(); j++)
			{
				if(positionsTo[i] == positionsFrom[j])
					break;
			}
			mapTo[i] = j;
			if(j == positionsFrom.size())
				positions.push_back(positionsTo[i]);
		}
	}
	else
	{
		positions.resize(positionsTo.size());
		for(size_t i=0; i<positionsTo.size(); i++)
		{
			positions[i] = positionsTo[i];
			mapTo[i] = i;
		}

		for(size_t  i=0; i<positionsFrom.size(); i++)
		{
			size_t j;
			for(j=0; j<positionsTo.size(); j++)
			{
				if(positionsFrom[i] == positionsTo[j])
					break;
			}
			mapFrom[i] = j;
			if(j == positionsTo.size())
				positions.push_back(positionsFrom[i]);
		}
	}


	std::fstream file(filename.c_str(), std::ios::out);
	file << CONNECTION_VIEWER_VERSION << std::endl;
	file << dimensions << std::endl;

	// write positions
	file << positions.size() << std::endl;

	if(dimensions == 1)
		for(size_t i=0; i < positions.size(); i++)
			file << positions[i][0] << " 0.0"  << std::endl;
	else if(dimensions == 2)

		for(size_t i=0; i < positions.size(); i++)
			file << positions[i][0] << " " << positions[i][1] << std::endl;
	else
		for(size_t i=0; i < positions.size(); i++)
		  file << positions[i][0] << " " << positions[i][1] << " " << positions[i][2] << std::endl;

	file << 1 << std::endl; // show all cons
	// write connections
	for(size_t i=0; i < A.num_rows(); i++)
	{
		for(typename Matrix_type::const_row_iterator conn = A.begin_row(i); conn != A.end_row(i); ++conn)
			if(conn.value() != 0.0)
				file << mapTo[i] << " " << mapFrom[conn.index()] << " " << conn.value() <<		std::endl;
			else
				file << mapTo[i] << " " << mapFrom[conn.index()] << " 0" << std::endl;
	}
	return true;
}


// WriteVectorToConnectionViewer
//--------------------------------------------------
/**
 * \brief writes to a file in somewhat SparseMatrix-market format (for connection viewer)
 * \param filename Filename to write matrix to
 * \param b Vector
 * \param positions Positions, there has to be one position for each i in (0, ..., max(A.num_rows(), A.num_cols())).
 * \param dimensions	Dimensions of Positions
 */
template<typename Vector_type, typename postype>
void WriteVectorToConnectionViewer(std::string filename, const Vector_type &b, postype *positions, int dimensions)
{
#ifdef UG_PARALLEL
	if(pcl::GetNumProcesses() > 1)
		filename = GetParallelName(filename);
#endif
	std::fstream file(filename.c_str(), std::ios::out);
	file << CONNECTION_VIEWER_VERSION << std::endl;
	file << dimensions << std::endl;

	size_t rows = b.size();
	// write positions
	file << rows << std::endl;

	if(dimensions == 1)
		for(size_t i=0; i < rows; i++)
			file << positions[i][0] << " 0.0"  << std::endl;
	else if(dimensions == 2)
		for(size_t i=0; i < rows; i++)
			file << positions[i][0] << " " << positions[i][1] << std::endl;
	else
		for(size_t i=0; i < rows; i++)
		  file << positions[i][0] << " " << positions[i][1] << " " << positions[i][2] << std::endl;

	file << 1 << std::endl; // show all cons
	// write connections
	for(size_t i=0; i < rows; i++)
	{
		file << i << " " << i << " " << b[i] <<		std::endl;
	}
}


template<typename Matrix_type, typename Vector_type, typename postype>
void WriteVectorToConnectionViewer(std::string filename, const Matrix_type &A, const Vector_type &v,
		postype *positions, int dimensions, const Vector_type *compareVec=NULL)
{
	if(dimensions != 2)
	{
		UG_LOG(__FILE__ << ":" << __LINE__ << " WriteVectorToConnectionViewer: only dimension=2 supported");
		return;
	}
#ifdef UG_PARALLEL
	if(pcl::GetNumProcesses() > 1)
		filename = GetParallelName(A, filename);
#endif

	std::fstream file(filename.c_str(), std::ios::out);
	file << CONNECTION_VIEWER_VERSION << std::endl;
	file << dimensions << std::endl;

	size_t rows = A.num_rows();

	// write positions
	file << rows << std::endl;

		for(size_t i=0; i < rows; i++)
			file << positions[i][0] << " " << positions[i][1] << std::endl;



	file << 1 << std::endl; // show all cons
	// write connections
	for(size_t i=0; i < rows; i++)
	{
		for(typename Matrix_type::const_row_iterator conn = A.begin_row(i); conn != A.end_row(i); ++conn)
			if(conn.value() != 0.0)
				file << i << " " << conn.index() << " " << conn.value() <<		std::endl;
			else
				file << i << " " << conn.index() << " 0" <<	std::endl;
	}


	std::string nameValues = filename;
	nameValues.resize(nameValues.find_last_of("."));
	nameValues.append(".values");
	file << "v " << nameValues << "\n";

	std::fstream fileValues(nameValues.c_str(), std::ios::out);
	if(compareVec == NULL)
	{
		for(size_t i=0; i < rows; i++)
			fileValues << i << " " << v[i] <<	"\n";
	}
	else
	{
		typename Vector_type::value_type t;
		for(size_t i=0; i < rows; i++)
		{
			t = v[i];
			t -= (*compareVec)[i];
			fileValues << i << " " << t <<	"\n";
		}
	}

}

#if 0
template<typename Vector_type, typename postype>
void WriteVectorToConnectionViewerNEW(std::string filename, const Vector_type &b, postype *positions, int dimensions)
{
#ifdef UG_PARALLEL
	if(pcl::GetNumProcesses() > 1)
		filename = GetParallelName(filename);
#endif

	std::fstream file(filename.c_str(), std::ios::out);
	file << CONNECTION_VIEWER_VERSION << std::endl;
	file << 3 << std::endl;

	double nmax=0;
	for(size_t i=0; i<b.size(); i++)
		if(nmax < BlockNorm(b[i])) nmax = BlockNorm(b[i]);

	nmax*=4;
	double scale = 1.0/nmax;
	size_t rows = b.size();
	// write positions
	file << rows << std::endl;
	if(dimensions == 1)
		for(size_t i=0; i < rows; i++)
			file << positions[i][0] << " 0.0" << std::endl;
	else if(dimensions == 2)
		for(size_t i=0; i < rows; i++)
			file << positions[i][0] << " " << positions[i][1] << " " << b[i] * scale << std::endl;
	else
		for(size_t i=0; i < rows; i++)
		  file << positions[i][0] << " " << positions[i][1] << " " << positions[i][2] << std::endl;

	file << 1 << std::endl; // show all cons
	// write connections
	for(size_t i=0; i < rows; i++)
	{
		file << i << " " << i << " " << b[i] <<		std::endl;
	}
}
#endif

} // namespace ug
#endif /* CONNECTIONVIEWEROUTPUT_H_ */
