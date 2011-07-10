/**
 * \file densematrix_impl.h
 *
 * \author Martin Rupp
 *
 * \date 01.11.2010
 *
 * Goethe-Center for Scientific Computing 2010.
 */

#ifndef __H__UG__LIB_ALGEBRA__PINVIT_H__
#define __H__UG__LIB_ALGEBRA__PINVIT_H__

// constructors
namespace ug{

template<typename T>
inline std::string ToString(const T &t)
{
	std::stringstream out;
	out << t;
	return out.str();
}

/*template<typename mat_type, typename vec_type, typename densematrix_type>
void MultiEnergyProd(const SparseMatrix<mat_type> &A,
			Vector<vec_type> **x,
			DenseMatrix<densematrix_type> &rA, size_t n)
{
	UG_ASSERT(n == rA.num_rows() && n == rA.num_cols(), "");
	vec_type Ai_xc;
	rA = 0.0;
	for(size_t i=0; i<A.num_rows(); i++)
	{
		for(size_t c=0; c<n; c++)
		{
			// Ai_xc = A[i] * x[c].
			Ai_xc = 0.0;
			A.mat_mult_add_row(i, Ai_xc, 1.0, (*x[c]));
			for(size_t r=0; r<n; r++)
				rA(c, r) += VecDot((*x[r])[i], Ai_xc);
		}
	}
}*/

template<typename vector_type, typename densematrix_type>
void MultiScalProd(vector_type **px,
			DenseMatrix<densematrix_type> &rA, size_t n)
{
	UG_ASSERT(n == rA.num_rows() && n == rA.num_cols(), "");
	for(size_t r=0; r<n; r++)
	{
		for(size_t c=r; c<n; c++)
			rA(r, c) = VecProd((*px[c]), (*px[r]));
	}

	for(size_t r=0; r<n; r++)
		for(size_t c=0; c<r; c++)
			rA(r,c) = rA(c, r);
}


template<typename matrix_type, typename vector_type>
double EnergyProd(vector_type &v1, matrix_type &A, vector_type &v2)
{
	vector_type t;
	CloneVector(t, v1);
	v2.change_storage_type(PST_CONSISTENT);
	A.apply(t, v2);
	t.change_storage_type(PST_CONSISTENT);
	return VecProd(v1, t);
}

template<typename matrix_type, typename vector_type, typename densematrix_type>
void MultiEnergyProd(matrix_type &A,
			vector_type **px,
			DenseMatrix<densematrix_type> &rA, size_t n)
{
	UG_ASSERT(n == rA.num_rows() && n == rA.num_cols(), "");
	vector_type t;
	CloneVector(t, *px[0]);

	for(size_t r=0; r<n; r++)
	{
		// todo: why is SparseMatrix<T>::apply not const ?!?
		px[r]->change_storage_type(PST_CONSISTENT);
		A.apply(t, (*px[r]));
		t.change_storage_type(PST_CONSISTENT);
		for(size_t c=r; c<n; c++)
			rA(r, c) = VecProd((*px[c]), t);
	}

	for(size_t r=0; r<n; r++)
		for(size_t c=0; c<r; c++)
			rA(r,c) = rA(c, r);
}


template<typename tvector>
void PrintStorageType(const tvector &v)
{
	if(v.has_storage_type(PST_UNDEFINED))
		UG_LOG("PST_UNDEFINED ");
	if(v.has_storage_type(PST_CONSISTENT))
		UG_LOG("PST_CONSISTENT ");
	if(v.has_storage_type(PST_ADDITIVE))
		UG_LOG("PST_ADDITIVE ");
	if(v.has_storage_type(PST_UNIQUE))
		UG_LOG("PST_UNIQUE ");
}


template<typename TAlgebra>
class PINVIT
{
public:
// 	Algebra type
	typedef TAlgebra algebra_type;

// 	Vector type
	typedef typename TAlgebra::vector_type vector_type;
	typedef typename TAlgebra::matrix_type matrix_type;

private:
	//ILinearOperator<vector_type,vector_type>* m_pA;
	//ILinearOperator<vector_type,vector_type>* m_pB;
	ILinearOperator<vector_type, vector_type> *m_pA;

	typedef typename IPreconditioner<TAlgebra>::matrix_operator_type matrix_operator_type;
	matrix_operator_type *m_pB;



	std::vector<vector_type*> px;
	ILinearIterator<vector_type, vector_type> *m_pPrecond;

	size_t m_maxIterations;
	double m_dPrecision;

public:
	PINVIT()
	{
		m_pA = NULL;
		m_pB = NULL;
	}

	void add_vector(vector_type &vec)
	{
		px.push_back(&vec);
	}

	void set_preconditioner(ILinearIterator<vector_type, vector_type> &precond)
	{
		m_pPrecond = &precond;
	}

	bool set_linear_operator_A(ILinearOperator<vector_type, vector_type> &A)
	{
		m_pA = &A;
		return true;
	}

