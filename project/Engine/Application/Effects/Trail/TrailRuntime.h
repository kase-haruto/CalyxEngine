#pragma once

#include <Data/Engine/Configs/Scene/Objects/Particle/TrailConfig.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>
#include <Engine/Objects/3D/Mesh/MeshData.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace CalyxEngine {

	struct TrailPoint {
		Vector3 position{};
		Quaternion rotation = Quaternion::MakeIdentity();
		float spawnTime = 0.0f;
		float cumulativeDistance = 0.0f;
		uint32_t randomSeed = 0;
	};

	struct TrailVertex {
		Vector3 position{};
		Vector2 uv{};
		Vector4 color{1,1,1,1};
		float normalizedAge = 0.0f;
		float normalizedLength = 0.0f;
		float randomValue = 0.0f;
		float emissiveIntensity = 0.0f;
	};

	struct TrailMeshData {
		std::vector<TrailVertex> vertices;
		std::vector<uint32_t> indices;
		void Clear() { vertices.clear(); indices.clear(); }
	};

	/*------------------------------------------------------------
	    TrailPointの寿命、点数上限、累積距離を管理する。
	    Transform参照およびGPUリソースは保持しない。
	------------------------------------------------------------*/
	class TrailHistory {
	public:
		void Clear() { points_.clear(); }
		void Reserve(size_t count) { points_.reserve(count); }
		const std::vector<TrailPoint>& GetPoints() const { return points_; }
		bool Empty() const { return points_.empty(); }

		void Add(const Vector3& position,const Quaternion& rotation,float time,uint32_t seed) {
			TrailPoint point{};
			point.position = position;
			point.rotation = rotation;
			point.spawnTime = time;
			point.randomSeed = seed;
			if(!points_.empty())
				point.cumulativeDistance = points_.back().cumulativeDistance + (position - points_.back().position).Length();
			points_.push_back(point);
		}

		void Update(float time,float lifetime,uint32_t maxPointCount) {
			const float safeLifetime = (std::max)(lifetime,0.001f);
			std::erase_if(points_,[&](const TrailPoint& p) { return time - p.spawnTime >= safeLifetime; });
			if(points_.size() > maxPointCount)
				points_.erase(points_.begin(),points_.begin() + (points_.size() - maxPointCount));
		}

	private:
		std::vector<TrailPoint> points_;
	};

	/*------------------------------------------------------------
	    追従位置を距離・時間条件でサンプリングしてTrailHistoryへ記録する。
	    描画処理やTransformの生ポインタは保持しない。
	------------------------------------------------------------*/
	class TrailEmitter {
	public:
		TrailSettingsConfig& Settings() { return settings_; }
		const TrailSettingsConfig& Settings() const { return settings_; }
		const TrailHistory& History() const { return history_; }
		float GetTime() const { return currentTime_; }

		void ApplySettings(const TrailSettingsConfig& settings) {
			settings_ = settings;
			history_.Reserve(settings_.maxPointCount);
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//    現在位置を距離・時間条件でサンプリングし、必要なら区間を補間する
		/////////////////////////////////////////////////////////////////////////////////////////
		void Update(const Vector3& position,const Quaternion& rotation,float deltaTime,bool emitting) {
			currentTime_ += (std::max)(deltaTime,0.0f);
			if(!settings_.enabled) { Reset(); return; }
			history_.Update(currentTime_,settings_.lifetime,settings_.maxPointCount);
			if(!emitting) { hasSample_ = false; return; }

			if(!hasSample_) {
				AddPoint(position,rotation,currentTime_);
				lastSamplePosition_ = position;
				lastSampleRotation_ = rotation;
				lastSampleTime_ = currentTime_;
				hasSample_ = true;
				return;
			}

			const Vector3 delta = position - lastSamplePosition_;
			const float distance = delta.Length();
			const bool movedEnough = distance >= settings_.minSampleDistance;
			const bool waitedTooLong = currentTime_ - lastSampleTime_ >= settings_.maxSampleInterval;
			if(!movedEnough && !waitedTooLong) return;

			const uint32_t divisions = movedEnough
				? (std::max)(1u,static_cast<uint32_t>(std::ceil(distance / settings_.minSampleDistance)))
				: 1u;
			for(uint32_t i = 1; i <= divisions; ++i) {
				const float t = static_cast<float>(i) / static_cast<float>(divisions);
				AddPoint(Vector3::Lerp(lastSamplePosition_,position,t),Quaternion::Slerp(lastSampleRotation_,rotation,t),
					std::lerp(lastSampleTime_,currentTime_,t));
			}
			lastSamplePosition_ = position;
			lastSampleRotation_ = rotation;
			lastSampleTime_ = currentTime_;
			history_.Update(currentTime_,settings_.lifetime,settings_.maxPointCount);
		}

		void StopSampling() { hasSample_ = false; }
		void Reset() { history_.Clear(); hasSample_ = false; currentTime_ = 0.0f; pointSequence_ = 0; }

	private:
		void AddPoint(const Vector3& p,const Quaternion& q,float time) {
			history_.Add(p,q,time,HashParticleSeed(++pointSequence_));
		}

		TrailSettingsConfig settings_{};
		TrailHistory history_{};
		Vector3 lastSamplePosition_{};
		Quaternion lastSampleRotation_ = Quaternion::MakeIdentity();
		float currentTime_ = 0.0f;
		float lastSampleTime_ = 0.0f;
		uint32_t pointSequence_ = 0;
		bool hasSample_ = false;
	};

	/*------------------------------------------------------------
	    TrailPoint列からCamera Facing等の連続Ribbon頂点を生成する。
	    生成結果は一時CPUデータでありGPUリソースを所有しない。
	------------------------------------------------------------*/
	class TrailMeshBuilder {
	public:
		static void Build(const TrailEmitter& emitter,const Vector3& cameraPosition,TrailMeshData& output,const MeshResource* geometryMesh=nullptr) {
			output.Clear();
			const auto& settings = emitter.Settings();
			const auto& source = emitter.History().GetPoints();
			if(!settings.enabled || source.size() < 2) return;

			std::vector<TrailPoint> samples;
			BuildSamples(source,settings,samples);
			if(samples.size() < 2) return;
			if(geometryMesh && !geometryMesh->Vertices().empty()) {
				if(settings.geometryMode==TrailGeometryMode::MeshExtrusion) {
					BuildExtrusion(emitter,samples,cameraPosition,*geometryMesh,output);
					if(!output.vertices.empty()) return;
				}
				if(settings.geometryMode==TrailGeometryMode::MeshInstances && !geometryMesh->Indices().empty()) {
					BuildInstances(emitter,samples,cameraPosition,*geometryMesh,output);
					if(!output.vertices.empty()) return;
				}
			}
			const uint32_t ribbonCount = settings.facingMode == TrailFacingMode::Cross ? 2u : 1u;
			output.vertices.reserve(samples.size() * 2u * ribbonCount);
			output.indices.reserve((samples.size() - 1u) * 6u * ribbonCount);
			const float startDistance = samples.front().cumulativeDistance;
			const float totalDistance = (std::max)(samples.back().cumulativeDistance - startDistance,0.0001f);

			for(uint32_t ribbon = 0; ribbon < ribbonCount; ++ribbon) {
				Vector3 previousSide = ribbon == 0 ? Vector3::Right() : Vector3::Up();
				const uint32_t baseVertex = static_cast<uint32_t>(output.vertices.size());
				for(size_t i = 0; i < samples.size(); ++i) {
					const TrailPoint& point = samples[i];
					Vector3 tangent = CalculateTangent(samples,i);
					Vector3 side = CalculateSide(settings,point,tangent,cameraPosition,ribbon,previousSide);
					if(Vector3::Dot(previousSide,side) < 0.0f) side = -side;
					previousSide = side;

					const float age = std::clamp((emitter.GetTime() - point.spawnTime) / settings.lifetime,0.0f,1.0f);
					const float length = (point.cumulativeDistance - startDistance) / totalDistance;
					const float random = ParticleRandom01(point.randomSeed,0u);
					const float width = settings.baseWidth * (std::max)(settings.widthOverLifetime.Evaluate(age,random),0.0f);
					Vector4 color = settings.colorOverLifetime.Evaluate(age);
					color.w *= settings.alphaOverLifetime.Evaluate(age,random);
					const float emissive = settings.emissiveOverLifetime.Evaluate(age,random);
					const float u = settings.uvMode == TrailUVMode::Distance
						? (point.cumulativeDistance - startDistance) * settings.uvTiling
						: length * settings.uvTiling;
					const Vector3 halfSide = side * (width * 0.5f);
					output.vertices.push_back({point.position - halfSide,{u,0.0f},color,age,length,random,emissive});
					output.vertices.push_back({point.position + halfSide,{u,1.0f},color,age,length,random,emissive});
				}
				for(uint32_t i = 0; i + 1u < samples.size(); ++i) {
					const uint32_t l0 = baseVertex + i * 2u;
					const uint32_t r0 = l0 + 1u;
					const uint32_t l1 = l0 + 2u;
					const uint32_t r1 = l0 + 3u;
					output.indices.insert(output.indices.end(),{l0,l1,r0,r0,l1,r1});
				}
			}
		}

	private:
		static TrailVertex MakeTrailVertex(const TrailEmitter& emitter,const TrailPoint& point,
			const Vector3& position,const Vector2& uv,float normalizedLength) {
			const auto& settings=emitter.Settings();
			const float age=std::clamp((emitter.GetTime()-point.spawnTime)/settings.lifetime,0.0f,1.0f);
			const float random=ParticleRandom01(point.randomSeed,0u);
			Vector4 color=settings.colorOverLifetime.Evaluate(age);
			color.w*=settings.alphaOverLifetime.Evaluate(age,random);
			return {position,uv,color,age,normalizedLength,random,settings.emissiveOverLifetime.Evaluate(age,random)};
		}

		static Vector3 CalculateFrameSide(const TrailSettingsConfig& settings,const TrailPoint& point,
			const Vector3& tangent,const Vector3& cameraPosition,const Vector3& fallback) {
			return CalculateSide(settings,point,tangent,cameraPosition,0u,fallback);
		}

		static void BuildExtrusion(const TrailEmitter& emitter,const std::vector<TrailPoint>& samples,
			const Vector3& cameraPosition,const MeshResource& mesh,TrailMeshData& output) {
			const auto& settings=emitter.Settings();
			std::vector<Vector2> section;
			section.reserve(mesh.Vertices().size());
			for(const auto& vertex:mesh.Vertices()) {
				const Vector2 p{vertex.position.x,vertex.position.y};
				const bool duplicate=std::any_of(section.begin(),section.end(),[&](const Vector2& v){const float dx=v.x-p.x,dy=v.y-p.y;return dx*dx+dy*dy<0.000001f;});
				if(!duplicate) section.push_back(p);
			}
			if(section.size()<2) return;
			Vector2 center{}; for(const auto& p:section){center.x+=p.x;center.y+=p.y;} center.x/=section.size();center.y/=section.size();
			std::sort(section.begin(),section.end(),[&](const Vector2& a,const Vector2& b){return std::atan2(a.y-center.y,a.x-center.x)<std::atan2(b.y-center.y,b.x-center.x);});
			const float startDistance=samples.front().cumulativeDistance;
			const float totalDistance=(std::max)(samples.back().cumulativeDistance-startDistance,0.0001f);
			const uint32_t ringSize=static_cast<uint32_t>(section.size());
			output.vertices.reserve(samples.size()*ringSize);
			Vector3 previousSide=Vector3::Right();
			for(size_t i=0;i<samples.size();++i) {
				const auto& point=samples[i]; const Vector3 tangent=CalculateTangent(samples,i);
				Vector3 side=CalculateFrameSide(settings,point,tangent,cameraPosition,previousSide);
				if(Vector3::Dot(previousSide,side)<0) side=-side; previousSide=side;
				Vector3 up=Vector3::Cross(side,tangent); if(up.LengthSquared()<0.000001f) up=Vector3::Up(); else up=up.Normalize();
				const float length=(point.cumulativeDistance-startDistance)/totalDistance;
				const float age=std::clamp((emitter.GetTime()-point.spawnTime)/settings.lifetime,0.0f,1.0f);
				const float width=settings.baseWidth*(std::max)(settings.widthOverLifetime.Evaluate(age,ParticleRandom01(point.randomSeed,0)),0.0f);
				const float u=settings.uvMode==TrailUVMode::Distance?(point.cumulativeDistance-startDistance)*settings.uvTiling:length*settings.uvTiling;
				for(uint32_t s=0;s<ringSize;++s) {
					const Vector2 local=section[s];
					const Vector3 position=point.position+side*(local.x*settings.geometryScale.x*width)+up*(local.y*settings.geometryScale.y*width);
					output.vertices.push_back(MakeTrailVertex(emitter,point,position,{u,static_cast<float>(s)/(settings.closeCrossSection?ringSize:(std::max)(ringSize-1u,1u))},length));
				}
			}
			const uint32_t edgeCount=settings.closeCrossSection?ringSize:ringSize-1u;
			for(uint32_t r=0;r+1u<samples.size();++r) for(uint32_t s=0;s<edgeCount;++s) {
				const uint32_t next=(s+1u)%ringSize,a=r*ringSize+s,b=(r+1u)*ringSize+s,c=r*ringSize+next,d=(r+1u)*ringSize+next;
				output.indices.insert(output.indices.end(),{a,b,c,c,b,d});
			}
		}

		static void BuildInstances(const TrailEmitter& emitter,const std::vector<TrailPoint>& samples,
			const Vector3& cameraPosition,const MeshResource& mesh,TrailMeshData& output) {
			const auto& settings=emitter.Settings();
			const uint32_t modelVertexCount=static_cast<uint32_t>(mesh.Vertices().size());
			const float startDistance=samples.front().cumulativeDistance;
			const float totalDistance=(std::max)(samples.back().cumulativeDistance-startDistance,0.0001f);
			output.vertices.reserve(samples.size()*modelVertexCount); output.indices.reserve(samples.size()*mesh.Indices().size());
			Vector3 previousSide=Vector3::Right();
			for(size_t i=0;i<samples.size();++i) {
				const auto& point=samples[i]; Vector3 tangent=CalculateTangent(samples,i);
				Vector3 side=CalculateFrameSide(settings,point,tangent,cameraPosition,previousSide); if(Vector3::Dot(previousSide,side)<0)side=-side;previousSide=side;
				Vector3 up=Vector3::Cross(side,tangent); if(up.LengthSquared()<0.000001f)up=Vector3::Up();else up=up.Normalize();
				if(!settings.alignInstancesToTangent){side=Quaternion::RotateVector(Vector3::Right(),point.rotation);up=Quaternion::RotateVector(Vector3::Up(),point.rotation);tangent=Quaternion::RotateVector(Vector3::Forward(),point.rotation);}
				const float length=(point.cumulativeDistance-startDistance)/totalDistance;
				const float age=std::clamp((emitter.GetTime()-point.spawnTime)/settings.lifetime,0.0f,1.0f);
				const float width=settings.baseWidth*(std::max)(settings.widthOverLifetime.Evaluate(age,ParticleRandom01(point.randomSeed,0)),0.0f);
				for(const auto& source:mesh.Vertices()) {
					// Model local +Z follows the trail tangent. Width controls the local XY cross-section only.
					const Vector3 local{source.position.x*settings.geometryScale.x*width,source.position.y*settings.geometryScale.y*width,source.position.z*settings.geometryScale.z};
					output.vertices.push_back(MakeTrailVertex(emitter,point,point.position+side*local.x+up*local.y+tangent*local.z,source.texcoord,length));
				}
				const uint32_t base=static_cast<uint32_t>(i)*modelVertexCount; for(uint32_t index:mesh.Indices()) output.indices.push_back(base+index);
			}
		}

		static Vector3 CatmullRom(const Vector3& p0,const Vector3& p1,const Vector3& p2,const Vector3& p3,float t) {
			const float t2=t*t,t3=t2*t;
			return (p1*2.0f + (p2-p0)*t + (p0*2.0f-p1*5.0f+p2*4.0f-p3)*t2 + (-p0+p1*3.0f-p2*3.0f+p3)*t3)*0.5f;
		}

		static void BuildSamples(const std::vector<TrailPoint>& source,const TrailSettingsConfig& settings,std::vector<TrailPoint>& out) {
			if(!settings.useSpline || source.size() < 3 || settings.splineSubdivision <= 1) { out = source; return; }
			out.reserve((source.size()-1u)*settings.splineSubdivision+1u);
			for(size_t i=0;i+1<source.size();++i) {
				const auto& p0=source[i==0?0:i-1]; const auto& p1=source[i];
				const auto& p2=source[i+1]; const auto& p3=source[(std::min)(i+2,source.size()-1)];
				for(uint32_t s=0;s<settings.splineSubdivision;++s) {
					const float t=static_cast<float>(s)/settings.splineSubdivision;
					TrailPoint p=p1; p.position=CatmullRom(p0.position,p1.position,p2.position,p3.position,t);
					p.spawnTime=std::lerp(p1.spawnTime,p2.spawnTime,t); p.cumulativeDistance=std::lerp(p1.cumulativeDistance,p2.cumulativeDistance,t);
					out.push_back(p);
				}
			}
			out.push_back(source.back());
		}

		static Vector3 CalculateTangent(const std::vector<TrailPoint>& p,size_t i) {
			Vector3 tangent = i==0 ? p[1].position-p[0].position : (i+1==p.size()?p[i].position-p[i-1].position:p[i+1].position-p[i-1].position);
			return tangent.LengthSquared()>0.000001f?tangent.Normalize():Vector3::Forward();
		}

		static Vector3 CalculateSide(const TrailSettingsConfig& s,const TrailPoint& p,const Vector3& tangent,const Vector3& camera,uint32_t ribbon,const Vector3& fallback) {
			Vector3 reference;
			if(s.facingMode==TrailFacingMode::LocalAxis) reference=Quaternion::RotateVector(s.localAxis,p.rotation);
			else if(s.facingMode==TrailFacingMode::WorldAxis) reference=s.worldAxis;
			else if(s.facingMode==TrailFacingMode::Cross && ribbon==1) reference=Vector3::Up();
			else { Vector3 view=camera-p.position; reference=view.LengthSquared()>0.000001f?view.Normalize():Vector3::Up(); }
			Vector3 side=Vector3::Cross(tangent,reference);
			if(side.LengthSquared()<=0.000001f) side=fallback;
			return side.LengthSquared()>0.000001f?side.Normalize():Vector3::Right();
		}
	};
}

template<>
struct VertexInputLayout<CalyxEngine::TrailVertex> {
	static std::vector<D3D12_INPUT_ELEMENT_DESC> Get() {
		return {
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",1,DXGI_FORMAT_R32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",2,DXGI_FORMAT_R32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",3,DXGI_FORMAT_R32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
			{"TEXCOORD",4,DXGI_FORMAT_R32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
	}
};
