#ifndef SOCKSCORE_H
#define SOCKSCORE_H

#include "connectionPool.h"
#include "entryPoint.h"

class socksCore{
	public:
		class socksCoreInfo{
			
			public:
				enum class stage{NEW_CONNECTION, LISTEN_REQUEST, TUNNELING};
				socksCoreInfo(entryPoint::connection &&c);
				stage getStage();
				void setStage(stage s);
				const entryPoint::connection& getConnection();
				const entryPoint::connection& getDestinationConnection();
				void setDestinationConnection(entryPoint::connection&& c);
			private:
				entryPoint::connection c;	
				entryPoint::connection dest;
				stage s;
		};
		typedef socksCoreInfo handlerInformation;
		void operator()(std::shared_ptr< connectionPool<socksCore, socksCore::handlerInformation> > pool);
		socksCore();
		socksCore(const socksCore& that);
	private:
		std::unique_ptr<socksCore::handlerInformation> info;
		void handleNewConnection();
		void handleListenRequest();
		void CommandConnect(const char* buffer, size_t size);
		void handleTunneling();
	
};





#endif
