/*
 * CModel.cpp
 *
 *  Created on: Nov 7, 2011
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
 
 /*
 *  Base class for all models implementing a common set of functions to get/set
 *  parameters and use shaders.
 *
 *  NOTE: When deriving from this object the yaw, pitch, and roll occupy the first
 *  three values in mParameters.
 */

#include "CModel.h"
// Header files for position objects.
#include "CPosition.h"
#include "CPositionXY.h"
#include "CPositionOrbit.h"
//#include "CFeature.h"
//#include "CFeatureList.h"

CModel::CModel(int n_params)
	: CParameters(4 + n_params)
{
	mPosition = NULL;
	mType = CModelList::NONE;

	mBaseParams = 3;	// Number of base params, less one (zero indexed).

	// Shader storage location, boolean if it is loaded:
	mShader = NULL;
	mShaderLoaded = false;

	// Init the yaw, pitch, and roll to be zero and fixed.  Set their names:
	mParamNames.push_back("Inclination");
	SetParam(0, 0);
	SetFree(0, false);
	mParamNames.push_back("Pos. Angle");
	SetParam(1, 0);
	SetFree(1, false);
	mParamNames.push_back("Rotation");
	SetParam(2, 0);
	SetFree(2, false);
	mParamNames.push_back("Color");
	SetParam(3, 1.0);
	SetFree(3, false);
}

CModel::~CModel()
{
	// Free up memory.
	delete mPosition;
}


/// Returns the values for all parameters in this model
/// including the model, position, shader, and all features.
void CModel::GetAllParameters(double * params, int n_params)
{
	// Send parameter set command to the components of this model.
	// We use pointer math to advance the position of the array passed to the functions
	int n = 0;
	GetParams(params, n_params);
	n += this->mNParams;
	mPosition->GetParams(params + n, n_params - n);
	n += mPosition->GetNParams();

	if(mShader != NULL)
	{
		mShader->GetParams(params + n, n_params - n);
		n += mShader->GetNParams();
	}

	// TODO: Implement this function
	//features->GetParams(params + n, n_params - n);
}

vector< pair<double, double> > CModel::GetFreeParamMinMaxes()
{
	vector< pair<double, double> > tmp1;
	vector< pair<double, double> > tmp2 = GetFreeMinMaxes();
	tmp1.insert( tmp1.end(), tmp2.begin(), tmp2.end() );
	tmp2 = mPosition->GetFreeMinMaxes();
	tmp1.insert( tmp1.end(), tmp2.begin(), tmp2.end() );

	if(mShader != NULL)
	{
		tmp2 = mShader->GetFreeMinMaxes();
		tmp1.insert( tmp1.end(), tmp2.begin(), tmp2.end() );
	}

	return tmp1;
}

/// Gets the free parameters for this model in a:
///  scale_params = false => uniform hypercube (x = [0...1])
///  scale_params = true => native values (x = [param.min... param.max])
void CModel::GetFreeParameters(double * params, int n_params, bool scale_params)
{
	int n = 0;
	GetFreeParams(params, n_params, scale_params);
	n += this->mNFreeParams;
	mPosition->GetFreeParams(params + n, n_params - n, scale_params);
	n += mPosition->GetNFreeParams();

	if(mShader != NULL)
	{
		mShader->GetFreeParams(params + n, n_params - n, scale_params);
		n += mShader->GetNFreeParams();
	}
}


/// Returns a vector of strings containing the names of the free parameters:
vector<string> CModel::GetFreeParameterNames()
{
	vector<string> tmp1;
	vector<string> tmp2 = GetFreeParamNames();
	tmp1.insert( tmp1.end(), tmp2.begin(), tmp2.end() );
	tmp2 = mPosition->GetFreeParamNames();
	tmp1.insert( tmp1.end(), tmp2.begin(), tmp2.end() );

	if(mShader != NULL)
	{
		tmp2 = mShader->GetFreeParamNames();
		tmp1.insert( tmp1.end(), tmp2.begin(), tmp2.end() );
	}

	return tmp1;
}

/// Returns the product of priors for all free model parameters.
/// NOTICE: This intentially overrides, but still calls, the GetFreePriorProd derived
/// by inheritance from CParameters.
double CModel::GetFreePriorProd()
{
	int tmp = CParameters::GetFreePriorProd();
	tmp *= mPosition->GetFreePriorProd();

	if(mShader != NULL)
		tmp *= mShader->GetFreePriorProd();

	return tmp;
}

int CModel::GetTotalFreeParameters()
{
	// Sum up the free parameters from the model, position, and features
	return this->GetNModelFreeParameters() + this->GetNPositionFreeParameters() + this->GetNShaderFreeParameters() + this->GetNFeatureFreeParameters();
}

void CModel::Color()
{
	glColor4d(mParams[3], 0.0, 0.0, 1.0);
}

