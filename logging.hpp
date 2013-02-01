//
// Copyright (C) 2019 Jack.
//
// Author: jack
// Email:  jack.wgm at gmail dot com
//

#pragma once

#include <clocale>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <tuple>

#include <boost/core/ignore_unused.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/nowide/convert.hpp>

//////////////////////////////////////////////////////////////////////////
#ifdef LOGGING_COMPRESS_LOGS
#	include <zlib.h>
#endif // LOGGING_COMPRESS_LOGS

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4244 4127)
#endif // _MSC_VER

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexpansion-to-defined"
#endif

#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <fmt/format.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

//#include <utf8.hpp>
//////////////////////////////////////////////////////////////////////////

#if defined(_WIN32) || defined(WIN32)
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif // !WIN32_LEAN_AND_MEAN
#	include <mmsystem.h>
#	include <windows.h>
#	pragma comment(lib, "Winmm.lib")
#endif // _WIN32

#ifdef USE_SYSTEMD_LOGGING
	#ifdef __linux__
	#	include <systemd/sd-journal.h>
	#endif // __linux__
#endif

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace util {

///内部使用的简易日志类.
// @begin example
//  #include "logging.hpp"
//  int main()
//  {
//     init_logging();
//     LOG_DEBUG << "Initialized.";
//     std::string result = do_something();
//     LOG_DEBUG << "do_something return : " << result;	// 输出do_something返回结果到日志.
//     ...
//  }
// @end example

#ifndef LOG_APPNAME
#	define LOG_APPNAME "application"
#endif

#ifndef LOG_MAXFILE_SIZE
#	define LOG_MAXFILE_SIZE (-1)
#endif // LOG_MAXFILE_SIZE


#ifdef LOGGING_COMPRESS_LOGS

namespace compress {

	const std::string GZ_SUFFIX = ".gz";
	// const size_t SUFFIX_LEN = sizeof(GZ_SUFFIX) - 1;
	const size_t BUFLEN = 65536;
	// const size_t MAX_NAME_LEN = 4096;

	inline boost::mutex& compress_lock()
	{
		static boost::mutex lock;
		return lock;
	}

	inline bool do_compress_gz(const std::string& infile)
	{
		std::string outfile = infile + GZ_SUFFIX;

		gzFile out = gzopen(outfile.c_str(), "wb6f");
		if (!out)
			return false;
		typedef typename std::remove_pointer<gzFile>::type gzFileType;
		std::unique_ptr<gzFileType, decltype(&gzclose)> gz_closer(out, &gzclose);

		FILE* in = fopen(infile.c_str(), "rb");
		if (!in)
			return false;
		std::unique_ptr<FILE, decltype(&fclose)> FILE_closer(in, &fclose);

		char buf[BUFLEN];
		int len;

		for (;;) {
			len = (int)fread(buf, 1, sizeof(buf), in);
			if (ferror(in))
				return false;

			if (len == 0)
				break;

			int total = 0;
			int ret;
			while (total < len) {
				ret = gzwrite(out, buf + total, (unsigned)len - total);
				if (ret <= 0) {
					return false;	// detail error information see gzerror(out, &ret);
				}
				total += ret;
			}
		}

		return true;
	}

}

#endif

namespace aux {

	static const boost::uint64_t epoch = 116444736000000000L; /* Jan 1, 1601 */
	typedef union {
		boost::uint64_t ft_scalar;
#if defined(WIN32) || defined(_WIN32)

		FILETIME ft_struct;
#else
		timeval ft_struct;
#endif
	} LOGGING_FT;

	inline int64_t gettime()
	{
#if defined(WIN32) || defined(_WIN32)
		thread_local int64_t system_start_time = 0;
		thread_local int64_t system_current_time = 0;
		thread_local uint32_t last_time = 0;

		auto tmp = timeGetTime();

		if (system_start_time == 0) {
			LOGGING_FT nt_time;
			GetSystemTimeAsFileTime(&(nt_time.ft_struct));
			int64_t tim = (__int64)((nt_time.ft_scalar - aux::epoch) / (__int64)10000);
			system_start_time = tim - tmp;
		}

		system_current_time += (tmp - last_time);
		last_time = tmp;
		return system_start_time + system_current_time;
#elif defined(__linux__) || defined(__APPLE__)
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return ((int64_t)tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
#endif
	}

	inline uint32_t decode(uint32_t* state, uint32_t* codep, uint32_t byte)
	{
		static constexpr uint8_t utf8d[] = {
			  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
			  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
			  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
			  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
			  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
			  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
			  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
			  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
			  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
			  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
			  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
			  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
			  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
			  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
		};

		uint32_t type = utf8d[byte];

		*codep = (*state != 0) ?
			(byte & 0x3fu) | (*codep << 6) :
			(0xff >> type) & (byte);

		*state = utf8d[256 + *state * 16 + type];
		return *state;
	}

	inline bool utf8_check_is_valid(const std::string& str)
	{
		uint32_t codepoint;
		uint32_t state = 0;
		size_t count = 0;
		uint8_t* s = (uint8_t*)str.c_str();

		for (count = 0; *s; ++s)
			if (!decode(&state, &codepoint, *s))
				count += 1;

		return state == 0;
	}

#if 0
	inline bool utf8_check_is_valid(const std::string& str)
	{
		int c, i, ix, n, j;
		for (i = 0, ix = static_cast<int>(str.size()); i < ix; i++)
		{
			c = (unsigned char)str[i];
			//if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
			if (0x00 <= c && c <= 0x7f) n = 0; // 0bbbbbbb
			else if ((c & 0xE0) == 0xC0) n = 1; // 110bbbbb
			else if (c == 0xed && i < (ix - 1) && ((unsigned char)str[i + 1] & 0xa0) == 0xa0)
				return false; // U+d800 to U+dfff
			else if ((c & 0xF0) == 0xE0) n = 2; // 1110bbbb
			else if ((c & 0xF8) == 0xF0) n = 3; // 11110bbb
												//else if (($c & 0xFC) == 0xF8) n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
												//else if (($c & 0xFE) == 0xFC) n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
			else return false;
			for (j = 0; j < n && i < ix; j++)	// n bytes matching 10bbbbbb follow ?
			{
				if ((++i == ix) || (((unsigned char)str[i] & 0xC0) != 0x80))
					return false;
			}
		}
		return true;
	}
#endif

	inline bool wide_string(const std::wstring& src, std::string& str)
	{
		std::locale sys_locale("");

		const wchar_t* data_from = src.c_str();
		const wchar_t* data_from_end = src.c_str() + src.size();
		const wchar_t* data_from_next = 0;

		int wchar_size = 4;
		std::unique_ptr<char> buffer(new char[(src.size() + 1) * wchar_size]);
		char* data_to = buffer.get();
		char* data_to_end = data_to + (src.size() + 1) * wchar_size;
		char* data_to_next = 0;

		memset(data_to, 0, (src.size() + 1) * wchar_size);

		typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;
		mbstate_t out_state;
		auto result = std::use_facet<convert_facet>(sys_locale).out(
			out_state, data_from, data_from_end, data_from_next,
			data_to, data_to_end, data_to_next);
		if (result == convert_facet::ok)
		{
			str = data_to;
			return true;
		}

		return false;
	}

	inline bool string_wide(const std::string& src, std::wstring& wstr)
	{
		std::locale sys_locale("");
		const char* data_from = src.c_str();
		const char* data_from_end = src.c_str() + src.size();
		const char* data_from_next = 0;

		std::unique_ptr<wchar_t> buffer(new wchar_t[src.size() + 1]);
		wchar_t* data_to = buffer.get();
		wchar_t* data_to_end = data_to + src.size() + 1;
		wchar_t* data_to_next = 0;

		wmemset(data_to, 0, src.size() + 1);

		typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;
		mbstate_t in_state;
		auto result = std::use_facet<convert_facet>(sys_locale).in(
			in_state, data_from, data_from_end, data_from_next,
			data_to, data_to_end, data_to_next);
		if (result == convert_facet::ok)
		{
			wstr = data_to;
			return true;
		}

		return false;
	}

	inline std::string string_utf8(const std::string& str)
	{
		if (!aux::utf8_check_is_valid(str))
		{
			std::wstring wres;
			if (aux::string_wide(str, wres))
				return boost::nowide::narrow(wres);
		}

		return str;
	}

	template <class Lock>
	Lock& lock_single()
	{
		static Lock lock_instance;
		return lock_instance;
	}

	template <class Writer>
	Writer& writer_single()
	{
		static Writer writer_instance;
		return writer_instance;
	}

	inline std::tuple<std::string, struct tm*> time_to_string(int64_t time)
	{
		std::string ret;
		std::time_t rawtime = time / 1000;
		struct tm* ptm = std::localtime(&rawtime);
		if (!ptm)
			return { ret, ptm };
		char buffer[1024];
		std::sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
			ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)(time % 1000));
		ret = buffer;
		return { ret, ptm };
	}
}


