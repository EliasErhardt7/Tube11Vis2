#pragma once

#include <vector>
#include <string>


#include <algorithm>
#include <DirectXMath.h>
#include <SimpleMath.h>
#include "fast_float.h"

#include "resource.h"
#include "geometry.h"
#include "util/util.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imfilebrowser.h"

/// <summary>
/// This class represents a Line-Dataset. It furthermore offers functionality to load data of an .obj-file and
/// does some preprocessing-steps.
/// </summary>
class Dataset
{

public:
	Dataset();

	/// <summary>
	/// Load the data of the given ".obj" file into the cpu (mPolyLine-Buffer) and starts some preprocessing work.
	/// </summary>
	/// <param name="filename">The full filename to the obj-file</param>
	void importFromFile(std::string filename);

	/// <summary>
	/// This function fills a VertexData-list and an index list. the vertices at in between points of a line
	/// get duplicated as proposed in the paper.
	/// </summary>
	void fillGPUReadyBuffer(std::vector<VertexData>& newVertexDataBuffer, std::vector<uint32_t>& newIndexBuffer);

	/// <summary>
	/// This function fills a VertexData-list and a list with information for the draw calls of the different
	/// line segments.
	/// </summary>
	void fillGPUReadyBuffer(std::vector<VertexData>& newVertexDataBuffer, std::vector<draw_call_t>& newDrawCalls);

	/// <summary>
	/// Returns the time it took to import the last file in seconds
	/// </summary>
	float getLastLoadingTime() { return mLastLoadingTime; }

	/// <summary>
	/// Returns the time it took to preprocess the last file in seconds
	/// </summary>
	float getLastPreprocessTime() { return mLastPreprocessTime; }

	/// <summary>
	/// Returns the amount of line individual line segments inside the file
	/// </summary>
	int getLineCount() { return this->mLineCount; }

	/// <summary>
	/// Returns the amount of connected line segments
	/// </summary>
	int getPolylineCount() { return this->mPolyLineBuffer.size(); }

	/// <summary>
	/// Returns the amount of singular vertices in the dataset
	/// </summary>
	int getVertexCount() { return this->mVertexCount; }

	/// <summary>
	/// returns the length of the longest line inside the dataset.
	/// </summary>
	float getMaxLineLength() { return this->mMaxLineLength; }

	/// <summary>
	/// returns the summed up length of the VertexData with the longest adjacend lines.
	/// </summary>
	float getMaxVertexAdjacentLineLength() { return this->mMaxVertexAdjacentLineLength; }

	/// <summary>
	/// returns true if a file was loaded and the polyline buffer is full
	/// </summary>
	bool isFileOpen() { return mLineCount > 0; }

	/// <summary>
	/// returns the dimensions of the dataset
	/// </summary>
	DirectX::XMFLOAT3 getDimensions() {
		return DirectX::XMFLOAT3(mMaximumCoordinateBounds.x - mMinimumCoordinateBounds.x, mMaximumCoordinateBounds.y - mMinimumCoordinateBounds.y, mMaximumCoordinateBounds.z - mMinimumCoordinateBounds.z);
	}

	/// <summary>
	/// returns the min- and maximum 
	/// </summary>
	/// <returns></returns>
	DirectX::XMFLOAT2 getDataBounds() {
		return DirectX::XMFLOAT2(mMinDataValue, mMaxDataValue);
	}

	/// <summary>
	/// Returns the filename of the currently loaded dataset
	/// </summary>
	std::string getName() { return mName; }


private:
	std::string mName; // The name of the file

	std::vector<Poly> mPolyLineBuffer; // contains the loaded polylines
	float mLastLoadingTime = 0.0F; // stores the time it took to load the data into cpu memory
	float mLastPreprocessTime = 0.0F; // stores the time it took to preprocess the current data in seconds

	DirectX::XMFLOAT3 mMinimumCoordinateBounds = DirectX::XMFLOAT3(1.0F, 1.0F, 1.0F);
	DirectX::XMFLOAT3 mMaximumCoordinateBounds = DirectX::XMFLOAT3(0.0F, 0.0F, 0.0F);
	float mMinDataValue = 1.0F;
	float mMaxDataValue = 0.0F;

	int mLineCount = 0;
	int mVertexCount = 0;
	
	float mMaxLineLength = 0.0F;
	float mMaxVertexAdjacentLineLength = 0.0F;

	/// <summary>
	/// This function gathers information about the dataset which is later used for the dynamic
	/// transformations. It furthermore edits the x coordinate of the end- and start points for correct
	/// cap clipping. It furthermore transforms scales the data and curvature in between 0 and 1
	/// </summary>
	void preprocessLineData();

	/// <summary>
	/// Initializes the member variables with their initial values.
	/// </summary>
	void initializeValues();

};

