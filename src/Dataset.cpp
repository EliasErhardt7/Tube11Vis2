#define NOMINMAX
#include "Dataset.h"

#include <filesystem>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <iostream>

enum class eStatus {
	INITIAL,
	AFTERV,
	READV,
	READVT,
	READL, // This status is obsolete because of the hack mentioned later
	IGNOREUNTILNEWLINE
};

Dataset::Dataset()
{
	initializeValues();
}

unsigned int getDigitCountForUInt(unsigned int src)
{
	unsigned int cnt = 1;
	while (src > 9) {
		src /= 10;
		cnt++;
	}
	return cnt;
}


void Dataset::importFromFile(std::string filename)
{
	initializeValues();
	auto start = std::chrono::steady_clock::now();

	// check if file ends with .obj and if it exists
	const std::filesystem::path filePath{ filename };
	const std::filesystem::path obj_extension{ ".obj" };
	if (filePath.extension() != obj_extension) throw std::runtime_error("extension error: " + filePath.extension().string());
	if (!std::filesystem::exists(filePath)) throw std::runtime_error("file " + filePath.filename().string() + " does not exist");
	mName = filePath.filename().string();

	// open ifstream for file
	std::ifstream inFile{ filePath, std::ios::in | std::ios::binary };
	if (!inFile) throw std::runtime_error("failed to load file " + filePath.filename().string());

	// copy file into data
	std::string data(static_cast<size_t>(std::filesystem::file_size(filePath)), 0);
	inFile.read(data.data(), data.size());

	std::vector<VertexData> tmpVertexBuffer;
	std::vector<Poly> newPolyBuffer;
	VertexData tmpVertex;
	unsigned int currVertexCountInPolyBuffer = 0;

	eStatus status = eStatus::INITIAL;
	int curLine = 1;
	char tmpNumber[20];	// Note: This array is not even really necessary and can be replaced by Start- and End-indexes refering to data.
	int tNl = 0;
	float tmpPosition[3] = { 0, 0, 0 }; int tPi = 0;
	
	for (std::string::size_type i = 0; i <= data.size(); ++i) {
		char& c = data[i];

		if (c == '\r') continue;

		switch (status) {

		case eStatus::INITIAL:
			if (c == 'v') status = eStatus::AFTERV;
			else if (c == 'l') {
				unsigned int jumpForward = (getDigitCountForUInt(currVertexCountInPolyBuffer) + 1) * tmpVertexBuffer.size(); 
				currVertexCountInPolyBuffer += tmpVertexBuffer.size();
				newPolyBuffer.push_back({ std::move(tmpVertexBuffer) });
				if (data[i+1] != '\r' && data[i+1] != '\0') i += jumpForward;
				status = eStatus::IGNOREUNTILNEWLINE;
			}
			else if (c == 'g') status = eStatus::IGNOREUNTILNEWLINE;
			break;
			
		case eStatus::AFTERV:
			if (c == 't') status = eStatus::READVT;
			else if (c == ' ') status = eStatus::READV;
			else status = eStatus::INITIAL; // shouldnt happen (go back to INITIAL)
			break;
			
		case eStatus::READVT:
			if (c == '\n' || c == ' ') {
				if (tNl > 0) { 
					float result;
					auto answer = fast_float::from_chars(&tmpNumber[0], &tmpNumber[tNl], result);
					if (answer.ec != std::errc()) {
						tmpNumber[tNl] = '\0';
						std::cerr << "float parsing failure of vt-string '" << tmpNumber << "' in line " << curLine << std::endl;
					}
					else {
						tmpVertex.data = result;
						
						tmpVertexBuffer.push_back(tmpVertex);
						status = eStatus::INITIAL;
					}
				}
				tNl = 0;
			}
			else {
				tmpNumber[tNl] = c;
				tNl++;
			}
			break;
			
		case eStatus::READV:
			if (c == '\n' || c == ' ') {
				if (tNl > 0) { // if not its probably just an extra line indent
					float result;
					auto answer = fast_float::from_chars(&tmpNumber[0], &tmpNumber[tNl], result);
					if (answer.ec != std::errc()) {
						tmpNumber[tNl] = '\0';
						std::cerr << "float parsing failure of v-string '" << tmpNumber << "' for component " << tPi << " in line " << curLine << std::endl;
					}
					else {
						tmpPosition[tPi] = result;
						tPi++;
						if (tPi == 3) {
							// We gathered three floats:
							tmpVertex.position = DirectX::XMFLOAT3(tmpPosition[0], tmpPosition[1], tmpPosition[2]);
							tPi = 0;
							status = eStatus::INITIAL;
						}
					}
				}
				if (c == '\n') {
					status = eStatus::INITIAL;
				}
				tNl = 0;
			}
			else {
				tmpNumber[tNl] = c;
				tNl++;
			}
			break;
			
		case eStatus::IGNOREUNTILNEWLINE:
			if (c == '\n') status = eStatus::INITIAL;
			break;
		}

		if (c == '\n') curLine++;
		
	}

	this->mLastLoadingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count() * 0.000000001;

	mPolyLineBuffer = std::move(newPolyBuffer);

	this->preprocessLineData();

}