class auto_logger_file {
public:
	auto_logger_file()
	{
		m_log_path = m_log_path / (LOG_APPNAME + std::string(".log"));
		boost::system::error_code ignore_ec;
		if (!boost::filesystem::exists(m_log_path, ignore_ec))
			boost::filesystem::create_directories(boost::filesystem::path(m_log_path).branch_path(), ignore_ec);
	}
	~auto_logger_file()
	{
	}

	typedef boost::shared_ptr<std::ofstream> ofstream_ptr;

	void open(const char* path)
	{
		m_log_path = path;
		boost::system::error_code ignore_ec;
		if (!boost::filesystem::exists(m_log_path, ignore_ec))
			boost::filesystem::create_directories(boost::filesystem::path(m_log_path).branch_path(), ignore_ec);
	}

	std::string log_path() const
	{
		return m_log_path.string();
	}

	void write(int64_t time, const char* str, std::streamsize size)
	{
		boost::ignore_unused(time);
#ifdef LOGGING_COMPRESS_LOGS
		bool condition = false;
		auto hours = time / 1000 / 3600;
		auto last_hours = m_last_time / 1000 / 3600;

		if (LOG_MAXFILE_SIZE > 0)
			condition = true;
		if (last_hours != hours && LOG_MAXFILE_SIZE < 0)
			condition = true;

		while (condition) {
			if (m_last_time == -1) {
				m_last_time = time;
				break;
			}

			auto [ts, ptm] = aux::time_to_string(m_last_time);
			boost::ignore_unused(ts);

			m_ofstream->close();
			m_ofstream.reset();

			auto logpath = boost::filesystem::path(m_log_path).branch_path();
			boost::filesystem::path filename;

			if (LOG_MAXFILE_SIZE <= 0) {
				auto logfile = fmt::format("{:04d}{:02d}{:02d}-{:02d}.log",
					ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour);
				filename = logpath / logfile;
			} else {
				auto utc_time = std::mktime(ptm);
				auto logfile = fmt::format("{:04d}{:02d}{:02d}-{}.log",
					ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, utc_time);
				filename = logpath / logfile;
			}

			m_last_time = time;

			boost::system::error_code ec;
			if (!boost::filesystem::copy_file(m_log_path, filename, ec))
				break;

			boost::filesystem::resize_file(m_log_path, 0, ec);
			m_log_size = 0;
			auto fn = filename.string();
			boost::async(boost::launch::async, [this, fn]() {
				std::string file = fn;
				boost::system::error_code ignore_ec;
				boost::mutex& m = compress::compress_lock();
				boost::lock_guard<boost::mutex> lock(m);
				if (!compress::do_compress_gz(fn)) {
					file = fn + compress::GZ_SUFFIX;
					boost::filesystem::remove(file, ignore_ec);
					if (ignore_ec)
						std::cout << "delete log failed: " << file
						<< ", error code: " << ignore_ec.message() << std::endl;
				}
				else {
					boost::filesystem::remove(fn, ignore_ec);
				}
			});

			break;
		}
#endif

		if (!m_ofstream) {
			m_ofstream.reset(new std::ofstream);
			m_ofstream->open(m_log_path.string().c_str(), std::ios_base::out | std::ios_base::app);
			m_ofstream->sync_with_stdio(false);
		}

		if (m_ofstream->is_open()) {
			m_log_size += size;
			m_ofstream->write(str, size);
			m_ofstream->flush();
		}
	}

private:
	boost::filesystem::path m_log_path{"./logs"};
	ofstream_ptr m_ofstream;
	int64_t m_last_time{-1};
	std::size_t m_log_size{0};
};

