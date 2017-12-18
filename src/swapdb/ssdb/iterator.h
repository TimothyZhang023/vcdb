/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef SSDB_ITERATOR_H_
#define SSDB_ITERATOR_H_

#include <string>
#include "util/bytes.h"

namespace rocksdb {
	class Iterator;
	class Snapshot;
}

class Iterator{
public:
	enum Direction{
		FORWARD, BACKWARD
	};
	Iterator(rocksdb::Iterator *it,
			const std::string &end,
			uint64_t limit,
			Direction direction=Iterator::FORWARD,
			const rocksdb::Snapshot *snapshot=nullptr
	);
	~Iterator();
	bool skip(uint64_t offset);
	bool next();
	Bytes key();
	Bytes val();

	const rocksdb::Snapshot *snapshot;
private:
	rocksdb::Iterator *it;
	std::string end;
	uint64_t limit;
	bool is_first;
	int direction;
};



class MIterator{
public:
	Bytes key;
	Bytes val;

	MIterator(Iterator *it);
	~MIterator();
	bool next();
private:
	Iterator *it;
};


class HIterator{
public:
	std::string name;
	Bytes 		key;
	Bytes 		val;
	uint16_t 	version;

	HIterator(Iterator *it, const Bytes &name, uint16_t version = 0);
	~HIterator();
	void return_val(bool onoff);
	bool next();
private:
	Iterator *it;
	bool return_val_;
};


class SIterator{
public:
	std::string name;
	Bytes 		key;
	uint16_t 	version;

	SIterator(Iterator *it, const Bytes &name, uint16_t version = 0);
	~SIterator();
	bool next();

private:
	Iterator	*it;
};


class ZIterator{
public:
	std::string name;
	Bytes		key;
	double      score;

	uint16_t 	version;

	ZIterator(Iterator *it, const Bytes &name, uint16_t version);
	~ZIterator();
	bool skip(uint64_t offset);
	bool next();
private:
	Iterator *it;
};

class ZIteratorByLex{
public:
	std::string name;
	Bytes		key;
	uint16_t 	version;

	ZIteratorByLex(Iterator *it, const Bytes &name, uint16_t version);
	~ZIteratorByLex();
	bool skip(uint64_t offset);
	bool next();
private:
	Iterator *it;
};

class LIterator{
public:
	std::string name;
	uint64_t    seq;
	uint16_t 	version;

	LIterator(Iterator *it, const Bytes &name, uint16_t version = 0);
	~LIterator();
	bool next();

private:
	Iterator	*it;
};



class EIterator{
public:
	Bytes		 key;
	int64_t      score;

	EIterator(Iterator *it);
	~EIterator();
	bool skip(uint64_t offset);
	bool next();
private:
	Iterator *it;
};


#endif