	bool set_linear_operator_B(matrix_operator_type &B)
	{
		m_pB = &B;
		return true;
	}

	void set_max_iterations(size_t maxIterations)
	{
		m_maxIterations = maxIterations;
	}

	void set_precision(double precision)
	{
		m_dPrecision = precision;
	}

	int apply()
	{
		UG_LOG("Eigensolver\n");
		DenseMatrix<VariableArray2<double> > rA;
		DenseMatrix<VariableArray2<double> > rB;
		DenseMatrix<VariableArray2<double> > r_ev;
		DenseVector<VariableArray1<double> > r_lambda;
		std::vector<double> lambda;


		typedef typename vector_type::value_type value_type;
		vector_type defect;
		CloneVector(defect, *px[0]);
		size_t n = px.size();

		size_t size = px[0]->size();
		/*
		ParallelMatrix<SparseMatrix<double> > B;
		B.resize(size, size);
		for(size_t i=0; i<size; i++)
			B(i,i) = 0.00390625;
		B.set_storage_type(PST_ADDITIVE);*/

		matrix_operator_type &B = *m_pB;

		vector_type tmp;
		CloneVector(tmp, *px[0]);
		std::vector<vector_type> corr;
		std::vector<vector_type> oldcorr;
		lambda.resize(n);
		corr.resize(n);
		oldcorr.resize(n);
		for(size_t i=0; i<n; i++)
		{
			UG_ASSERT(px[0]->size() == px[i]->size(), "all vectors must have same size");
			CloneVector(corr[i], *px[0]);
			CloneVector(oldcorr[i], *px[0]);
		}

		std::vector<vector_type *> pTestVectors;

		std::vector<double> corrnorm(n, m_dPrecision*10);

		std::vector<std::string> testVectorDescription;

		m_pPrecond->init(*m_pA);

		for(size_t iteration=0; iteration<m_maxIterations; iteration++)
		{
			UG_LOG("iteration " << iteration << "\n");
			// 0. normalize
			if(m_pB)
			{
				for(size_t i=0; i<n; i++)
				{
					UG_LOG(i << ": " << sqrt( EnergyProd(*px[i], B, *px[i])) << " , " << (sqrt(0.00390625)*px[i]->two_norm()) << "\n");

					//(*px[i]) *= 1/ sqrt( EnergyProd(*px[i], *m_pB, *px[i]));
					(*px[i]) *= 1/ sqrt( EnergyProd(*px[i], B, *px[i]));
				}
			}
			else
			{
				for(size_t i=0; i<n; i++)
					(*px[i]) *= 1/ (sqrt(0.00390625)*px[i]->two_norm());
			}



			// 1. before calculating new correction, save old correction
			for(size_t i=0; i<n; i++)
				std::swap(oldcorr[i], corr[i]);

			//  2. compute rayleigh quotient, residuum, apply preconditioner, compute corrections norm
			size_t nrofconverged=0;

			for(size_t i=0; i<n; i++)
			{
				// 2a. compute rayleigh quotients
				// lambda = <x, Ax>/<x,x>
				// todo: replace with MatMult
//				UG_LOG("m_pA has storage type "); PrintStorageType(*m_pA); UG_LOG(", and vector px[" << i << "] has storage type"); PrintStorageType(*px[i]); UG_LOG("\n");
				// px can be set to unique because of two_norm
				px[i]->set_storage_type(PST_CONSISTENT);
				m_pA->apply(defect, *px[i]);
#ifdef UG_PARALLEL
				defect.change_storage_type(PST_UNIQUE);
				px[i]->change_storage_type(PST_UNIQUE);
#endif
				lambda[i] = VecProd(*px[i], defect); // / <px[i], px[i]> = 1.0.
				UG_LOG("lambda[" << i << "] = " << lambda[i] << "\n");

				// 2b. calculate residuum
				// defect = A px[i] - lambda[i] B px[i]
				if(m_pB)
				{
					// todo: replace with MatMultAdd
					//MatMultAddDirect(defect, 1.0, defect, -lambda[i], *m_pB, *px[i]);
					px[i]->change_storage_type(PST_CONSISTENT);
					MatMultAddDirect(defect, 1.0, defect, -lambda[i], B, *px[i]);
				}
				else
					VecScaleAdd(defect, 1.0, defect, -0.00390625*lambda[i], *px[i]);

				// 2c. check if converged

#ifdef UG_PARALLEL
				defect.change_storage_type(PST_UNIQUE);
#endif
				corrnorm[i] = defect.two_norm();
				UG_LOG("corrnorm[" << i << "] = " << corrnorm[i] << "\n");
				if(corrnorm[i] < m_dPrecision)
					nrofconverged++;
				if(corrnorm[i] < 1e-12)
					continue;
				// 2d. apply preconditioner
				m_pPrecond->apply(corr[i], defect);
#ifdef UG_PARALLEL
				corr[i].change_storage_type(PST_UNIQUE);
#endif
			}

			// output
			UG_LOG("================================================\n");
			UG_LOG("iteration " << iteration << "\n");

			for(size_t i=0; i<n; i++)
				UG_LOG(i << " lambda: " << std::setw(14) << lambda[i] << " defect: " <<
						std::setw(14) << corrnorm[i] << "\n");
			UG_LOG("\n");

			if(nrofconverged==n)
			{
				UG_LOG("all eigenvectors converged\n");
				return true;
			}

			// 5. add Testvectors

			pTestVectors.clear();
			testVectorDescription.clear();
			for(size_t i=0; i<n; i++)
			{
				pTestVectors.push_back(px[i]);
				testVectorDescription.push_back(std::string("eigenvector [") + ToString(i) + std::string("]") );
				if(corrnorm[i] < 1e-12)
					continue;
				pTestVectors.push_back(&corr[i]);
				testVectorDescription.push_back(std::string("correction [") + ToString(i) + std::string("]") );

				//pTestVectors.push_back(&oldcorr[i]);
				//testVectorDescription.push_back(string("old correction [") + ToString(i) + string("]") );
			}

			size_t iNrOfTestVectors = pTestVectors.size();

			// 5. compute reduced Matrices rA, rB
			rA.resize(iNrOfTestVectors, iNrOfTestVectors);
			rB.resize(iNrOfTestVectors, iNrOfTestVectors);

			if(m_pB)
				MultiEnergyProd(*m_pB, &pTestVectors[0], rB, iNrOfTestVectors);
			else
				MultiScalProd(&pTestVectors[0], rB, iNrOfTestVectors);
			// todo: remove doubled corrections by gauss-eliminating rB

			MultiEnergyProd(*m_pA, &pTestVectors[0], rA, iNrOfTestVectors);

			/*UG_LOG("A:\nA := matrix([");
			for(size_t r=0; r<rA.num_rows(); r++)
			{
				UG_LOG("[");
				for(size_t c=0; c<rA.num_cols(); c++)
				{
					UG_LOG(rA(r, c));
					if(c < rA.num_cols()-1) UG_LOG(", ");
				}
				UG_LOG("]");
				if(r < rA.num_rows()-1) UG_LOG(", ");
			}
			UG_LOG("]);\n");

			UG_LOG("B:\n")B := matrix([");
			for(size_t r=0; r<rB.num_rows(); r++)
			{
				UG_LOG("[");
				for(size_t c=0; c<rB.num_cols(); c++)
				{
					UG_LOG(rB(r, c));
					if(c < rB.num_cols()-1) UG_LOG(", ");
				}
				UG_LOG("]");
				if(r < rB.num_rows()-1) UG_LOG(", ");
			}
			UG_LOG("]);\n");

			cout.flush();*/
			// output

			// 6. solve reduced eigenvalue problem

			r_ev.resize(iNrOfTestVectors, iNrOfTestVectors);
			r_lambda.resize(iNrOfTestVectors);

			// solve rA x = lambda rB, --> r_ev, r_lambda
			GeneralizedEigenvalueProblem(rA, r_ev, r_lambda, rB, true);

			size_t nrzero;
			for(nrzero=0; nrzero<iNrOfTestVectors; nrzero++)
				if(r_lambda[nrzero] > 1e-15)
					break;

			if(nrzero)
			{
				UG_LOG("Lambda < 0: \n");
				for(size_t i=0; i<nrzero; i++)
					UG_LOG(i << ".: " << r_lambda[i] << "\n");
			}

			UG_LOG("Lambda > 0: \n");
			for(size_t i=nrzero; i<r_lambda.size(); i++)
				UG_LOG(i << ".: " << r_lambda[i] << "\n");

			UG_LOG("\n");


			UG_LOG("evs: \n");
			for(size_t c=nrzero; c < std::min(nrzero+n, r_ev.num_cols()); c++)
			{
				UG_LOG("ev [" << c << "]:\n");
				for(size_t r=0; r<r_ev.num_rows(); r++)
					if(dabs(r_ev(r, c)) > 1e-5 )
						UG_LOG(std::setw(14) << r_ev(r, c) << "   " << testVectorDescription[r] << "\n");
				UG_LOG("\n\n");
			}


			// assume r_lambda is sorted
			std::vector<typename vector_type::value_type> x_tmp(n);
			for(size_t i=0; i<size; i++)
			{
				// since x is part of the Testvectors, temporary safe result in x_tmp.
				for(size_t r=0; r<n; r++)
				{
					x_tmp[r] = 0.0;
					for(size_t c=0; c<iNrOfTestVectors; c++)
						x_tmp[r] += r_ev(c, r+nrzero) * (*pTestVectors[c])[i];
				}

				// now overwrite
				for(size_t r=0; r<n; r++)
					(*px[r])[i] = x_tmp[r];

			}
		}

		UG_LOG("not converged after" << m_maxIterations << " steps.\n");
		return false;
	}



};

} // namespace ug


#endif // __H__UG__LIB_ALGEBRA__PINVIT_H__
