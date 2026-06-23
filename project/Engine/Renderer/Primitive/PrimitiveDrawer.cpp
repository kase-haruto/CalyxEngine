#include "PrimitiveDrawer.h"

#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <algorithm>
#include <cmath>
#include <numbers>

PrimitiveDrawer* PrimitiveDrawer::GetInstance(){
	static PrimitiveDrawer instance;
	return &instance;
}

void PrimitiveDrawer::Initialize(){
	lineDrawer_ = std::make_unique<LineDrawer>();
	lineDrawer_->Initialize();

	lineNoDepthDrawer_ = std::make_unique<LineDrawer>();
	lineNoDepthDrawer_->Initialize();

	debugViewLineDrawer_ = std::make_unique<LineDrawer>();
	debugViewLineDrawer_->Initialize();

	debugViewLineNoDepthDrawer_ = std::make_unique<LineDrawer>();
	debugViewLineNoDepthDrawer_->Initialize();

	boxDrawer_ = std::make_unique<BoxDrawer>();
	boxDrawer_->Initialize();

	effectPreviewLineDrawer_ = std::make_unique<LineDrawer>();
	effectPreviewLineDrawer_->Initialize();

	effectPreviewBoxDrawer_ = std::make_unique<BoxDrawer>();
	effectPreviewBoxDrawer_->Initialize();
}

void PrimitiveDrawer::Finalize(){
	if (lineDrawer_){
		lineDrawer_->Clear();
	}
	lineDrawer_.reset();

	if(lineNoDepthDrawer_) {
		lineNoDepthDrawer_->Clear();
	}
	lineNoDepthDrawer_.reset();

	if(debugViewLineDrawer_) {
		debugViewLineDrawer_->Clear();
	}
	debugViewLineDrawer_.reset();

	if(debugViewLineNoDepthDrawer_) {
		debugViewLineNoDepthDrawer_->Clear();
	}
	debugViewLineNoDepthDrawer_.reset();

	if (boxDrawer_) {
		boxDrawer_->Clear();
	}
	boxDrawer_.reset();

	if(effectPreviewLineDrawer_) {
		effectPreviewLineDrawer_->Clear();
	}
	effectPreviewLineDrawer_.reset();

	if(effectPreviewBoxDrawer_) {
		effectPreviewBoxDrawer_->Clear();
	}
	effectPreviewBoxDrawer_.reset();
}


void PrimitiveDrawer::DrawLine3d(const CalyxEngine::Vector3& start, const CalyxEngine::Vector3& end, const CalyxEngine::Vector4& color, LineDepthMode depthMode){
	LineDrawer* drawer = depthMode == LineDepthMode::NoDepthTest ? lineNoDepthDrawer_.get() : lineDrawer_.get();
	if(drawer) {
		drawer->DrawLine(start, end, color);
	}
}

void PrimitiveDrawer::DrawDebugViewLine3d(const CalyxEngine::Vector3& start, const CalyxEngine::Vector3& end, const CalyxEngine::Vector4& color, LineDepthMode depthMode) {
	LineDrawer* drawer = depthMode == LineDepthMode::NoDepthTest ? debugViewLineNoDepthDrawer_.get() : debugViewLineDrawer_.get();
	if(drawer) {
		drawer->DrawLine(start, end, color);
	}
}

namespace {
	void DrawCircleLines(
		LineDrawer* drawer,
		const CalyxEngine::Vector3& center,
		const CalyxEngine::Quaternion& rotate,
		float radiusX,
		float radiusZ,
		const CalyxEngine::Vector4& color,
		int subdivision) {
		if(!drawer) return;

		const int segments = (std::max)(subdivision, 3);
		const float step = 2.0f * float(std::numbers::pi) / static_cast<float>(segments);

		for(int i = 0; i < segments; ++i) {
			const float t0 = step * static_cast<float>(i);
			const float t1 = step * static_cast<float>(i + 1);
			const CalyxEngine::Vector3 p0 = center + CalyxEngine::Quaternion::RotateVector(
				{std::cos(t0) * radiusX, 0.0f, std::sin(t0) * radiusZ},
				rotate);
			const CalyxEngine::Vector3 p1 = center + CalyxEngine::Quaternion::RotateVector(
				{std::cos(t1) * radiusX, 0.0f, std::sin(t1) * radiusZ},
				rotate);
			drawer->DrawLine(p0, p1, color);
		}
	}