/// Creates a lookup table of sine and cosine values for use in drawing
/// Taken from http://openglut.cvs.sourceforge.net/viewvc/openglut/openglut/lib/src/og_geometry.c
void CModel::CircleTable( double * sint, double * cost, const int n )
{
    int i;
    const int size = abs( n );
    double angle;

    assert( n );
    angle = 2 * PI / ( double )n;

    for( i = 0; i < size; i++ )
    {
        sint[ i ] = sin( angle * i );
        cost[ i ] = cos( angle * i );
    }

    /* Last sample is duplicate of the first */
    sint[ size ] = sint[ 0 ];
    cost[ size ] = cost[ 0 ];
}

void CModel::Rotate()
{
	// Rotations are implemented in the standard way, namely
	//  R_x(gamma) * R_y(beta) * R_z(alpha)
	// where gamma = pitch, beta = roll, alpha = yaw.

	glRotatef(mParams[0], 1, 0, 0);	// inclination
	glRotatef(mParams[1], 0, 1, 0); // position angle
	glRotatef(mParams[2], 0, 0, 1); // roll
	CCL_GLThread::CheckOpenGLError("CModel::Rotate()");
}

void CModel::Restore(Json::Value input, CGLShaderList * shader_list)
{
	// Restore the base parameters
	CParameters::Restore(input["base"]);
	CGLShaderWrapperPtr shader;

	// Now the position
	if(input.isMember("position"))
	{
		if(input["position"].isMember("type"))
		{
			SetPositionType( CPosition::PositionTypes(input["position"]["type"].asInt()) );
			mPosition->Restore(input["position"]);
		}
	}

	// Now the shader
	if(input.isMember("shader"))
	{
		if(input["shader"].isMember("type"))
		{
			shader = shader_list->GetShader( CGLShaderList::ShaderTypes( input["shader"]["type"].asInt()) );
			SetShader(shader);
			mShader->Restore(input["shader"]);
		}
	}
}

/// Sets up the matrix mode for rendering models.
void CModel::SetupMatrix()
{
    // Rotate from (x,y,z) to (North, East, Away).  Note, we are following the (x,y,z)
    // convention of the orbital equations here.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(90, 0, 0, 1);
	glScalef(1, 1, -1);

}

/// Serializes a model object into a JSON object.
Json::Value CModel::Serialize()
{
	Json::Value output;
	output["base"] = CParameters::Serialize();
	output["base"]["type"] = mType;
	output["position"] = mPosition->Serialize();
	output["position"]["type"] = mPosition->GetType();

	if(mShader != NULL)
	{
		output["shader"] = mShader->Serialize();
		output["shader"]["type"] = mShader->GetType();
	}

	return output;
}

/// Copies inc and Omega from the position object iff it's type is CPosition::ORBIT
/// and the angular parameter is not free.
void CModel::SetAnglesFromPosition()
{
	if(mPosition->GetType() == CPosition::ORBIT)
	{
		// Inclination
		if(!this->IsFree(0))
			mParams[0] = mPosition->GetParam(0);

		// Omega / Position angle
		// TODO: Is this right?
		if(!this->IsFree(1))
			mParams[1] = mPosition->GetParam(1);
	}
}

/// Sets the parameters for this model, the position, shader, and all features.
void CModel::SetFreeParameters(double * in_params, int n_params, bool scale_params)
{
	// Here we use pointer math to advance the position of the array passed to the functions
	// that set the parameters.  First assign values to this model (use pull_params):
	int n = 0;
	SetFreeParams(in_params, n_params, scale_params);
	n += mNFreeParams;
	// Now set the values for the position object
	mPosition->SetFreeParams(in_params + n, n_params - n, scale_params);
	n += mPosition->GetNFreeParams();
	// Then the shader.
	if(mShader != NULL)
	{
		mShader->SetFreeParams(in_params + n, n_params - n, scale_params);
		n += mShader->GetNFreeParams();
	}
	// Lastly the features
	//features->SetParams(in_params + n, n_params - n);

	// Now copy angles over from the orbit object (if they are not free)
	SetAnglesFromPosition();
}

/// Assigns and initializes a position type.
void CModel::SetPositionType(CPosition::PositionTypes type)
{
	// If the position is already set and is of the current type, break.
	if(mPosition != NULL && mPosition->GetType() == type)
		return;

	mPosition = CPosition::GetPosition(type);
}

void CModel::SetTime(double time)
{
	CPositionOrbit * tmp;
	if(mPosition->GetType() == CPosition::ORBIT)
	{
		tmp = reinterpret_cast<CPositionOrbit*>(mPosition);
		tmp->SetTime(time);
	}
}

void CModel::SetShader(CGLShaderWrapperPtr shader)
{
	mShader = shader;
}

void CModel::Translate()
{
	double x, y, z;
	mPosition->GetXYZ(x, y, z);

	// Call the translation routines.  Use the double-precision call.
	glTranslatef(x, y, z);
	CCL_GLThread::CheckOpenGLError("CModel::Translate()");
}

void CModel::UseShader(double min_xyz[3], double max_xyz[3])
{
	if(mShader != NULL)
		mShader->UseShader(min_xyz, max_xyz);

	CCL_GLThread::CheckOpenGLError("CModel::UseShader()");
}
