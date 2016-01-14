#ifndef __LogData__hh__
#define __LogData__hh__

#include <string>
#include <tuple>
#include <cstddef>
#include <map>
#include <stdint.h>
#include <utility>
#include <mutex>

typedef uint64_t key_type;

class LogData {
private:
	typedef std::map<::key_type, std::size_t> impl_type;
	impl_type impl;
public:
	typedef impl_type::const_iterator const_iterator;
	typedef impl_type::iterator iterator;
	typedef impl_type::key_type key_type;
	typedef impl_type::value_type value_type;

	virtual ~LogData();
	virtual bool add(key_type&&, std::size_t);
	virtual bool add(const key_type&, std::size_t);
	const_iterator lower_bound(const key_type&) const;
	const_iterator begin() const;
	iterator begin();
	const_iterator end() const;
	iterator end();
};

class SyncLogData : public LogData {
private:
	std::mutex mtx;
public:
	virtual bool add(key_type&&, std::size_t);
	virtual bool add(const key_type&, std::size_t);
};

inline
LogData::~LogData() {
}

inline bool
LogData::add(key_type&& k, std::size_t v) {
	std::pair<impl_type::iterator, bool> p = impl.insert(impl_type::value_type(std::move(k), v));
	if (!p.second)
		p.first->second += v;
	return p.second;
}

inline bool
LogData::add(const key_type& k, std::size_t v) {
	std::pair<impl_type::iterator, bool> p = impl.insert(impl_type::value_type(k, v));
	if (!p.second)
		p.first->second += v;
	return p.second;
}

inline LogData::const_iterator
LogData::begin() const {
	return impl.begin();
}

inline LogData::iterator
LogData::begin() {
	return impl.begin();
}

inline LogData::const_iterator
LogData::end() const {
	return impl.end();
}

inline LogData::iterator
LogData::end() {
	return impl.end();
}

inline LogData::const_iterator
LogData::lower_bound(const key_type& k) const {
	return impl.lower_bound(k);
}

inline bool
SyncLogData::add(key_type&& k, std::size_t v) {
	std::unique_lock<std::mutex> lck(mtx);
	return LogData::add(std::move(k), v);
}

inline bool
SyncLogData::add(const key_type& k, std::size_t v) {
	std::unique_lock<std::mutex> lck(mtx);
	return LogData::add(k, v);
}

#endif
