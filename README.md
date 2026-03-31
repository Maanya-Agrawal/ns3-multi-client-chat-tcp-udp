Multi Client Chatting using TCP & UDP (ns-3)
Description
This project demonstrates multi-client communication using TCP and UDP protocols in the ns-3 network simulator. It simulates a server interacting with multiple clients, where the server processes incoming messages and sends acknowledgements.

Objectives
To implement multi-client communication using TCP and UDP
To understand connection-oriented and connectionless protocols
To perform server-side calculations (timestamp, packet size)
To observe basic network performance
Technologies Used
ns-3 Network Simulator
C++

How to Run
./ns3 run scratch/multi-clien-chat-udp
./ns3 run scratch/multi-clien-chat-tcp

Conclusion
UDP provides faster communication with low overhead, while TCP ensures reliable and ordered data transfer. The experiment demonstrates effective multi-client communication and server response handling.

Author
Maanya Agrawal
