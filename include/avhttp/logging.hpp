//
// Copyright (C) 2013 Jack.
//
// Author: jack
// Email:  jack.wgm@gmail.com
//

#ifndef LOGGING_HPP
#define LOGGING_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <iostream>
#include <string>
#include <fstream>

#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace avhttp {

	///内部使用的简易日志类.
	// 使用说明:
	//	在程序入口(如:main)函数调用 INIT_LOGGER 宏, 它有两个参数, 第一个参数指定了日志文件保存
	//	的路径, 第二个参数指定了日志文件保存的文件名, 详细见INIT_LOGGER.
	//	然后就可以使用LOG_DBG/LOG_INFO/LOG_WARN/LOG_ERR这几个宏来输出日志信息.
	// @begin example
	//  #include "logging.hpp"
	//  int main()
	//  {
	//     AVHTTP_AUTO_LOGGER(".");					// 在当前目录创建以日期命名的日志文件.
	//     // 也可 AVHTTP_INIT_LOGGER("example.log");	// 指定日志文件名.
	//     AVHTTP_LOG_DEBUG << "Initialized.";
	//     std::string result = do_something();
	//     AVHTTP_LOG_DEBUG << "do_something return : " << result;	// 输出do_something返回结果到日志.
	//     ...
	//  }
	// @end example

	class auto_logger_file
	{
	public:
		auto_logger_file()
			: m_auto_mode(false)
		{}
		~auto_logger_file()
		{}

		typedef boost::shared_ptr<std::ofstream> ofstream_ptr;
		typedef std::map<std::string, ofstream_ptr> loglist;

		enum {
			file_num = 3
		};

		void open(const char * filename, std::ios_base::openmode flag)
		{
			m_auto_mode = false;
			const char* pos = std::strstr(filename, "*");
			if (pos)
			{
				m_auto_mode = true;
				char save_path[65536] = { 0 };
				int len = pos - filename;
				if (len < 0 || len > 65536)
					return;
				strncpy(save_path, filename, pos - filename);
				m_log_path = save_path;
			}
			else
			{
				m_file.open(filename, flag);
			}
			std::string start_string = "\n\n\n*** starting log ***\n\n\n";
			write(start_string.c_str(), start_string.size());
		}

		bool is_open() const
		{
			if (m_auto_mode) return true;
			return m_file.is_open();
		}

		void write(const char* str, std::streamsize size)
		{
			if (!m_auto_mode)
			{
				m_file.write(str, size);
				return;
			}

			ofstream_ptr of;
			std::string fn = make_filename(m_log_path.string());
			loglist::iterator iter = m_log_list.find(fn);
			if (iter == m_log_list.end())
			{
				of.reset(new std::ofstream);
				of->open(fn.c_str(), std::ios_base::out | std::ios_base::app);
				m_log_list.insert(std::make_pair(fn, of));
				if (m_log_list.size() > file_num)
				{
					iter = m_log_list.begin();
					fn = iter->first;
					ofstream_ptr f = iter->second;
					m_log_list.erase(iter);
					f->close();
					f.reset();
					boost::system::error_code ignore_ec;
					boost::filesystem::remove(fn, ignore_ec);
					if (ignore_ec)
						std::cout << "delete log failed: " << fn <<
							", error code: " << ignore_ec.message() << std::endl;
				}
			}
			else
			{
				of = iter->second;
			}

			if (of->is_open())
			{
				(*of).write(str, size);
				(*of).flush();
			}
		}

		void flush()
		{
			m_file.flush();
		}

		std::string make_filename(const std::string &p = "") const
		{
			boost::posix_time::ptime time = boost::posix_time::second_clock::local_time();
			if (m_last_day != boost::posix_time::not_a_date_time && m_last_day.date().day() == time.date().day())
				return m_last_filename;
			m_last_day = time;
			std::ostringstream oss;
			boost::posix_time::time_facet* _facet = new boost::posix_time::time_facet("%Y%m%d");
			oss.imbue(std::locale(std::locale::classic(), _facet));
			oss << boost::posix_time::second_clock::local_time();
			if (!boost::filesystem::exists(p)) {
				boost::system::error_code ignore_ec;
				boost::filesystem::create_directories(p, ignore_ec);
			}
			m_last_filename = (boost::filesystem::path(p) / (oss.str() + ".log")).string();
			return m_last_filename;
		}

	private:
		std::fstream m_file;
		bool m_auto_mode;
		boost::filesystem::path m_log_path;
		loglist m_log_list;
		mutable boost::posix_time::ptime m_last_day;
		mutable std::string m_last_filename;
	};

	namespace aux {
		template<class Lock>
		Lock& lock_single()
		{
			static Lock lock_instance;
			return lock_instance;
		}

		template<class Writer>
		Writer& writer_single()
		{
			static Writer writer_instance;
			return writer_instance;
		}

		inline char const* time_now_string()
		{
			std::ostringstream oss;
			boost::posix_time::time_facet* _facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S.%f");
			oss.imbue(std::locale(std::locale::classic(), _facet));
			oss << boost::posix_time::microsec_clock::local_time();
			std::string s = oss.str();
			if (s.size() > 3)
				s = std::string(s.substr(0, s.size() - 3));
			static char str[200];
			std::sprintf(str, "%s ", s.c_str());
			return str;
		}
	}

