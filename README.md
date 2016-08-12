# quantum-mare
A demonstration of 'quantum insert' TCP timing attack, for the purpose of helping developers write software that can resist network-level attacks. In this example, quantum-mare is both a sniffer and a packet injector. Mare injects packets into the network, and upon sniffing TCP port 80 traffic, Mare will attempt to respond first with a 302 redirect to another page.

## Purpose Statement
I've said this before, but this research is being made available to the community in hopes that it can be used intelliagbly by security researchers, and those interested in learning about threats to the underlying structure of the Internet. If you decide to use any of this code, you will want to create an isolated environment to test in. Never run this code on someone else's network without explicit permission from the network owner.

## For Developers
A typical strategy to prevent traffic manipulation and provide data integrity would be to use cryptography and signature verification to protect user sessions. The focus needs to be on hardening the protocol, ensuring that it doesn't expose too much metadata, and that it is protected cryptographically against traffic manipulation and timing attacks.