void Dataset::fillGPUReadyBuffer(std::vector<VertexData>& newVertexBuffer, std::vector<uint32_t>& newIndexBuffer)
{
    newVertexBuffer.clear();
    newIndexBuffer.clear();
    newVertexBuffer.reserve(mVertexCount);
    newIndexBuffer.reserve(mVertexCount); // Increased reservation for adjacency information

    uint32_t currIndex = 0;
	int count = 0;
    for (const Poly& pl : mPolyLineBuffer) {
		
		// Add middle vertices
		for (uint32_t i = 0; i < pl.vertices.size(); i++) {
			newVertexBuffer.push_back(pl.vertices[i]);
		}

		newIndexBuffer.push_back(currIndex+1);
		
		for (uint32_t i = 1; i < pl.vertices.size()-1; i++) {
			newIndexBuffer.push_back(currIndex + i);
		}
		newIndexBuffer.push_back(currIndex + pl.vertices.size() - 2);
		
		currIndex += pl.vertices.size();
			
		
		++count;
    }
}

void Dataset::fillGPUReadyBuffer(std::vector<VertexData>& newVertexBuffer, std::vector<draw_call_t>& newDrawCalls) {
	newVertexBuffer.clear();
	newDrawCalls.clear();
	newVertexBuffer.reserve(mVertexCount);
	newDrawCalls.reserve(mPolyLineBuffer.size());
	unsigned int currIndex = 0;
	for (Poly& pl : mPolyLineBuffer) {
		newVertexBuffer.insert(newVertexBuffer.end(), pl.vertices.begin(), pl.vertices.end());
		newDrawCalls.push_back({ (uint32_t)currIndex, (uint32_t)pl.vertices.size() });
		currIndex += pl.vertices.size();
	}
}

void Dataset::preprocessLineData()
{
	auto start = std::chrono::steady_clock::now();
	
	auto boundsCurvature = DirectX::XMFLOAT2(100.0F, 0.0F);
	mLineCount = 0;
	mVertexCount = 0;
	for (Poly& pLine : mPolyLineBuffer) {
		float lastLineLength = 0.0F;
		for (unsigned int i = 0; i < pLine.vertices.size(); i++) {
			VertexData& v = pLine.vertices[i];
			DirectX::XMStoreFloat3(&v.position, DirectX::XMVectorSaturate(DirectX::XMLoadFloat3(&v.position)));
			v.data = std::max(0.0F, std::min(v.data, 1.0F)); // clamp

			DirectX::XMStoreFloat3(&mMaximumCoordinateBounds, DirectX::XMVectorMax(DirectX::XMLoadFloat3(&v.position), DirectX::XMLoadFloat3(&mMaximumCoordinateBounds)));
			DirectX::XMStoreFloat3(&mMinimumCoordinateBounds, DirectX::XMVectorMin(DirectX::XMLoadFloat3(&v.position), DirectX::XMLoadFloat3(&mMinimumCoordinateBounds)));
			mMinDataValue = std::min(v.data, mMinDataValue);
			mMaxDataValue = std::max(v.data, mMaxDataValue);
			
			float preLength = 0.0F;
			float postLength = 0.0F;
			if (i > 0) {
				DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&pLine.vertices[i].position), DirectX::XMLoadFloat3(&pLine.vertices[i - 1].position));
				preLength = DirectX::XMVectorGetX(DirectX::XMVector3Length(diff));
			}
			if (i < pLine.vertices.size() - 1) {
				DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&pLine.vertices[i + 1].position), DirectX::XMLoadFloat3(&pLine.vertices[i].position));
				postLength = DirectX::XMVectorGetX(DirectX::XMVector3Length(diff));
			}
			mMaxLineLength = std::max(mMaxLineLength, preLength);
			mMaxVertexAdjacentLineLength = std::max(mMaxVertexAdjacentLineLength, preLength + postLength);

			if (preLength > 0 && postLength > 0) {
				DirectX::XMVECTOR a = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&pLine.vertices[i + 1].position), DirectX::XMLoadFloat3(&pLine.vertices[i].position));
				DirectX::XMVECTOR b = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&pLine.vertices[i - 1].position), DirectX::XMLoadFloat3(&pLine.vertices[i].position));
				float dotProduct = DirectX::XMVectorGetX(DirectX::XMVector3Dot(a, b));
				float alpha = std::acos(dotProduct / (preLength * postLength));
				boundsCurvature.x = std::min(alpha, boundsCurvature.x);
				boundsCurvature.y = std::max(alpha, boundsCurvature.y);
				pLine.vertices[i].curvature = alpha;
			}
		}
		// Mark beginnings and ends
		pLine.vertices[0].position.x *= -1;
		pLine.vertices[pLine.vertices.size() - 1].position.x *= -1;

		mLineCount += pLine.vertices.size() / 2 - 1;
		mVertexCount += pLine.vertices.size();
	}

	// Now lets run again and scale the data and curvature in between 0 and 1
	for (Poly& pLine : mPolyLineBuffer) {
		for (VertexData& v : pLine.vertices) {
			v.data = (v.data - mMinDataValue) / (mMaxDataValue - mMinDataValue);

			if (v.curvature < 0.0) v.curvature = boundsCurvature.x; 
			v.curvature = 1.0F - (v.curvature - boundsCurvature.x) / (boundsCurvature.y - boundsCurvature.x);
		}
	}

	this->mLastPreprocessTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count() * 0.000000001;
}

void Dataset::initializeValues()
{
	mLastLoadingTime = 0.0F;
	mLastPreprocessTime = 0.0F; 
	mMinimumCoordinateBounds = DirectX::XMFLOAT3(1.0F, 1.0F, 1.0F);
	mMaximumCoordinateBounds = DirectX::XMFLOAT3(0.0F, 0.0F, 0.0F);
	mMinDataValue = 1.0F;
	mMaxDataValue = 0.0F;
	mLineCount = 0;
	mVertexCount = 0;
	mMaxLineLength = 0.0F;
	mMaxVertexAdjacentLineLength = 0.0F;
}