	void DrawConeLines(
		LineDrawer* drawer,
		const CalyxEngine::Vector3& apex,
		const CalyxEngine::Quaternion& rotate,
		float height,
		float radiusX,
		float radiusZ,
		const CalyxEngine::Vector4& color,
		int subdivision) {
		if(!drawer) return;

		const int segments = (std::max)(subdivision, 3);
		const float step = 2.0f * float(std::numbers::pi) / static_cast<float>(segments);
		const CalyxEngine::Vector3 baseCenter = apex + CalyxEngine::Quaternion::RotateVector({0.0f, height, 0.0f}, rotate);

		for(int i = 0; i < segments; ++i) {
			const float t0 = step * static_cast<float>(i);
			const float t1 = step * static_cast<float>(i + 1);
			const CalyxEngine::Vector3 p0 = baseCenter + CalyxEngine::Quaternion::RotateVector(
				{std::cos(t0) * radiusX, 0.0f, std::sin(t0) * radiusZ},
				rotate);
			const CalyxEngine::Vector3 p1 = baseCenter + CalyxEngine::Quaternion::RotateVector(
				{std::cos(t1) * radiusX, 0.0f, std::sin(t1) * radiusZ},
				rotate);

			drawer->DrawLine(p0, p1, color);
			if(i % (std::max)(segments / 4, 1) == 0) {
				drawer->DrawLine(apex, p0, color);
			}
		}
	}

	void DrawCapsuleLines(
		LineDrawer* drawer,
		const CalyxEngine::Vector3& center,
		const CalyxEngine::Quaternion& rotate,
		float radius,
		float height,
		const CalyxEngine::Vector4& color,
		int subdivision) {
		if(!drawer) return;

		const int segments = (std::max)(subdivision, 8);
		const int arcSegments = (std::max)(segments / 4, 4);
		const float safeRadius = (std::max)(radius, 0.0f);
		const float safeHeight = (std::max)(height, safeRadius * 2.0f);
		const float halfSegment = (std::max)(0.0f, safeHeight * 0.5f - safeRadius);
		const float step = 2.0f * float(std::numbers::pi) / static_cast<float>(segments);

		auto toWorld = [&](const CalyxEngine::Vector3& local) {
			return center + CalyxEngine::Quaternion::RotateVector(local, rotate);
		};

		// カプセル中央の円柱部を示す上下リングと側面線を描画する
		for(int i = 0; i < segments; ++i) {
			const float t0 = step * static_cast<float>(i);
			const float t1 = step * static_cast<float>(i + 1);
			const CalyxEngine::Vector3 top0 = {std::cos(t0) * safeRadius, halfSegment, std::sin(t0) * safeRadius};
			const CalyxEngine::Vector3 top1 = {std::cos(t1) * safeRadius, halfSegment, std::sin(t1) * safeRadius};
			const CalyxEngine::Vector3 bottom0 = {top0.x, -halfSegment, top0.z};
			const CalyxEngine::Vector3 bottom1 = {top1.x, -halfSegment, top1.z};

			drawer->DrawLine(toWorld(top0), toWorld(top1), color);
			drawer->DrawLine(toWorld(bottom0), toWorld(bottom1), color);

			if(i % (std::max)(segments / 4, 1) == 0) {
				drawer->DrawLine(toWorld(top0), toWorld(bottom0), color);
			}
		}

		// 半球は複数方向の縦アークで表現する
		const int meridianCount = 4;
		for(int meridian = 0; meridian < meridianCount; ++meridian) {
			const float theta = float(meridian) * float(std::numbers::pi) / static_cast<float>(meridianCount);
			const float cosTheta = std::cos(theta);
			const float sinTheta = std::sin(theta);

			for(int i = 0; i < arcSegments; ++i) {
				const float p0 = (float(i) / float(arcSegments)) * (float(std::numbers::pi) * 0.5f);
				const float p1 = (float(i + 1) / float(arcSegments)) * (float(std::numbers::pi) * 0.5f);

				const CalyxEngine::Vector3 top0 = {
					std::cos(p0) * cosTheta * safeRadius,
					halfSegment + std::sin(p0) * safeRadius,
					std::cos(p0) * sinTheta * safeRadius};
				const CalyxEngine::Vector3 top1 = {
					std::cos(p1) * cosTheta * safeRadius,
					halfSegment + std::sin(p1) * safeRadius,
					std::cos(p1) * sinTheta * safeRadius};
				const CalyxEngine::Vector3 bottom0 = {
					std::cos(p0) * cosTheta * safeRadius,
					-halfSegment - std::sin(p0) * safeRadius,
					std::cos(p0) * sinTheta * safeRadius};
				const CalyxEngine::Vector3 bottom1 = {
					std::cos(p1) * cosTheta * safeRadius,
					-halfSegment - std::sin(p1) * safeRadius,
					std::cos(p1) * sinTheta * safeRadius};

				drawer->DrawLine(toWorld(top0), toWorld(top1), color);
				drawer->DrawLine(toWorld(bottom0), toWorld(bottom1), color);
			}
		}
	}
}

