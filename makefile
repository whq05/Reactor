all:client tcpepoll

client:client.cpp
	g++ -g -o client client.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp
tcpepoll:tcpepoll.cpp
	g++ -g -o tcpepoll tcpepoll.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp

clean:
	rm -f client tcpepoll
