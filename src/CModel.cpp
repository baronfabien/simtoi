/*
 * CModel.cpp
 *
 *  Created on: Nov 7, 2011
 *      Author: bkloppenborg
 */

#include "CModel.h"
// Header files for position objects.
#include "CPosition.h"
#include "CPositionXY.h"
//#include "CFeature.h"
//#include "CFeatureList.h"
#include "misc.h"	// needed for pull_params

CModel::CModel(int n_params)
{
	// Init the object to have no rotation in yaw, pitch, or roll.
	rotation[0] = rotation[1] = rotation[2] = 0;
	mShader = NULL;
	position = NULL;

	mNParams = n_params;
	mScales = new float[mNParams];
	mParams = new float[mNParams];
	mNFreeParams = 0;
	mFreeParams = new bool[mNParams];;
	mScales = new float[mNParams];;
	mScale_mins = new float[mNParams];;
	mShaderLoaded = false;

//	CFeatureList * features = NULL;
//	double * scale = NULL;
//	double * scale_min = NULL;

//	n_free_parameters = n_free_params;
//	scale = float[n_free_parameters];
//	scale_min = float[n_free_parameters];
}

CModel::~CModel()
{
	// Free up memory.
	delete position;
	delete mParams;
	delete mFreeParams;
	delete mScales;
	delete mScale_mins;
}

int CModel::GetNPositionFreeParameters()
{
	return position->GetNFreeParameters();
}

int CModel::GetNFeatureFreeParameters()
{
	return 0;
	//return features->GetNFreeParameters();
}

void CModel::Rotate()
{
	// Rotations are implemented in the standard way, namely
	//  R_x(gamma) * R_y(beta) * R_z(alpha)
	// where gamma = pitch, beta = roll, alpha = yaw.

	glRotated(rotation[0], 1, 0, 0);
	glRotated(rotation[1], 0, 1, 0);
	glRotated(rotation[2], 0, 0, 1);
}

void CModel::Translate()
{
	float x, y, z;
	position->GetXYZ(x, y, z);

	// Call the translation routines.  Use the double-precision call.
	glTranslated(x, y, z);
}

/// Internal routine that reports the values of this object's parameters only.
void CModel::GetParams(float * out_params, int n_params)
{
	pull_params(mParams, mNParams, out_params, n_params, mFreeParams);
}

/// Returns the values for all parameters in this model
/// including the model, position, shader, and all features.
void CModel::GetAllParameters(float * params, int n_params)
{
	// Send parameter set command to the components of this model.
	// We use pointer math to advance the position of the array passed to the functions
	int n = 0;
	GetParams(params, n_params);
	n += this->n_free_parameters;
	position->GetParams(params + n, n_params - n);
	n += position->GetNFreeParameters();

	if(mShader != NULL)
	{
		mShader->GetParams(params + n, n_params - n);
		n += mShader->GetNFreeParams();
	}

	// TODO: Implement this function
	//features->GetParams(params + n, n_params - n);
}

int CModel::GetTotalFreeParameters()
{
	// Sum up the free parameters from the model, position, and features
	return this->GetNModelFreeParameters() + this->GetNPositionFreeParameters() + this->GetNFeatureFreeParameters();
}

int CModel::GetNModelFreeParameters()
{
	return mNFreeParams;
}

/// Internal routine to set the parameters for this object.
void CModel::SetParams(float * in_params, int n_params)
{
	pull_params(in_params, n_params, mParams, mNParams, mFreeParams);
}


/// Sets the parameters for this model, the position, shader, and all features.
void CModel::SetAllParameters(float * in_params, int n_params)
{
	// Here we use pointer math to advance the position of the array passed to the functions
	// that set the parameters.  First assign values to this model (use pull_params):
	int n = 0;
	SetParams(in_params, n_params);
	n += mNFreeParams;
	// Now set the values for the position object
	position->SetParams(in_params + n, n_params - n);
	n += position->GetNFreeParameters();
	// Then the shader.
	if(mShader != NULL)
	{
		mShader->SetParams(in_params + n, n_params - n);
		n += mShader->GetNFreeParams();
	}
	// Lastly the features
	//features->SetParams(in_params + n, n_params - n);
}

/// Assigns and initializes a position type.
void CModel::SetPositionType(ePositionTypes type)
{
	// If the position is already set and is of the current type, break.
	if(position != NULL && position->GetType() == type)
		return;

	// Otherwise assign the position.
	switch(type)
	{
//	case Orbit:
//		position = new PositionOrbit();
//		break;
	default:
		// By default models use XY position.
		position = new CPositionXY();
		break;
	}
}

void CModel::SetShader(CGLShaderWrapper * shader)
{
	this->mShader = shader;
}

void CModel::UseShader()
{
	if(this->mShader != NULL)
		mShader->UseShader();
}