void PrimitiveDrawer::DrawBox(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color) {
	if (boxDrawer_) {
		boxDrawer_->DrawBox(center, rotate,size, color);
	}
}

void PrimitiveDrawer::DrawEffectPreviewBox(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color) {
	if(effectPreviewBoxDrawer_) {
		effectPreviewBoxDrawer_->DrawBox(center, rotate, size, color);
	}
}

void PrimitiveDrawer::DrawCircle(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, float radiusX, float radiusZ, const CalyxEngine::Vector4& color, int subdivision) {
	DrawCircleLines(lineDrawer_.get(), center, rotate, radiusX, radiusZ, color, subdivision);
}

void PrimitiveDrawer::DrawCone(const CalyxEngine::Vector3& apex, const CalyxEngine::Quaternion& rotate, float height, float radiusX, float radiusZ, const CalyxEngine::Vector4& color, int subdivision) {
	DrawConeLines(lineDrawer_.get(), apex, rotate, height, radiusX, radiusZ, color, subdivision);
}

void PrimitiveDrawer::DrawEffectPreviewCircle(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, float radiusX, float radiusZ, const CalyxEngine::Vector4& color, int subdivision) {
	DrawCircleLines(effectPreviewLineDrawer_.get(), center, rotate, radiusX, radiusZ, color, subdivision);
}

void PrimitiveDrawer::DrawEffectPreviewCone(const CalyxEngine::Vector3& apex, const CalyxEngine::Quaternion& rotate, float height, float radiusX, float radiusZ, const CalyxEngine::Vector4& color, int subdivision) {
	DrawConeLines(effectPreviewLineDrawer_.get(), apex, rotate, height, radiusX, radiusZ, color, subdivision);
}

void PrimitiveDrawer::DrawGrid(){
	const uint32_t kSubdivision = 32;
	const float kGridHalfWidth = 32.0f;
	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

	for (uint32_t index = 0; index <= kSubdivision; ++index){
		float offset = -kGridHalfWidth + index * kGridEvery;

		// --- 縦線（Z軸方向） ---
		CalyxEngine::Vector3 verticalStart(offset, 0.0f, kGridHalfWidth);
		CalyxEngine::Vector3 verticalEnd(offset, 0.0f, -kGridHalfWidth);

		CalyxEngine::Vector4 verticalColor = (std::abs(offset) < 0.001f) ? CalyxEngine::Vector4(0.0f, 1.0f, 0.0f, 1.0f) // X=0 line
			: CalyxEngine::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		DrawLine3d(verticalStart, verticalEnd, verticalColor);

		CalyxEngine::Vector3 horizontalStart(-kGridHalfWidth, 0.0f, offset);
		CalyxEngine::Vector3 horizontalEnd(kGridHalfWidth, 0.0f, offset);

		CalyxEngine::Vector4 horizontalColor = (std::abs(offset) < 0.001f) ? CalyxEngine::Vector4(1.0f, 0.0f, 0.0f, 1.0f) // Z=0 line
			: CalyxEngine::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		DrawLine3d(horizontalStart, horizontalEnd, horizontalColor);
	}
}

