/*
 * CModelSphere.h
 *
 *  Created on: Nov 8, 2011
 *      Author: bkloppenborg
 */
 
 /* 
 * Copyright (c) 2011 Brian Kloppenborg
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

#ifndef CMODELSPHERE_H_
#define CMODELSPHERE_H_

#include "CModel.h"

class CModelSphere: public CModel
{
protected:
	int mSlices;

public:
	CModelSphere();
	virtual ~CModelSphere();

	double GetMaxHeight();

	void Render(GLuint framebuffer_object, int width, int height);
};

#endif /* CMODELSPHERE_H_ */
