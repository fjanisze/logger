#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <stdexcept>
#include <ctime>
#include <sstream>
#include <mutex>

#ifndef LOG_HPP
#define LOG_HPP

namespace logging
{

	/*
	 * log_policy for the logger
	 */

	class log_policy_interface
	{
	public:
		virtual void		open_ostream(const std::string& name) = 0;
		virtual void		close_ostream() = 0;
		virtual void		write(const std::string& msg) = 0;

	};

	/*
	 * Implementation which allow to write into a file
	 */

	class file_log_policy : public log_policy_interface
	{
		std::unique_ptr< std::ofstream > out_stream;
	public:
		file_log_policy() : out_stream( new std::ofstream ) {   }
		void open_ostream(const std::string& name);
		void close_ostream();
		void write(const std::string& msg);
		~file_log_policy();
	};

	enum severity_type
	{
		debug = 1,
		error,
		warning
	};

	/*
	 * the Logger class, shall be instantiated with a specific log_policy
	 */

	template< typename log_policy >
	class logger
	{
		unsigned log_line_number;
		std::string get_time();
		std::string get_logline_header();
		std::stringstream log_stream;
		log_policy* policy;
		std::mutex write_mutex;

		//Core printing functionality
		void print_impl();
		template<typename First, typename...Rest>
		void print_impl(First parm1, Rest...parm);
	public:
		logger( const std::string& name ) throw( std::bad_alloc );

		template< severity_type severity , typename...Args >
		void print( Args...args );

		~logger();
	};

	/*
	 * Implementation for logger
	 */

	template< typename log_policy >
		template< severity_type severity , typename...Args >
	void logger< log_policy >::print( Args...args )
	{
		std::lock_guard< std::mutex > lock( write_mutex );
		switch( severity )
		{
			case severity_type::debug:
				log_stream<<"<DEBUG>~:";
				break;
			case severity_type::warning:
				log_stream<<"<WARNING>~:";
				break;
			case severity_type::error:
				log_stream<<"<ERROR>~:";
				break;
		};
		print_impl( args... );
	}

	template< typename log_policy >
	logger< log_policy >::logger( const std::string& name )  throw( std::bad_alloc )
	{
		log_line_number = 0;
		policy = new log_policy;
		policy->open_ostream( name );
	}

	template< typename log_policy >
	std::string logger< log_policy >::get_time()
	{
		std::string time_str;
		time_t raw_time;

		time( & raw_time );
		time_str = ctime( &raw_time );

		//without the newline character
		return time_str.substr( 0 , time_str.size() - 1 );
	}

	template< typename log_policy >
	std::string logger< log_policy >::get_logline_header()
	{
		std::stringstream header;

		header.str("");
		header.fill('0');
		header << log_line_number++ <<" < "<<get_time()<<" - ";

		header.fill('0');
		header <<clock()<<" >~";

		return header.str();
	}

	template< typename log_policy >
	void logger< log_policy >::print_impl()
	{
		policy->write( get_logline_header() + log_stream.str() );
		log_stream.str("");
	}

	template< typename log_policy >
		template<typename First, typename...Rest >
	void logger< log_policy >::print_impl(First parm1, Rest...parm)
	{
		log_stream<<parm1;
		print_impl(parm...);
	}

	template< typename log_policy >
	logger< log_policy >::~logger()
	{
		if( policy )
		{
			policy->close_ostream();
			delete policy;
		}
	}
}

#endif