void PrimitiveDrawer::DrawAABB(const CalyxEngine::Vector3& minP, const CalyxEngine::Vector3& maxP,
							   const CalyxEngine::Vector4& color) {
	// 中心とサイズを計算
	CalyxEngine::Vector3 center = (minP + maxP) * 0.5f;
	CalyxEngine::Vector3 size = (maxP - minP);

	// OBB は回転付きだが、Identity を渡せば AABB 扱いになる
	CalyxEngine::Quaternion identity; // (0,0,0,1)
	identity.MakeIdentity();

	DrawBox(center, identity, size, color); // 既存 API を再利用
}

void PrimitiveDrawer::DrawOBB(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, const CalyxEngine::Vector3& size, const CalyxEngine::Vector4& color){
	const uint32_t vertexNum = 8;

	// 各軸の半サイズをクォータニオンで回転
	CalyxEngine::Vector3 halfSizeX = CalyxEngine::Vector3::Transform({1.0f, 0.0f, 0.0f}, rotate) * (size.x * 0.5f);
	CalyxEngine::Vector3 halfSizeY = CalyxEngine::Vector3::Transform({0.0f, 1.0f, 0.0f}, rotate) * (size.y * 0.5f);
	CalyxEngine::Vector3 halfSizeZ = CalyxEngine::Vector3::Transform({0.0f, 0.0f, 1.0f}, rotate) * (size.z * 0.5f);

	// 頂点を計算
	CalyxEngine::Vector3 vertices[vertexNum];
	CalyxEngine::Vector3 offsets[vertexNum] = {
		{-1, -1, -1}, {-1,  1, -1}, {1, -1, -1}, {1,  1, -1},
		{-1, -1,  1}, {-1,  1,  1}, {1, -1,  1}, {1,  1,  1}
	};

	for (int i = 0; i < vertexNum; ++i){
		CalyxEngine::Vector3 localVertex =
			offsets[i].x * halfSizeX +
			offsets[i].y * halfSizeY +
			offsets[i].z * halfSizeZ;
		vertices[i] = center + localVertex;
	}

	// 辺を描画
	int edges[12][2] = {
		{0, 1}, {1, 3}, {3, 2}, {2, 0},
		{4, 5}, {5, 7}, {7, 6}, {6, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	for (int i = 0; i < 12; ++i){
		DrawLine3d(vertices[edges[i][0]], vertices[edges[i][1]], color);
	}
}

void PrimitiveDrawer::DrawSphere(const CalyxEngine::Vector3& center, const float radius, int subdivision, CalyxEngine::Vector4 color){

	// 分割数
	const uint32_t kSubdivision = subdivision;
	const float kLonEvery = 2 * float(std::numbers::pi) / kSubdivision;
	const float kLatEvery = float(std::numbers::pi) / kSubdivision;
	CalyxEngine::Vector3 a, b, c, d;

	// 緯度方向に分割
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex){
		float lat = -float(std::numbers::pi) / 2.0f + kLatEvery * latIndex;
		float nextLat = lat + kLatEvery;

		// 経度方向に分割
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex){
			float lon = lonIndex * kLonEvery;
			float nextLon = lon + kLonEvery;

			// 点の座標を計算
			a.x = radius * (std::cos(lat) * std::cos(lon)) + center.x;
			a.y = radius * std::sin(lat) + center.y;
			a.z = radius * (std::cos(lat) * std::sin(lon)) + center.z;

			b.x = radius * (std::cos(nextLat) * std::cos(lon)) + center.x;
			b.y = radius * std::sin(nextLat) + center.y;
			b.z = radius * (std::cos(nextLat) * std::sin(lon)) + center.z;

			c.x = radius * (std::cos(lat) * std::cos(nextLon)) + center.x;
			c.y = radius * std::sin(lat) + center.y;
			c.z = radius * (std::cos(lat) * std::sin(nextLon)) + center.z;

			d.x = radius * (std::cos(nextLat) * std::cos(nextLon)) + center.x;
			d.y = radius * std::sin(nextLat) + center.y;
			d.z = radius * (std::cos(nextLat) * std::sin(nextLon)) + center.z;

			// 経度方向の線を描画
			DrawLine3d(a, c, color);
			DrawLine3d(a, b, color);

			// 緯度方向の線を描画
			DrawLine3d(b, d, color);
			DrawLine3d(c, d, color);
		}
	}

}

void PrimitiveDrawer::DrawCapsule(const CalyxEngine::Vector3& center, const CalyxEngine::Quaternion& rotate, float radius, float height, const CalyxEngine::Vector4& color, int subdivision) {
	DrawCapsuleLines(lineDrawer_.get(), center, rotate, radius, height, color, subdivision);
}

