//
// Created by int3 on 19.10.16.
//

#pragma once

#include <chrono>
#include <functional>
#include <list>
#include <algorithm>
#include <random>

template <class T, class BaseTimer = std::chrono::high_resolution_clock>
class Heart {
	static double RandomDouble(double from, double to) {
		thread_local std::mt19937 engine(std::random_device{}());
		thread_local std::uniform_real_distribution<double> dist;
		return from + dist(engine) * (to - from);
	}

	using period_t = std::chrono::microseconds;
	using callback_method_t = void (T::*)();
	using callback_func_t = std::function<void()>;

	class Node {
		using callback_t = std::function<void()>;

		int32_t _id;
		period_t _delta;
		period_t _current_delta;
		period_t _elapsed;
		period_t _random;
		bool _once;
		bool _removed;

		callback_t _callback;

		void UpdateCurrentPeriod() {
			_current_delta = _delta + std::chrono::duration_cast<period_t>(RandomDouble(-1, 1) * _random);
		}
	public:
		Node(period_t delta, callback_t cb, bool once) : _id(0), _delta(delta), _current_delta(0), _elapsed(0), _random(0), _once(once), _removed(false), _callback(cb) {
			UpdateCurrentPeriod();
		}

		void Beat(period_t step) {
			if (_removed) return;

			_elapsed += step;
			if (_once) {
				if (_elapsed >= _current_delta) {
					_callback();
					_removed = true;
				}
			}
			else {
				while (_elapsed >= _current_delta) {
					_elapsed -= _current_delta;
					_callback();
					UpdateCurrentPeriod();
				}
			}
		}

		void Remove() {
			_removed = true;
		}

		bool IsRemoved() const {
			return _removed;
		}

		void SetId(int32_t id) {
			_id = id;
		}

		int32_t GetId() const {
			return _id;
		}

		Node& SetDelta(period_t delta, period_t random = {}) {
			_delta = delta;
			return SetRandom(random);
		}

		Node& SetRandom(period_t random) {
			_random = random;
			UpdateCurrentPeriod();
			return *this;
		}

		Node& SetOnce(bool once = true) {
			_once = once;
			return *this;
		}
	};

	BaseTimer _clock;
	typename BaseTimer::time_point _last;

	T* _obj;
	std::list<Node> _nodes;

	void Call(callback_method_t cb) {
		(_obj->*cb)();
	}
public:
	Heart(T *obj) : _obj(obj) {
		_last = _clock.now();
	}

	Heart(const Heart&) = delete;
	Heart& operator=(const Heart&) = delete;

	Node& Push(callback_func_t cb) {
		return Push(std::chrono::seconds(1'000'000'000), cb);
	}

	Node& Push(callback_method_t cb) {
		return Push(std::chrono::seconds(1'000'000'000), cb);
	}

	Node& Push(period_t period, callback_func_t cb) {
		_nodes.emplace_back(period, cb, false);
		return _nodes.back();
	}

	Node& Push(period_t period, callback_method_t cb) {
		return Push(period, [=] { Call(cb); });
	}

	Node& PushOnce(period_t delta, callback_func_t cb) {
		_nodes.emplace_back(delta, cb, true);
		return _nodes.back().SetOnce(true);
	}

	Node& PushOnce(period_t period, callback_method_t cb) {
		return PushOnce(period, [=] { Call(cb); });
	}

	// делает Push через wait
	Node& DelayedPush(period_t wait, period_t period, callback_func_t cb) {
		return PushOnce(wait, [=]{
			Push(period, cb);
		});
	}

	Node& DelayedPush(period_t wait, period_t period, callback_method_t cb) {
		return DelayedPush(wait, period, [=] { Call(cb); });
	}

	// делает первый вызов через first, далее каждые period сек
	Node& Push(period_t first, period_t period, callback_func_t cb) {
		return PushOnce(first, [=]{
			cb();
			Push(period, cb);
		});
	}

	Node& Push(period_t first, period_t period, callback_method_t cb) {
		return Push(first, period, [=] { Call(cb); });
	}

	void Beat() {
		auto now = _clock.now();
		auto step = now - _last;
		_last = now;
		for (Node& node : _nodes) {
			node.Beat(std::chrono::duration_cast<period_t>(step));
		}
		_nodes.remove_if([](const Node& node) -> bool { return node.IsRemoved(); });
	}

	void Remove(int32_t id) {
		for (Node& node : _nodes) {
			if (node.GetId() == id) node.Remove();
		}
	}

	bool Has(int32_t id) const {
		return std::any_of(_nodes.begin(), _nodes.end(), [id](const Node& node) {
			return node.GetId() == id;
		});
	}
};