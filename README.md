The reliable delivery system uses a fixed size packet of 256B, and the first four bytes represent a 32 bit Int LE. There is no CRC because of the Datagram delivery a CRC already exists on each packet. The current implementation sends a long string.