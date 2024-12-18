#pragma once

#include <vector>
#include <string>


#include <algorithm>
#include <DirectXMath.h>
#include <SimpleMath.h>
#include "fast_float.h"

#include "resource.h"
#include "geometry.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imfilebrowser.h"

class Dataset
{

public:
	Dataset();

	void importFromFile(std::string filename);

	void fillGPUReadyBuffer(std::vector<VertexData>& newVertexDataBuffer, std::vector<uint32_t>& newIndexBuffer);

	void fillGPUReadyBuffer(std::vector<VertexData>& newVertexDataBuffer, std::vector<draw_call_t>& newDrawCalls);

	float getLastLoadingTime() { return mLastLoadingTime; }

	float getLastPreprocessTime() { return mLastPreprocessTime; }

	int getLineCount() { return this->mLineCount; }

	int getPolylineCount() { return this->mPolyLineBuffer.size(); }

	int getVertexCount() { return this->mVertexCount; }

	float getMaxLineLength() { return this->mMaxLineLength; }

	float getMaxVertexAdjacentLineLength() { return this->mMaxVertexAdjacentLineLength; }

	bool isFileOpen() { return mLineCount > 0; }

	DirectX::XMFLOAT3 getDimensions() {
		return DirectX::XMFLOAT3(mMaximumCoordinateBounds.x - mMinimumCoordinateBounds.x, mMaximumCoordinateBounds.y - mMinimumCoordinateBounds.y, mMaximumCoordinateBounds.z - mMinimumCoordinateBounds.z);
	}

	DirectX::XMFLOAT2 getDataBounds() {
		return DirectX::XMFLOAT2(mMinDataValue, mMaxDataValue);
	}

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

	void preprocessLineData();

	void initializeValues();

};

