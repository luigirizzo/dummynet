#include <ntddk.h>

const char* texify_cmd(int i)
{
	if (i==110)
		return("IP_FW_ADD");
	if (i==111)
		return("IP_FW_DEL");
	if (i==112)
		return("IP_FW_FLUSH");
	if (i==113)
		return("IP_FW_ZERO");
	if (i==114)
		return("IP_FW_GET");
	if (i==115)
		return("IP_FW_RESETLOG");
	if (i==116)
		return("IP_FW_NAT_CFG");
	if (i==117)
		return("IP_FW_NAT_DEL");
	if (i==118)
		return("IP_FW_NAT_GET_CONFIG");
	if (i==119)
		return("IP_FW_NAT_GET_LOG");
	if (i==120)
		return("IP_DUMMYNET_CONFIGURE");
	if (i==121)
		return("IP_DUMMYNET_DEL");
	if (i==122)
		return("IP_DUMMYNET_FLUSH");
	if (i==124)
		return("IP_DUMMYNET_GET");
	if (i==108)
		return("IP_FW3");
	if (i==109)
		return("IP_DUMMYNET3");
	return ("BOH");
}

const char* texify_proto(unsigned int p)
{
	if (p==1)
		return("ICMP");
	if (p==6)
		return("TCP");
	if (p==17)
		return("UDP");
	return("OTHER");
}

void hexdump(unsigned char* addr, int len, const char *msg)
{
	int i;
	const  int cicli = len/8;
	const int resto = len%8;
	unsigned char d[8];

	DbgPrint("%s at %p len %d\n", msg, addr, len);
	for (i=0; i<=cicli; i++) {
		bzero(d, 8);
		bcopy(addr+i*8, d, i < cicli ? 8 : resto);
		DbgPrint("%04X %02X %02X %02X %02X %02X %02X %02X %02X\n",
			i*8, d[0], d[1], d[2], d[3], d[4],
			d[5], d[6], d[7]);
	}
	DbgPrint("\n");
}
