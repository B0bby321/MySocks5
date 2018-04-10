#ifndef ENTRYPOINT_H
#define ENTRYPOINT_H

#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netdb.h>

class entryPoint{
	public: 
		class connection{
			private:
				int ioCon;
				struct sockaddr_storage address;
				pollfd pol;
			public:
				connection(int connection, const struct sockaddr_storage &addr);
				connection(const connection& that) = delete;
				connection(connection&& that);
				connection(const struct sockaddr& ai_addr, int ai_socktype=SOCK_STREAM, int ai_family=AF_INET, int ai_addrlen = sizeof(struct sockaddr_in));
				connection();
				~connection();
				connection& operator=(connection&& that);
				const struct sockaddr_storage& getAddress() const;
				std::string getClientIP() const;
				bool isEmpty() const;
				int send(const void* data, size_t data_size) const;
				int recv(void* dest, size_t dest_size) const;
				bool pollRecv() const;
		};
		entryPoint(uint16_t port, int timeout = 1);
		entryPoint(const entryPoint& that) = delete;
		~entryPoint();
		bool pollAccept() const;
		connection accept() const;
	private:
		const int timeout;
		int listener;
		pollfd pol;

};



#endif
