/*
 * CMinimizer_levmar.h
 *
 *  Created on: Feb 13, 2012
 *      Author: bkloppenborg
 */
 
 /* 
 * Copyright (c) 2012 Brian Kloppenborg
 *
 * If you use this software as part of a scientific publication, please cite as:
 *
 * Kloppenborg, B.; Baron, F. (2012), "SIMTOI: The SImulation and Modeling 
 * Tool for Optical Interferometry" (Version X). 
 * Available from  <https://github.com/bkloppenborg/simtoi>.
 *
 * This file is part of the SImulation and Modeling Tool for Optical 
 * Interferometry (SIMTOI).
 * 
 * SIMTOI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * as published by the Free Software Foundation version 3.
 * 
 * SIMTOI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with SIMTOI.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CMINIMIZER_LEVMAR_H_
#define CMINIMIZER_LEVMAR_H_

#include <valarray>

using namespace std;

#include "CMinimizer.h"

class CMinimizer_levmar: public CMinimizer
{
protected:
	float * mResiduals;

public:
	CMinimizer_levmar(CCL_GLThread * cl_gl_thread);
	virtual ~CMinimizer_levmar();

	static void ErrorFunc(double * params, double * output, int nParams, int nOutput, void * misc);

	string GetExitString(int exit_num);

	virtual void Init();

	void printresult(double * x, int n_pars, int n_data, vector<string> names, valarray<double> & info, valarray<double> & covar);

	virtual int run();
	int run(void (*error_func)(double *p, double *hx, int m, int n, void *adata));
};

#endif /* CMINIMIZER_LEVMAR_H_ */
