#pragma once
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <atomic>

/*-----------------------------------------------------------------------------------------
 * EventBus
 * - イベントバスクラス
 * - 型安全なイベントの発行・購読機能とRAII形式の接続管理を提供
 *---------------------------------------------------------------------------------------*/
class EventBus{
private:
	// 消去型ハンドラ
	using ErasedFn = std::function<void(const void*)>;
	struct HandlerEntry{
		size_t   id;
		ErasedFn fn;
	};
	using HandlerVec = std::vector<HandlerEntry>;

public:
	// ------------------------------------------------------------------
	//  subscribe から返る RAII オブジェクト
	// ------------------------------------------------------------------
	class Connection{
	public:
		Connection() = default;
		~Connection(){ disconnect(); }

		Connection(const Connection&) = delete;
		Connection& operator=(const Connection&) = delete;

		Connection(Connection&& other) noexcept{ swap(other); }
		Connection& operator=(Connection&& other) noexcept{ swap(other); return *this; }

		void disconnect(){
			if (!connected_) return;
			EventBus::unsubscribe(type_, id_);
			connected_ = false;
		}
		bool connected() const noexcept{ return connected_; }
	private:
		friend class EventBus;
		Connection(std::type_index t, size_t i)
			: type_(t), id_(i), connected_(true){}
		void swap(Connection& o){ std::swap(type_, o.type_); std::swap(id_, o.id_); std::swap(connected_, o.connected_); }

		std::type_index type_ {typeid(void)};
		size_t          id_ {0};
		bool            connected_ {false};
	};

	template<class Event>
	using Handler = std::function<void(const Event&)>;

	// ------------------------------------------------------------------
	// ハンドラ登録 → Connection 返却
	// ------------------------------------------------------------------
	template<class Event>
	static Connection Subscribe(Handler<Event> h){
		size_t id = nextId_++;
		auto& vec = handlers()[typeid(Event)];
		vec.push_back({id, [fn = std::move(h)] (const void* e){ fn(*static_cast< const Event* >(e)); }});
		return Connection {typeid(Event), id};
	}

	// ------------------------------------------------------------------
	// イベント発行
	// ------------------------------------------------------------------
	template<class Event>
	static void Publish(const Event& e){
		auto it = handlers().find(typeid(Event));
		if (it == handlers().end()) return;
		for (const auto& entry : it->second){ entry.fn(&e); }
	}

private:
	// id 指定でハンドラ削除
	static void unsubscribe(std::type_index key, size_t id){
		auto it = handlers().find(key);
		if (it == handlers().end()) return;
		auto& vec = it->second;
		vec.erase(std::remove_if(vec.begin(), vec.end(),
				  [id] (const HandlerEntry& e){ return e.id == id; }),
				  vec.end());
	}

	static std::unordered_map<std::type_index, HandlerVec>& handlers(){
		static std::unordered_map<std::type_index, HandlerVec> s;
		return s;
	}
	static inline std::atomic_size_t nextId_ {1};
};

// ============================================================================
// イベント型
// ============================================================================
struct ObjectAdded{ std::shared_ptr<class SceneObject> sp; };
struct ObjectRemoved{ std::shared_ptr<class SceneObject> sp; };
