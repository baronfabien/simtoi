/*
 * CPosition.cpp
 *
 *  Created on: Nov 7, 2011
 *      Author: bkloppenborg
 */

#include "CPosition.h"
#include "misc.h"	// needed for pull_params

CPosition::CPosition(int n_parameters)
	: CParameters(n_parameters)
{
	// Init to an invalid position type.
	mType = POSITION_NONE;
}

CPosition::~CPosition()
{

}

