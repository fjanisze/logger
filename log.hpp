#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <stdexcept>
#include <ctime>
#include <sstream>
#include <mutex>
#include <chrono>
#include <cstring>
#include <thread>
#include <map>
#include <queue>
#include <atomic>
#include <cassert>

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
	class logger;

	template< typename log_policy >
	void logging_daemon( logger< log_policy >* logger )
	{
		//Dump the log data if any
		do{
			std::this_thread::sleep_for( std::chrono::milliseconds{ 500 } );
			if( logger->log_buffer.size() )
			{
				std::lock_guard< std::mutex > lock{ logger->write_mutex };
				for( auto& elem : logger->log_buffer )
				{
					logger->policy->write( elem );
				}
				logger->log_buffer.clear();
			}
		}while( logger->is_still_running.test_and_set() || logger->log_buffer.size() );
		logger->policy->write( " - Terminating the logger daemon! - " );
	}
	
	template< typename log_policy >
	class logger
	{
		static std::string source_name;
		std::chrono::high_resolution_clock::time_point reference_epoch;
		
		unsigned log_line_number;

		static std::stringstream log_stream;
		log_policy* policy;
		std::mutex write_mutex;

		std::vector< std::string > log_buffer;
		std::thread daemon;
		std::atomic_flag is_still_running{ ATOMIC_FLAG_INIT };

		//Core printing functionality
		void print_impl();

		template<typename First, typename...Rest>
		void print_impl(First parm1, Rest...parm);

		std::map< std::thread::id , std::string > thread_name;
	public:
		logger( const std::string& name ) throw( std::bad_alloc );

		template< severity_type severity , typename...Args >
		void print( Args...args );

		void set_thread_name( const std::string& name );
		void terminate_logger();

		~logger();

		template< typename policy >
		friend void logging_daemon( logger< policy >* logger );
	};

	template< typename log_policy >
	std::string logger< log_policy >::source_name{};

	template< typename log_policy >
	std::stringstream logger< log_policy >::log_stream{};

	/*
	 * Implementation for logger
	 */

	template< typename log_policy >
	void logger< log_policy >::terminate_logger()
	{
		//Terminate the daemon activity
		is_still_running.clear();
		daemon.join(); 
	}

	template< typename log_policy >
	void logger< log_policy >::set_thread_name( const std::string& name )
	{
		thread_name[ std::this_thread::get_id() ] = name;
	}

	template< typename log_policy >
		template< severity_type severity , typename...Args >
	void logger< log_policy >::print( Args...args )
	{
		std::lock_guard< std::mutex > lock( write_mutex );
		log_stream.str("");
		//Prepare the header
		auto cur_time = std::chrono::high_resolution_clock::now();
		std::time_t tt = std::chrono::high_resolution_clock::to_time_t( cur_time );
		char* tt_s = ctime( &tt );
		tt_s[ strlen( tt_s ) - 1 ] = 0;

		log_stream << log_line_number++ <<" < "<< tt_s <<" - ";
		log_stream << std::chrono::duration_cast< std::chrono::milliseconds >( cur_time - reference_epoch  ).count() <<"ms > ";

		switch( severity )
		{
			case severity_type::debug:
				log_stream<<" DBG/";
				break;
			case severity_type::warning:
				log_stream<<" WRN/";
				break;
			case severity_type::error:
				log_stream<<" ERR/";
				break;
		};
		log_stream << thread_name[ std::this_thread::get_id() ] <<", ";
		print_impl( args... );
	}

	template< typename log_policy >
	void logger< log_policy >::print_impl()
	{
		log_buffer.push_back( log_stream.str() );
		log_stream.str("");
	}

	template< typename log_policy >
		template< typename First, typename...Rest >
	void logger< log_policy >::print_impl(First parm1, Rest...parm)
	{
		log_stream << parm1;
		print_impl(parm...);
	}

	template< typename log_policy >
	logger< log_policy >::logger( const std::string& name )  throw( std::bad_alloc )
	{
		log_line_number = 0;
		policy = new log_policy;
		assert( policy != nullptr );

		policy->open_ostream( name );
		reference_epoch = std::chrono::high_resolution_clock::now();
		//Set the running flag ans spawn the daemon
		is_still_running.test_and_set();
		daemon = std::move( std::thread{ logging_daemon< log_policy > , this } );
	}

	template< typename log_policy >
	logger< log_policy >::~logger()
	{
		policy->write( "- Logger activity terminated -" );
		policy->close_ostream();
		delete policy;	
	}
}

#endif