#ifdef LOGGER_THREAD_SAFE
#	define LOGGER_LOCKS_() boost::mutex::scoped_lock lock(aux::lock_single<boost::mutex>())
#else
#	define LOGGER_LOCKS_() ((void)0)
#endif // LOGGER_THREAD_SAFE

#if defined(WIN32) && defined(LOGGER_DBG_VIEW)
#	define LOGGER_DBG_VIEW_(x) do { ::OutputDebugStringA(x.c_str()); } while (0)
#else
#	define LOGGER_DBG_VIEW_(x) ((void)0)
#endif // WIN32 && LOGGER_DBG_VIEW

	inline void logger_writer(std::string& level, std::string& message, bool disable_cout = false)
	{
		LOGGER_LOCKS_();
		std::ostringstream oss;
		oss << aux::time_now_string() << "[" << level << "]: " << message << std::endl;
		std::string tmp = oss.str();
		if (aux::writer_single<auto_logger_file>().is_open())
		{
			aux::writer_single<auto_logger_file>().write(tmp.c_str(), tmp.size());
			aux::writer_single<auto_logger_file>().flush();
		}
		LOGGER_DBG_VIEW_(tmp);
#ifndef AVHTTP_DISABLE_LOGGER_TO_CONSOLE
		if (!disable_cout)
		{
			std::cout << tmp;
			std::cout.flush();
		}
#endif
	}

	class logger : boost::noncopyable
	{
	public:
		logger(std::string& level)
			: level_(level)
			, m_disable_cout(false)
		{}
		logger(std::string& level, bool disable_cout)
			: level_(level)
			, m_disable_cout(disable_cout)
		{}
		~logger()
		{
			logger_writer(level_, std::string(oss_.str()), m_disable_cout);
		}

		template <class T>
		logger& operator << (T const& v)
		{
			oss_ << v;
			return *this;
		}

		std::ostringstream oss_;
		std::string& level_;
		bool m_disable_cout;
	};

	static std::string LOGGER_DEBUG_STR = "DEBUG";
	static std::string LOGGER_INFO_STR = "INFO";
	static std::string LOGGER_WARN_STR = "WARNING";
	static std::string LOGGER_ERR_STR = "ERROR";
	static std::string LOGGER_FILE_STR = "FILE";

} // namespace avhttp

#define AVHTTP_INIT_LOGGER(logfile) do \
	{ \
		avhttp::auto_logger_file& file = avhttp::aux::writer_single<avhttp::auto_logger_file>(); \
		std::string filename = logfile; \
		if (!filename.empty()) \
			file.open(filename.c_str(), std::ios::in | std::ios::out | std::ios::app); \
	} while (0)

#define AVHTTP_AUTO_LOGGER(path) do \
	{ \
	avhttp::auto_logger_file& file = avhttp::aux::writer_single<avhttp::auto_logger_file>(); \
		std::string filename = "*"; \
		filename = std::string(path) + filename; \
		if (!filename.empty()) \
			file.open(filename.c_str(), std::ios::in | std::ios::out | std::ios::app); \
	} while (0)


#if defined(DEBUG) || defined(_DEBUG) || defined(ENABLE_LOGGER)

#define AVHTTP_LOG_DBG avhttp::logger(avhttp::LOGGER_DEBUG_STR)
#define AVHTTP_LOG_INFO avhttp::logger(avhttp::LOGGER_INFO_STR)
#define AVHTTP_LOG_WARN avhttp::logger(avhttp::LOGGER_WARN_STR)
#define AVHTTP_LOG_ERR avhttp::logger(avhttp::LOGGER_ERR_STR)
#define AVHTTP_LOG_FILE avhttp::logger(avhttp::LOGGER_FILE_STR, true)

#else

#define AVHTTP_LOG_DBG(message) ((void)0)
#define AVHTTP_LOG_INFO(message) ((void)0)
#define AVHTTP_LOG_WARN(message) ((void)0)
#define AVHTTP_LOG_ERR(message) ((void)0)
#define AVHTTP_LOG_OUT(message) ((void)0)
#define AVHTTP_LOG_FILE(message) ((void)0)

#endif

#endif // LOGGING_HPP
