//
// rangefield.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __RANGEFIELD_HPP__
#define __RANGEFIELD_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <map>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/assert.hpp>
#include <boost/thread/mutex.hpp>

namespace avhttp {

// 一个按区域来划分的位图实现, 该类对象线程访问安全.
class rangefield
{
public:
	// @param size表示区间的总大小.
	rangefield(boost::int64_t size = 0)
		: m_size(size)
		, m_need_gc(false)
	{}

	~rangefield()
	{}

public:

	///重置rangefield.
	// @param size表示区间的总大小.
	void reset(boost::int64_t size = 0)
	{
		boost::mutex::scoped_lock lock(m_mutex);

		m_size = size;
		m_need_gc = false;
		m_ranges.clear();
	}

	///获得区域的大小.
	// @返回这个区间的总大小.
	boost::int64_t size() const
	{
		return m_size;
	}

	///添加或更新range, 区间为[left, right)
	// @param left左边边界.
	// @param right右边边界, 不包含边界处.
	// @备注: 添加一个区间到range中, 可以重叠添加, 成功返回true.
	inline bool update(boost::int64_t left, boost::int64_t right)
	{
		BOOST_ASSERT((left >= 0 && left < right) && right <= m_size);

		boost::mutex::scoped_lock lock(m_mutex);

		if ((left < 0 || right > m_size) || (right <= left))
			return false;
		m_ranges[left] = right;
		m_need_gc = true;
		return true;
	}

	///是否在区间里.
	// @param left左边边界.
	// @param right右边边界, 不包含边界处.
	// @返回这个[left, right)这个区间是否完整的被包含在range中.
	// @备注: 检查的区间是一个半开区间[left, right), 即不包含右边界.
	inline bool in_range(boost::int64_t left, boost::int64_t right)
	{
		BOOST_ASSERT((left >= 0 && left < right) && right <= m_size);

		boost::mutex::scoped_lock lock(m_mutex);

		// 先整理.
		if (m_need_gc)
			gc();

		for (std::map<boost::int64_t, boost::int64_t>::iterator i = m_ranges.begin();
			i != m_ranges.end(); i++)
		{
			if (left >= i->first && right <= i->second)
				return true;
		}

		return false;
	}

	///输出空隙.
	// @param left 表示从左开始起始边界.
	// @param right 表示右边的边界, 不包括边界处.
	// @返回false表示没有空间或失败.
	// @备注: 输出的区间是一个半开区间[left, right), 即不包含右边界.
	inline bool out_space(boost::int64_t &left, boost::int64_t &right)
	{
		boost::mutex::scoped_lock lock(m_mutex);

		// 先整理.
		if (m_need_gc)
			gc();

		if (m_ranges.size() == 0)
		{
			left = 0;
			right = m_size;
			return true;
		}

		left = -1;
		right = -1;
		bool record = false;
		for (std::map<boost::int64_t, boost::int64_t>::iterator i = m_ranges.begin();
			i != m_ranges.end(); i++)
		{
			if (i == m_ranges.begin())
			{
				if (i->first != 0)
				{
					record = true;
					left = 0;
					right = i->first;
					return true;
				}
			}

			if (record)
			{
				right = i->first;
				break;
			}
			else
			{
				left = i->second;
				record = true;
				continue;
			}
		}

		if ((left != -1 && right == -1 && left != m_size) && m_ranges.size() == 1)
			right = m_size;
		if (left == -1 || right == -1)
			return false;

		return true;
	}

	///按指定大小输出为位图块.
	// @param bitfield以int为单位的位图数组, 每个元素表示1个piece, 为0表示空, 为1表示满.
	// @param piece_size指定的piece大小.
	inline void range_to_bitfield(std::vector<int> &bitfield, int piece_size)
	{
		// 先整理.
		if (m_need_gc)
			gc();

		int piece_num = (m_size / piece_size) + (m_size % piece_size == 0 ? 0 : 1);
		boost::int64_t l = 0;
		boost::int64_t r = 0;
		for (int i = 0; i < piece_num; i++)
		{
			l = i * piece_size;
			r = (i + 1) * piece_size;
			r = r > m_size ? m_size : r;

			if (in_range(l, r))
				bitfield.push_back(1);
			else
				bitfield.push_back(0);
		}
	}

	///输出range内容, 调试使用.
	inline void print()
	{
		boost::mutex::scoped_lock lock(m_mutex);

		for (std::map<boost::int64_t, boost::int64_t>::iterator i = m_ranges.begin();
			i != m_ranges.end(); i++)
		{
			std::cout << i->first << "   ---    " << i->second << "\n";
		}
	}

protected:
	///整理回收range中的重叠部分.
	inline void gc()
	{
		std::map<boost::int64_t, boost::int64_t> result;
		std::pair<boost::int64_t, boost::int64_t> max_value;
		max_value.first = 0;
		max_value.second = 0;
		for (std::map<boost::int64_t, boost::int64_t>::iterator i = m_ranges.begin();
			i != m_ranges.end(); i++)
		{
			// i 在max_value 之间, 忽略之.
			// [        ]
			//   [    ]
			// --------------
			// [         ]
			if (i->first >= max_value.first && i->second <= max_value.second)
				continue;
			// 情况如下:
			//      [   ]
			// [       ]
			// --------------
			// [        ]
			if (i->first <= max_value.first && (i->second >= max_value.first && i->second <= max_value.second))
			{
				max_value.first = i->first;	// 更新首部位置.
				continue;
			}
			// 情况如下:
			// [   ]
			//     [     ]
			// --------------
			// [         ]
			if ((i->first > max_value.first && i->first <= max_value.second))
			{
				max_value.second = i->second;
				continue;
			}
			// 情况如下:
			// [   ]
			//       [     ]
			// --------------
			// [   ] [     ]
			if ((i->first > max_value.second) || (max_value.first == 0 && max_value.second == 0))
			{
				if (max_value.first != 0 || max_value.second != 0)
					result.insert(max_value);
				max_value = *i;
			}
		}
		result.insert(max_value);
		m_ranges = result;
		m_need_gc = false;
	}

private:
	bool m_need_gc;
	boost::int64_t m_size;
	std::map<boost::int64_t, boost::int64_t> m_ranges;
	boost::mutex m_mutex;
};

} // namespace avhttp

#endif // __RANGEFIELD_HPP__
