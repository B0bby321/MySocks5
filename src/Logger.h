#ifndef LOGGER_H
#define LOGGER_H
#include <memory>
#include <string>
#include <sstream>
#include <ctime>
#include <thread>

#include <stdio.h>
namespace mylog{
	
	enum class msgType{GENERAL=0, ERROR};
	extern const char* msgTypeWords[2];	

	template<class T_Endpoint>
	class Logger{
		public:
			Logger(std::unique_ptr<T_Endpoint> _e){
				e = std::move(_e);
			}
			 //[hh:mm:ss] [Thread: ] [Type] MESSAGE
			void log(std::string message, msgType type = msgType::GENERAL){
				std::stringstream ss;
				time_t now = time(0);
				char timestamp[40] = "";
				strftime(timestamp, 40,"%c", localtime(&now));
				ss<<"["<<timestamp<<"]"<<"[Thread: "<<std::this_thread::get_id()<<"]"<<"["<<msgTypeWords[(int)type]<<"]"<<message;
				(*e)(ss.str());
			}
		private:
			std::unique_ptr<T_Endpoint> e;
			
	};


	class fileEndPoint{
		
		public:
			fileEndPoint(std::string filename);
			~fileEndPoint();
			void operator()(std::string msg);
		private:
			FILE* f;
			
	};

	extern std::unique_ptr<Logger<fileEndPoint>> globalLog;
	void setupGlobalLog(std::string filename);
	
}






#endif
