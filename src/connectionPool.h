#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#include <queue>
#include <memory>
#include <mutex> 
#include <thread>
#include <stdexcept>

template<typename T_handler, typename T_handlerInformation>
class connectionPool{
	public:
		typedef std::shared_ptr< connectionPool<T_handler, T_handlerInformation> > connectionPoolPtr;
		typedef std::unique_ptr<T_handlerInformation> handlerInformationPtr;
		handlerInformationPtr getTask();
		void addTask(handlerInformationPtr informationForHandler);
		bool shouldTerminate();
		void terminateAll();
		static connectionPoolPtr newConnectionPool(unsigned int numberOfThreads, const T_handler& handler);
	private:
		std::weak_ptr< connectionPool<T_handler, T_handlerInformation> > thisPtr;
		std::mutex queueLock;
		T_handler m_handler;
		bool shouldTerminateFlag;
		std::queue<handlerInformationPtr> tasks;
		static void threadWorker(connectionPoolPtr pool);
		connectionPoolPtr makeThreads(unsigned int numberOfThreads);
		connectionPool(const T_handler& handler);
		
};



template<class T_handler, class T_handlerInformation>
connectionPool<T_handler,T_handlerInformation>::connectionPool(const T_handler& handler): m_handler(handler), shouldTerminateFlag(false){
}

template<class T_handler, class T_handlerInformation>
typename connectionPool<T_handler,T_handlerInformation>::connectionPoolPtr connectionPool<T_handler,T_handlerInformation>::makeThreads(unsigned int numberOfThreads){
	connectionPoolPtr pool(this);
	for(int i=0; i<numberOfThreads; i++){
		std::thread(threadWorker, pool).detach();
	}
	return pool;
}

template<class T_handler, class T_handlerInformation>
typename connectionPool<T_handler, T_handlerInformation>::connectionPoolPtr connectionPool<T_handler, T_handlerInformation>::newConnectionPool(unsigned int numberOfThreads, const T_handler& handler){
	if(numberOfThreads<=0){
		throw std::invalid_argument("numberOfThreads less than or equal to zero");
	}
	return (new connectionPool<T_handler, T_handlerInformation>(handler))->makeThreads(numberOfThreads);
}


template<class T_handler, class T_handlerInformation> 
void connectionPool<T_handler, T_handlerInformation>::threadWorker(typename connectionPool<T_handler, T_handlerInformation>::connectionPoolPtr pool){
	T_handler copy(pool->m_handler);
	copy(pool);
}

template<class T_handler, class T_handlerInformation> 
bool connectionPool<T_handler, T_handlerInformation>::shouldTerminate(){
	return shouldTerminateFlag;
}

template<class T_handler, class T_handlerInformation> 
void connectionPool<T_handler, T_handlerInformation>::terminateAll(){
	shouldTerminateFlag = true;
}

template<class T_handler, class T_handlerInformation>
typename connectionPool<T_handler, T_handlerInformation>::handlerInformationPtr connectionPool<T_handler, T_handlerInformation>::getTask(){
	std::lock_guard<std::mutex> lock(queueLock);
	typename connectionPool<T_handler, T_handlerInformation>::handlerInformationPtr info;
	if(tasks.empty()){
		info = nullptr;
	}else{
		info = std::move(this->tasks.front());
		this->tasks.pop();
	}
	return info;
}


template<class T_handler, class T_handlerInformation>
void connectionPool<T_handler, T_handlerInformation>::addTask(typename connectionPool<T_handler, T_handlerInformation>::handlerInformationPtr informationForHandler){
	std::lock_guard<std::mutex> lock(queueLock);
	this->tasks.push(std::move(informationForHandler));
}







#endif
