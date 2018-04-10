#include "entryPoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <stdexcept>
#include <cassert>

#include <iostream> //REMOVE
entryPoint::connection::connection(const struct sockaddr& ai_addr,int ai_socktype, int ai_family, int ai_addrlen){
	ioCon = socket(ai_family, ai_socktype, 0);
	if(ioCon<0){
		return;	
	}
	if(connect(ioCon, &ai_addr, ai_addrlen) < 0){
		if(errno != EINPROGRESS){
			close(ioCon);
			ioCon = -1;
		}
	}
	fcntl(ioCon, F_SETFL, O_NONBLOCK); // MOVE UP!!
	address = *((struct sockaddr_storage*)(&ai_addr));
	//fcntl(ioCon, F_SETFL, O_NONBLOCK);
	pol.fd = ioCon;
	pol.events = POLLIN;
	pol.revents = 0;
}

entryPoint::connection::connection(int connection, const struct sockaddr_storage &addr):ioCon(connection), address(addr){
	if(ioCon < 0){
		throw std::invalid_argument("connection was less than zero");
	}
	//fcntl(ioCon, F_SETFL, O_NONBLOCK);
	pol.fd = ioCon;
	pol.events = POLLIN;
	pol.revents = 0;
}

entryPoint::connection::connection():ioCon(-1){
}

entryPoint::connection::connection(connection&& that): ioCon(that.ioCon), address(that.address), pol(that.pol){
	that.ioCon = -1;
}

entryPoint::connection::~connection(){
	if(!isEmpty()){
		close(ioCon);
	}
}

entryPoint::connection& entryPoint::connection::operator=(connection&& that){
	if(this != &that){

		if(isEmpty()){
			close(ioCon);

		}
		ioCon = that.ioCon;
		address = that.address;
		pol = that.pol;
		
		that.ioCon = -1;
	}
	return *this;
}

bool entryPoint::connection::isEmpty() const{
	return (ioCon<0);
}

const struct sockaddr_storage& entryPoint::connection::getAddress() const{
	return address;
}

void *get_in_addr(const struct sockaddr *sa) {
  return sa->sa_family == AF_INET
    ? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
    : (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}

std::string entryPoint::connection::getClientIP() const{
	char ipv4[INET_ADDRSTRLEN];
	char ipv6[INET6_ADDRSTRLEN];
	const char* dest = nullptr;
	if(address.ss_family == AF_INET){
		dest = inet_ntop(AF_INET, (const void*)(get_in_addr((struct sockaddr*)&address)), ipv4, INET_ADDRSTRLEN);
	}else if(address.ss_family == AF_INET6){
		dest = inet_ntop(AF_INET6, (const void*)(get_in_addr((struct sockaddr*)&address)), ipv6, INET6_ADDRSTRLEN);
	}
	if(dest == nullptr){
		return std::string();	
	}
	return std::string(dest);
}

/*
int sendP(int iocon, const void* data,  size_t data_size, int flags){
	puts("-----------SENT------------");
	for(int i=0; i<data_size; i++){
		//putchar(((uint8_t*)data)[i]);
		printf("%02x ", ((uint8_t*)data)[i] );
	}
	puts("\n-----------SENT------------");
	return send(iocon, data, data_size, flags);
}
*/
int entryPoint::connection::send(const void* data, size_t data_size) const{
	if(isEmpty()){
		return -1;
	}
	int sent = 0;
	while(0<data_size){
		int change = ::send(ioCon, data, data_size, 0);
		if(change == -1){
			return -1;
		}
		sent += change;
		data_size -= change;
		data = ((uint8_t*)data) + change;
	}
	return sent;
}

int entryPoint::connection::recv(void* dest, size_t dest_size) const{
	if(isEmpty()){
		return -1;
	}
	errno = 0;
	int ret = ::recv(ioCon, dest, dest_size, 0);
	if(ret==0){
		ret = -1;
	}else if(ret < 0){
		if(errno == EAGAIN || errno == EWOULDBLOCK){
			ret = 0;
		}else{
			ret = -1;
		}
	}
	return ret;
}

bool entryPoint::connection::pollRecv() const{
	if(isEmpty()){
		return false;
	}
	pollfd npol = pol;
	poll(&npol, 1, 0);
	if(npol.revents & POLLIN){
		return true;
	}
	return false;
}

entryPoint::entryPoint(uint16_t port, int time): timeout(time){
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(-1);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            close(listener);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(listener, 128) == -1) {
        perror("listen");
        exit(1);
    }
	pol.fd = listener;
	pol.events = POLLIN;
	pol.revents = 0;
}

bool entryPoint::pollAccept() const{
	pollfd npoll = pol;
	poll(&npoll, 1, timeout);
	if(npoll.revents & POLLIN){
		return true;
	}
	return false;
}

entryPoint::~entryPoint(){
	if(listener>=0){
		close(listener);	
	}
}

entryPoint::connection entryPoint::accept() const{
	struct sockaddr_storage their_addr;
	socklen_t sin_size = sizeof(struct sockaddr_storage);
	int fd = ::accept(listener, (struct sockaddr *)&their_addr, &sin_size);
	return connection(fd, their_addr);
}