#ifndef DISABLE_LOGGER_THREAD_SAFE
#define LOGGER_LOCKS_() boost::lock_guard<boost::mutex> lock(aux::lock_single<boost::mutex>())
#else
#define LOGGER_LOCKS_() ((void)0)
#endif // LOGGER_THREAD_SAFE

#ifndef LOGGER_DBG_VIEW_
#if defined(WIN32) && (defined(LOGGER_DBG_VIEW) || defined(DEBUG) || defined(_DEBUG))
#define LOGGER_DBG_VIEW_(x)              \
	do {                                 \
		::OutputDebugStringA(x.c_str()); \
	} while (0)
#else
#define LOGGER_DBG_VIEW_(x) ((void)0)
#endif // WIN32 && LOGGER_DBG_VIEW
#endif // LOGGER_DBG_VIEW_

static std::string LOGGER_DEBUG_STR = "DEBUG";
static std::string LOGGER_INFO_STR = "INFO";
static std::string LOGGER_WARN_STR = "WARNING";
static std::string LOGGER_ERR_STR = "ERROR";
static std::string LOGGER_FILE_STR = "FILE";

inline void output_console(std::string& level, const std::string& prefix, const std::string& message)
{
#ifdef WIN32
	HANDLE handle_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(handle_stdout, &csbi);
	if (level == LOGGER_INFO_STR)
		SetConsoleTextAttribute(handle_stdout, FOREGROUND_GREEN);
	else if (level == LOGGER_DEBUG_STR)
		SetConsoleTextAttribute(handle_stdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	else if (level == LOGGER_WARN_STR)
		SetConsoleTextAttribute(handle_stdout, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	else if (level == LOGGER_ERR_STR)
		SetConsoleTextAttribute(handle_stdout, FOREGROUND_RED | FOREGROUND_INTENSITY);
	std::wstring wstr = boost::nowide::widen(prefix);
	WriteConsoleW(handle_stdout, wstr.data(), (DWORD)wstr.size(), nullptr, nullptr);
	SetConsoleTextAttribute(handle_stdout, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);
	wstr = boost::nowide::widen(message);
	WriteConsoleW(handle_stdout, wstr.data(), (DWORD)wstr.size(), nullptr, nullptr);
	SetConsoleTextAttribute(handle_stdout, csbi.wAttributes);
#else
	fmt::memory_buffer out;
	if (level == LOGGER_INFO_STR)
		fmt::format_to(out, "\033[32m{}\033[0m{}", prefix, message);
	else if (level == LOGGER_DEBUG_STR)
		fmt::format_to(out, "\033[1;32m{}\033[0m{}", prefix, message);
	else if (level == LOGGER_WARN_STR)
		fmt::format_to(out, "\033[1;33m{}\033[0m{}", prefix, message);
	else if (level == LOGGER_ERR_STR)
		fmt::format_to(out, "\033[1;31m{}\033[0m{}", prefix, message);
	std::cout << fmt::to_string(out);
	std::cout.flush();
#endif
}

#ifdef USE_SYSTEMD_LOGGING
inline void output_systemd(const std::string& level, const std::string& message)
{
	if (level == LOGGER_INFO_STR)
		sd_journal_print(LOG_INFO, "%s", message.c_str());
	else if (level == LOGGER_DEBUG_STR)
		sd_journal_print(LOG_DEBUG, "%s", message.c_str());
	else if (level == LOGGER_WARN_STR)
		sd_journal_print(LOG_WARNING, "%s", message.c_str());
	else if (level == LOGGER_ERR_STR)
		sd_journal_print(LOG_ERR, "%s", message.c_str());
}
#endif // USE_SYSTEMD_LOGGING

inline void logger_writer(int64_t time, std::string level,
	std::string message, bool disable_cout = false)
{
	(void)disable_cout;
	LOGGER_LOCKS_();
	auto [ts, ptm] = aux::time_to_string(time);
	boost::ignore_unused(ptm);
	std::string prefix = ts + std::string(" [") + level + std::string("]: ");
	std::string tmp = message + "\n";
	std::string whole = prefix + tmp;
#ifdef __ANDROID__
	if (level == LOGGER_INFO_STR)
		__android_log_print(ANDROID_LOG_INFO, "LOGGING", "%s", whole.c_str());
	else if (level == LOGGER_DEBUG_STR)
		__android_log_print(ANDROID_LOG_DEBUG, "LOGGING", "%s", whole.c_str());
	else if (level == LOGGER_WARN_STR)
		__android_log_print(ANDROID_LOG_WARN, "LOGGING", "%s", whole.c_str());
	else if (level == LOGGER_ERR_STR)
		__android_log_print(ANDROID_LOG_ERROR, "LOGGING", "%s", whole.c_str());
	else
		__android_log_print(ANDROID_LOG_DEFAULT, "LOGGING", "%s", whole.c_str());
	return;
#endif
#ifndef DISABLE_WRITE_LOGGING
	util::aux::writer_single<util::auto_logger_file>().write(time, whole.c_str(), whole.size());
#endif // !DISABLE_WRITE_LOGGING
	LOGGER_DBG_VIEW_(whole);
#ifndef DISABLE_LOGGER_TO_CONSOLE
	if (!disable_cout)
		output_console(level, prefix, tmp);
#endif
#ifdef USE_SYSTEMD_LOGGING
	output_systemd(level, message);
#endif // USE_SYSTEMD_LOGGING
}

namespace aux {

	class logger_internal {
	public:
		logger_internal()
		{
		}
		~logger_internal()
		{
			if (m_main_thread.joinable()) {
				if (!m_io_context.stopped())
					m_io_context.stop();
				m_main_thread.join();
			}
		}

	public:
		void start()
		{
			m_main_thread = boost::thread(boost::bind(&logger_internal::main_thread, this));
		}

		void stop()
		{
			if (!m_io_context.stopped())
				m_io_context.stop();
		}

		void post_log(std::string level,
			std::string message, bool disable_cout = false)
		{
			m_io_context.post(boost::bind(&logger_writer, aux::gettime(),
				level, message, disable_cout));
		}

	private:
		void main_thread()
		{
			boost::asio::io_context::work work(m_io_context);
			try {
				m_io_context.run();
			} catch (std::exception&) {}
		}

	private:
		boost::asio::io_context m_io_context;
		boost::thread m_main_thread;
	};
}

inline boost::shared_ptr<aux::logger_internal>& fetch_log_obj()
{
	static boost::shared_ptr<aux::logger_internal> logger_obj_;
	return logger_obj_;
}

inline void init_logging(bool use_async = true, const std::string& path = "")
{
	auto_logger_file& file = aux::writer_single<util::auto_logger_file>();
	if (!path.empty())
		file.open(path.c_str());

	auto& log_obj = fetch_log_obj();
	if (use_async && !log_obj) {
		log_obj.reset(new aux::logger_internal());
		log_obj->start();
	}
}

inline std::string log_path()
{
	auto_logger_file& file = aux::writer_single<util::auto_logger_file>();
	return file.log_path();
}

inline void shutdown_logging()
{
	auto& log_obj = fetch_log_obj();
	if (log_obj) {
		log_obj->stop();
		log_obj.reset();
	}
}

inline bool& logging_flag()
{
	static bool logging_ = true;
	return logging_;
}

inline void toggle_logging()
{
	logging_flag() = !logging_flag();
}

class logger : boost::noncopyable {
public:
	logger(std::string& level, bool disable_cout = false)
		: level_(level)
		, m_disable_cout(disable_cout)
	{
		if (!logging_flag())
			return;
	}
	~logger()
	{
		if (!logging_flag())
			return;
		std::string message = aux::string_utf8(fmt::to_string(out_));
		if (fetch_log_obj())
			fetch_log_obj()->post_log(level_, message, m_disable_cout);
		else
			logger_writer(aux::gettime(), level_, message, m_disable_cout);
	}

	template <class T>
	inline logger& strcat_impl(T const& v)
	{
		if (!logging_flag())
			return *this;
		fmt::format_to(out_, "{}", v);
		return *this;
	}

	inline logger& operator<<(bool v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(short v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(unsigned short v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(int v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(unsigned int v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(unsigned long long v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(long v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(long long v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(float v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(double v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(long double v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(unsigned long int v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(const std::string& v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(const char* v)
	{
		return strcat_impl(v);
	}
	inline logger& operator<<(const void *v)
	{
		if (!logging_flag())
			return *this;
		fmt::format_to(out_, "{:#010x}", (std::size_t)v);
		return *this;
	}
	inline logger& operator<<(const boost::posix_time::ptime& p)
	{
		if (!logging_flag())
			return *this;

		if (!p.is_not_a_date_time())
		{
			auto date = p.date().year_month_day();
			auto time = p.time_of_day();

			fmt::format_to(out_, "{:04}", date.year);
			fmt::format_to(out_, "-{:02}", date.month.as_number());
			fmt::format_to(out_, "-{:02}", date.day.as_number());

			fmt::format_to(out_, " {:02}", time.hours());
			fmt::format_to(out_, ":{:02}", time.minutes());
			fmt::format_to(out_, ":{:02}", time.seconds());

			auto ms = time.total_milliseconds() % 1000;		// milliseconds.
			if (ms != 0)
				fmt::format_to(out_, ".{:03}", ms);
		}
		else
		{
			fmt::format_to(out_, "NOT A DATE TIME");
		}

		return *this;
	}

	fmt::memory_buffer out_;
	std::string& level_;
	bool m_disable_cout;
};

class empty_logger : boost::noncopyable {
public:
	template <class T>
	empty_logger& operator<<(T const&/*v*/)
	{
		return *this;
	}
};
} // namespace util

using util::init_logging;
using util::shutdown_logging;

#if (defined(DEBUG) || defined(_DEBUG) || defined(ENABLE_LOGGER)) && !defined(DISABLE_LOGGER)

#undef LOG_DBG
#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERR
#undef LOG_FILE

#define LOG_DBG util::logger(util::LOGGER_DEBUG_STR)
#define LOG_INFO util::logger(util::LOGGER_INFO_STR)
#define LOG_WARN util::logger(util::LOGGER_WARN_STR)
#define LOG_ERR util::logger(util::LOGGER_ERR_STR)
#define LOG_FILE util::logger(util::LOGGER_FILE_STR, true)

#define VLOG_DBG LOG_DBG << "(" << __FILE__ << ":" << __LINE__ << "): "
#define VLOG_INFO LOG_INFO << "(" << __FILE__ << ":" << __LINE__ << "): "
#define VLOG_WARN LOG_WARN << "(" << __FILE__ << ":" << __LINE__ << "): "
#define VLOG_ERR LOG_ERR << "(" << __FILE__ << ":" << __LINE__ << "): "
#define VLOG_FILE LOG_FILE << "(" << __FILE__ << ":" << __LINE__ << "): "

#else

#define LOG_DBG util::empty_logger()
#define LOG_INFO util::empty_logger()
#define LOG_WARN util::empty_logger()
#define LOG_ERR util::empty_logger()
#define LOG_FILE util::empty_logger()

#define VLOG_DBG LOG_DBG
#define VLOG_INFO LOG_INFO
#define VLOG_WARN LOG_WARN
#define VLOG_ERR LOG_ERR
#define VLOG_FILE LOG_FILE

#endif