void PrimitiveDrawer::DrawEffectPreviewSphere(const CalyxEngine::Vector3& center, const float radius, int subdivision, CalyxEngine::Vector4 color) {
	if(!effectPreviewLineDrawer_) return;

	const uint32_t kSubdivision = subdivision;
	const float kLonEvery = 2 * float(std::numbers::pi) / kSubdivision;
	const float kLatEvery = float(std::numbers::pi) / kSubdivision;
	CalyxEngine::Vector3 a, b, c, d;

	for(uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -float(std::numbers::pi) / 2.0f + kLatEvery * latIndex;
		float nextLat = lat + kLatEvery;

		for(uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			float lon = lonIndex * kLonEvery;
			float nextLon = lon + kLonEvery;

			a.x = radius * (std::cos(lat) * std::cos(lon)) + center.x;
			a.y = radius * std::sin(lat) + center.y;
			a.z = radius * (std::cos(lat) * std::sin(lon)) + center.z;

			b.x = radius * (std::cos(nextLat) * std::cos(lon)) + center.x;
			b.y = radius * std::sin(nextLat) + center.y;
			b.z = radius * (std::cos(nextLat) * std::sin(lon)) + center.z;

			c.x = radius * (std::cos(lat) * std::cos(nextLon)) + center.x;
			c.y = radius * std::sin(lat) + center.y;
			c.z = radius * (std::cos(lat) * std::sin(nextLon)) + center.z;

			d.x = radius * (std::cos(nextLat) * std::cos(nextLon)) + center.x;
			d.y = radius * std::sin(nextLat) + center.y;
			d.z = radius * (std::cos(nextLat) * std::sin(nextLon)) + center.z;

			effectPreviewLineDrawer_->DrawLine(a, c, color);
			effectPreviewLineDrawer_->DrawLine(a, b, color);
			effectPreviewLineDrawer_->DrawLine(b, d, color);
			effectPreviewLineDrawer_->DrawLine(c, d, color);
		}
	}
}

void PrimitiveDrawer::Render(bool includeDebugViewOnly, LineDepthMode depthMode){
#if defined(_DEBUG) || defined(DEVELOP)
	if(depthMode == LineDepthMode::DepthTest && lineDrawer_) {
		lineDrawer_->Render();
	}

	if(depthMode == LineDepthMode::DepthTest && includeDebugViewOnly && debugViewLineDrawer_) {
		debugViewLineDrawer_->Render();
	}

	if(depthMode == LineDepthMode::NoDepthTest && lineNoDepthDrawer_) {
		lineNoDepthDrawer_->Render();
	}

	if(depthMode == LineDepthMode::NoDepthTest && includeDebugViewOnly && debugViewLineNoDepthDrawer_) {
		debugViewLineNoDepthDrawer_->Render();
	}

	if(depthMode == LineDepthMode::DepthTest && boxDrawer_) {
		boxDrawer_->Render();
	}
#endif // _DEBUG


	
}

void PrimitiveDrawer::RenderEffectPreview() {
#if defined(_DEBUG) || defined(DEVELOP)
	if(effectPreviewLineDrawer_) {
		effectPreviewLineDrawer_->Render();
	}

	if(effectPreviewBoxDrawer_) {
		effectPreviewBoxDrawer_->Render();
	}
#endif // _DEBUG
}

void PrimitiveDrawer::ClearMesh(){
	if (lineDrawer_){
		lineDrawer_->Clear();
	}

	if(lineNoDepthDrawer_) {
		lineNoDepthDrawer_->Clear();
	}

	if(debugViewLineDrawer_) {
		debugViewLineDrawer_->Clear();
	}

	if(debugViewLineNoDepthDrawer_) {
		debugViewLineNoDepthDrawer_->Clear();
	}

	if (boxDrawer_) {
		boxDrawer_->Clear();
	}

	ClearEffectPreview();
}

void PrimitiveDrawer::ClearEffectPreview() {
	if(effectPreviewLineDrawer_) {
		effectPreviewLineDrawer_->Clear();
	}

	if(effectPreviewBoxDrawer_) {
		effectPreviewBoxDrawer_->Clear();
	}
}
