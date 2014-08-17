/*
 * Copyright (C) 2010 Luigi Rizzo, Francesco Magno, Universita` di Pisa
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * kernel variables and functions that are not available in Windows.
 */

#include <net/pfil.h> /* provides PFIL_IN and PFIL_OUT */
#include <arpa/inet.h>
#include <netinet/in.h>			/* in_addr */
#include <ndis.h>
#include <sys/mbuf.h>
#include <passthru.h>

/* credentials check */
int
cred_check(void *_insn,  int proto, struct ifnet *oif,
    struct in_addr dst_ip, u_int16_t dst_port, struct in_addr src_ip,
    u_int16_t src_port, struct bsd_ucred *u, int *ugid_lookupp,
    struct sk_buff *skb)
{
	return 0;
}

/*
 * as good as anywhere, place here the missing calls
 */

void *
my_alloc(int size)
{
	void *_ret = ExAllocatePoolWithTag(NonPagedPool, size, 'wfpi');
	if (_ret)
		memset(_ret, 0, size);
	return _ret;
}

void
panic(const char *fmt, ...)
{
	printf("%s", fmt);
	for (;;);
}

int securelevel = 0;

int ffs(int bits)
{
	int i;
	if (bits == 0)
		return (0);
	for (i = 1; ; i++, bits >>= 1) {
		if (bits & 1)
			break;
	}
	return (i);
}

void
do_gettimeofday(struct timeval *tv)
{
	static LARGE_INTEGER prevtime; //system time in 100-nsec resolution
	static LARGE_INTEGER prevcount; //RTC counter value
	static LARGE_INTEGER freq; //frequency

	LARGE_INTEGER currtime;
	LARGE_INTEGER currcount;
	if (prevtime.QuadPart == 0) { //first time we ask for system time
		KeQuerySystemTime(&prevtime);
		prevcount = KeQueryPerformanceCounter(&freq);
		currtime.QuadPart = prevtime.QuadPart;
	} else {
		KeQuerySystemTime(&currtime);
		currcount = KeQueryPerformanceCounter(&freq);
		if (currtime.QuadPart == prevtime.QuadPart) {
			//time has NOT changed, calculate time using ticks and DO NOT update
			LONGLONG difftime = 0; //difference in 100-nsec
			LONGLONG diffcount = 0; //clock count difference
			//printf("time has NOT changed\n");
			diffcount = currcount.QuadPart - prevcount.QuadPart;
			diffcount *= 10000000;
			difftime = diffcount / freq.QuadPart;
			currtime.QuadPart += difftime;
		} else {	
			//time has changed, update and return SystemTime
			//printf("time has changed\n");
			prevtime.QuadPart = currtime.QuadPart;
			prevcount.QuadPart = currcount.QuadPart;
		}
	}
	currtime.QuadPart /= 10; //convert in usec
	tv->tv_sec = currtime.QuadPart / (LONGLONG)1000000;
	tv->tv_usec = currtime.QuadPart % (LONGLONG)1000000;
	//printf("sec %d usec %d\n",tv->tv_sec, tv->tv_usec);
}

int time_uptime_w32()
{
	int ret;
	LARGE_INTEGER tm;
	KeQuerySystemTime(&tm);
	ret = (int)(tm.QuadPart / (LONGLONG)1000000);
	return ret;
}


/*
 * Windows version of firewall hook. We receive a partial copy of
 * the packet which points to the original buffers. In output,
 * the refcount has been already incremented.
 * The function reconstructs
 * the whole packet in a contiguous memory area, builds a fake mbuf,
 * calls the firewall, does the eventual cleaning and returns
 * to MiniportSend or ProtocolReceive, which will silently return
 * (dropping packet) or continue its execution (allowing packet).
 * The memory area contains:
 * - the fake mbuf, filled with data needed by ipfw, and information
 *   for reinjection
 * - the packet data
 */
void hexdump(PUCHAR,int, const char *);
static char _if_in[] = "incoming";
static char _if_out[] = "outgoing";

int
ipfw2_qhandler_w32(PNDIS_PACKET pNdisPacket, int direction,
	NDIS_HANDLE Context)
{	
	unsigned int		BufferCount = 0;
	unsigned			TotalPacketLength = 0;
	PNDIS_BUFFER		pCurrentBuffer = NULL;
	PNDIS_BUFFER		pNextBuffer = NULL;
	struct mbuf*		m;
	unsigned char*		payload = NULL;
	unsigned int		ofs, l;
	unsigned short		EtherType = 0;
	unsigned int		i = 0;
	int					ret = 0;
	PNDIS_BUFFER		pNdisBuffer, old_head, old_tail;
	NDIS_HANDLE			PacketPool;
	PADAPT				pAdapt;
	NDIS_STATUS			Status;

	/* In NDIS, packets are a chain of NDIS_BUFFER. We query
	 * the packet to get a pointer of chain's head, the length
	 * of the chain, and the length of the packet itself.
	 * Then allocate a buffer for the mbuf and the payload.
	 */
	NdisQueryPacket(pNdisPacket, NULL, &BufferCount,
		&pCurrentBuffer, &TotalPacketLength);
	m = malloc(sizeof(struct mbuf) + TotalPacketLength, 0, 0 );
	if (m == NULL) //resource shortage, drop the packet
		goto drop_pkt;

	/* set mbuf fields to point past the MAC header.
	 * Also set additional W32 info
	 */
	payload = (unsigned char*)(m + 1);
	m->m_len = m->m_pkthdr.len = TotalPacketLength-14;
	m->m_pkthdr.rcvif = (void *)((direction==INCOMING) ? _if_in : NULL);
	m->m_data = payload + 14; /* past the MAC header */
	m->direction = direction;
	m->context = Context;
	m->pkt = pNdisPacket;

	/* m_skb != NULL is used in the ip_output routine to check
	 * for packets that come from the stack and differentiate
	 * from those internally generated by ipfw.
	 * The pointer is not used, just needs to be non-null.
	 */
	m->m_skb = (void *)pNdisPacket;
	/*
	 * Now copy the data from the Windows buffers to the mbuf.
	 */
	for (i=0, ofs = 0; i < BufferCount; i++) {
		unsigned char* src;
		NdisQueryBufferSafe(pCurrentBuffer, &src, &l,
			NormalPagePriority);
		bcopy(src, payload + ofs, l);
		ofs += l;
		NdisGetNextBuffer(pCurrentBuffer, &pNextBuffer);
		pCurrentBuffer = pNextBuffer;
	}
	/*
	 * Identify EtherType. If the packet is not IP, simply allow
	 * and don't bother the firewall. XXX should be done before.
	 */
	EtherType = *(unsigned short*)(payload + 12);
	EtherType = RtlUshortByteSwap(EtherType);
	if (EtherType != 0x0800) {
		//DbgPrint("ethertype = %X, skipping ipfw\n",EtherType);
		free(m, 0);
		return PASS;
	}

	/*
	 * Now build a buffer descriptor to replace the original chain.
	 */
	pAdapt = Context;
	PacketPool = direction == OUTGOING ?
		pAdapt->SendPacketPoolHandle : pAdapt->RecvPacketPoolHandle;
        NdisAllocateBuffer(&Status, &pNdisBuffer,
                PacketPool, payload, m->m_pkthdr.len+14);
        if (Status != NDIS_STATUS_SUCCESS)
                goto drop_pkt;
        /*
	 * Save the old buffer pointers, and put the new one
	 * into the chain.
         */
        pNdisBuffer->Next = NULL;
	old_head = NDIS_PACKET_FIRST_NDIS_BUFFER(pNdisPacket);
	old_tail = NDIS_PACKET_LAST_NDIS_BUFFER(pNdisPacket);
	NdisReinitializePacket(pNdisPacket);
	NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);
#if 0
	if (direction == INCOMING) {
		DBGPRINT(("incoming: proto %u (%s), src %08X, dst %08X, sport %u, dport %u, len %u\n", *(payload+14+9), texify_proto(*(payload+14+9)), *(unsigned int*)(payload+14+12), *(unsigned int*)(payload+14+16), ntohs((*((unsigned short int*)(payload+14+20)))), ntohs((*((unsigned short int*)(payload+14+22)))), TotalPacketLength));
	} else {
		DBGPRINT(("outgoing: proto %u (%s), src %08X, dst %08X, sport %u, dport %u, len %u\n", *(payload+14+9), texify_proto(*(payload+14+9)), *(unsigned int*)(payload+14+12), *(unsigned int*)(payload+14+16), ntohs((*((unsigned short int*)(payload+14+20)))), ntohs((*((unsigned short int*)(payload+14+22)))), TotalPacketLength));
	}
#endif
	if (direction == INCOMING)
		ret = ipfw_check_hook(NULL, &m, NULL, PFIL_IN, NULL);
	else
		ret = ipfw_check_hook(NULL, &m, (struct ifnet*)_if_out, PFIL_OUT, NULL);

	if (m != NULL) {
		/* Accept. Restore the old buffer chain, free
		 * the mbuf and return PASS.
		 */
		//DBGPRINT(("accepted\n"));
		NdisReinitializePacket(pNdisPacket);
		NDIS_PACKET_FIRST_NDIS_BUFFER(pNdisPacket) = old_head;
		NDIS_PACKET_LAST_NDIS_BUFFER(pNdisPacket) = old_tail;
		NdisFreeBuffer(pNdisBuffer);
		m_freem(m);
		return PASS;
	} else if (ret == 0) {
		/* dummynet has kept the packet, will reinject later. */
		//DBGPRINT(("kept by dummynet\n"));
		return DUMMYNET;
	} else {
		/*
		 * Packet dropped by ipfw or dummynet. Nothing to do as
		 * FREE_PKT already freed the fake mbuf
		 */
		//DBGPRINT(("dropped by dummynet, ret = %i\n", ret));
		return DROP;
	}
drop_pkt:
	/* for some reason we cannot proceed. Free any resources
	 * including those received from above, and return
	 * faking success. XXX this must be fixed later.
	 */
	NdisFreePacket(pNdisPacket);
	return DROP;
}

/*
 * Windows reinjection function.
 * The packet is already available as m->pkt, so we only
 * need to send it to the right place.
 * Normally a ndis intermediate driver allocates
 * a fresh descriptor, while the actual data's ownership is
 * retained by the protocol, or the miniport below.
 * Since an intermediate driver behaves as a miniport driver
 * at the upper edge (towards the protocol), and as a protocol
 * driver at the lower edge (towards the NIC), when we handle a
 * packet we have a reserved area in both directions (we can use
 * only one for each direction at our own discretion).
 * Normally this area is used to save a pointer to the original
 * packet, so when the driver is done with it, the original descriptor
 * can be retrieved, and the resources freed (packet descriptor,
 * buffer descriptor(s) and the actual data). In our driver this
 * area is used to mark the reinjected packets as 'orphan', because
 * the original descriptor is gone long ago. This way we can handle
 * correctly the resource freeing when the callback function
 * is called by NDIS.
 */

void 
netisr_dispatch(int num, struct mbuf *m)
{
	unsigned char*		payload = (unsigned char*)(m+1);
	PADAPT				pAdapt = m->context;
	NDIS_STATUS			Status;
	PNDIS_PACKET		pPacket = m->pkt;
	PNDIS_BUFFER		pNdisBuffer;
	NDIS_HANDLE			PacketPool;

	if (num < 0)
		goto drop_pkt;

	//debug print
#if 0
	DbgPrint("reinject %s\n", m->direction == OUTGOING ?
		"outgoing" : "incoming");
#endif
	NdisAcquireSpinLock(&pAdapt->Lock);
	if (m->direction == OUTGOING) {
		//we must first check if the adapter is going down,
		// in this case abort the reinjection
		if (pAdapt->PTDeviceState > NdisDeviceStateD0) {
			pAdapt->OutstandingSends--;
			// XXX should we notify up ?
			NdisReleaseSpinLock(&pAdapt->Lock);
			goto drop_pkt;
		}
	} else {
		/* if the upper miniport edge is not initialized or
		 * the miniport edge is in low power state, abort
		 * XXX we should notify the error.
		 */
		if (!pAdapt->MiniportHandle ||
		    pAdapt->MPDeviceState > NdisDeviceStateD0) {
			NdisReleaseSpinLock(&pAdapt->Lock);
			goto drop_pkt;
		}
	}
	NdisReleaseSpinLock(&pAdapt->Lock);

	if (m->direction == OUTGOING) {
		PSEND_RSVD	SendRsvd;
		/* use the 8-bytes protocol reserved area, the first
		 * field is used to mark/the packet as 'orphan', the
		 * second stores the pointer to the mbuf, so in the
		 * the SendComplete handler we know that this is a
		 * reinjected packet and can free correctly.
		 */
		SendRsvd = (PSEND_RSVD)(pPacket->ProtocolReserved);
		SendRsvd->OriginalPkt = NULL;
		SendRsvd->pMbuf = m;
		//do the actual send
		NdisSend(&Status, pAdapt->BindingHandle, pPacket);
		if (Status != NDIS_STATUS_PENDING) {
			/* done, call the callback now */
			PtSendComplete(m->context, m->pkt, Status);
		}
		return; /* unconditional return here. */
	} else {
		/* There's no need to check the 8-bytes miniport 
		 * reserved area since the path going up will be always
		 * syncronous, and all the cleanup will be done inline.
		 * If the reinjected packed comes from a PtReceivePacket, 
		 * there will be no callback.
		 * Otherwise PtReceiveComplete will be called but will just
		 * return since all the cleaning is alreqady done */
		// do the actual receive. 
		ULONG Proc = KeGetCurrentProcessorNumber();
		pAdapt->ReceivedIndicationFlags[Proc] = TRUE;
		NdisMEthIndicateReceive(pAdapt->MiniportHandle, NULL, payload, 14, payload+14, m->m_len, m->m_len);
		NdisMEthIndicateReceiveComplete(pAdapt->MiniportHandle);
		pAdapt->ReceivedIndicationFlags[Proc] = FALSE;
	}
drop_pkt:
	/* NDIS_PACKET exists and must be freed only if
	 * the packet come from a PtReceivePacket, oherwise
	 * m->pkt will ne null.
	 */
	if (m->pkt != NULL)
	{
		NdisUnchainBufferAtFront(m->pkt, &pNdisBuffer);
		NdisFreeBuffer(pNdisBuffer);
		NdisFreePacket(m->pkt);
	}
	m_freem(m);
}

void win_freem(void *);	/* wrapper for m_freem() for protocol.c */
void
win_freem(void *_m)
{
	struct mbuf *m = _m;
	m_freem(m);
}

/*
 * not implemented in linux.
 * taken from /usr/src/lib/libc/string/strlcpy.c
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;
 
        /* Copy as many bytes as will fit */
        if (n != 0 && --n != 0) {
                do {
                        if ((*d++ = *s++) == 0)
                                break;
                } while (--n != 0);
        }

        /* Not enough room in dst, add NUL and traverse rest of src */
        if (n == 0) {
                if (siz != 0)
                        *d = '\0';              /* NUL-terminate dst */
                while (*s++)
                        ;
        }

        return(s - src - 1);    /* count does not include NUL */
}

void CleanupReinjected(PNDIS_PACKET Packet, struct mbuf* m, PADAPT pAdapt)
{
	PNDIS_BUFFER pNdisBuffer;

	NdisQueryPacket(Packet, NULL, NULL, &pNdisBuffer, NULL);
	NdisUnchainBufferAtFront(Packet, &pNdisBuffer);
	NdisFreeBuffer(pNdisBuffer);
	win_freem(m);
	NdisFreePacket(Packet);
	ADAPT_DECR_PENDING_SENDS(pAdapt);
}

int
ipfw2_qhandler_w32_oldstyle(int direction,
	NDIS_HANDLE         ProtocolBindingContext,
    unsigned char*      HeaderBuffer,
    unsigned int        HeaderBufferSize,
    unsigned char*      LookAheadBuffer,
    unsigned int        LookAheadBufferSize,
    unsigned int        PacketSize)
{
	struct mbuf* m;
	unsigned char*		payload = NULL;
	unsigned short		EtherType = 0;
	int					ret = 0;
	
	/* We are in a special case when NIC signals an incoming
	 * packet using old style calls. This is done passing
	 * a pointer to the MAC header and a pointer to the
	 * rest of the packet.
	 * We simply allocate space for the mbuf and the
	 * subsequent payload section.
	 */
	m = malloc(sizeof(struct mbuf) + HeaderBufferSize + LookAheadBufferSize, 0, 0 );
	if (m == NULL) //resource shortage, drop the packet
		return DROP;
	
	/* set mbuf fields to point past the MAC header.
	 * Also set additional W32 info.
	 * m->pkt here is set to null because the notification
	 * from the NIC has come with a header+loolahead buffer,
	 * no NDIS_PACKET has been provided.
	 */
	payload = (unsigned char*)(m + 1);
	m->m_len = m->m_pkthdr.len = HeaderBufferSize+LookAheadBufferSize-14;
	m->m_data = payload + 14; /* past the MAC header */
	m->direction = direction;
	m->context = ProtocolBindingContext;
	m->pkt = NULL;
	
	/*
	 * Now copy the data from the Windows buffers to the mbuf.
	 */
	bcopy(HeaderBuffer, payload, HeaderBufferSize);
	bcopy(LookAheadBuffer, payload+HeaderBufferSize, LookAheadBufferSize);
	//hexdump(payload,HeaderBufferSize+LookAheadBufferSize,"qhandler");
	/*
	 * Identify EtherType. If the packet is not IP, simply allow
	 * and don't bother the firewall. XXX should be done before.
	 */
	EtherType = *(unsigned short*)(payload + 12);
	EtherType = RtlUshortByteSwap(EtherType);
	if (EtherType != 0x0800) {
		//DbgPrint("ethertype = %X, skipping ipfw\n",EtherType);
		free(m, 0);
		return PASS;
	}

	//DbgPrint("incoming_raw: proto %u (%s), src %08X, dst %08X, sport %u, dport %u, len %u\n", *(payload+14+9), texify_proto(*(payload+14+9)), *(unsigned int*)(payload+14+12), *(unsigned int*)(payload+14+16), ntohs((*((unsigned short int*)(payload+14+20)))), ntohs((*((unsigned short int*)(payload+14+22)))), HeaderBufferSize+LookAheadBufferSize);
	
	/* Query the firewall */
	ret = ipfw_check_hook(NULL, &m, NULL, PFIL_IN, NULL);

	if (m != NULL) {
		/* Accept. Free the mbuf and return PASS. */
		//DbgPrint("accepted\n");
		m_freem(m);
		return PASS;
	} else if (ret == 0) {
		/* dummynet has kept the packet, will reinject later. */
		//DbgPrint("kept by dummynet\n");
		return DUMMYNET;
	} else {
		/*
		 * Packet dropped by ipfw or dummynet. Nothing to do as
		 * FREE_PKT already freed the fake mbuf
		 */
		//DbgPrint("dropped by dummynet, ret = %i\n", ret);
		return DROP;
	}
}

/* forward declaration because those functions are used only here,
 * no point to make them visible in passthru/protocol/miniport */
int do_ipfw_set_ctl(struct sock *sk, int cmd,
	void __user *user, unsigned int len);
int do_ipfw_get_ctl(struct sock *sk, int cmd,
	void __user *user, int *len);

NTSTATUS
DevIoControl(
    IN PDEVICE_OBJECT    pDeviceObject,
    IN PIRP              pIrp
    )
/*++

Routine Description:

    This is the dispatch routine for handling device ioctl requests.

Arguments:

    pDeviceObject - Pointer to the device object.

    pIrp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    PIO_STACK_LOCATION  pIrpSp;
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    unsigned long       BytesReturned = 0;
    unsigned long       FunctionCode;
    unsigned long       len;
    struct sockopt		*sopt;
    int					ret = 0;
    
    UNREFERENCED_PARAMETER(pDeviceObject);
    
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    
    /*
     * Using METHOD_BUFFERED as communication method, the userland
     * side calls DeviceIoControl passing an input buffer and an output
     * and their respective length (ipfw uses the same length for both).
     * The system creates a single I/O buffer, with len=max(inlen,outlen).
     * In the kernel we can read information from this buffer (which is
     * directly accessible), overwrite it with our results, and set
     * IoStatus.Information with the number of bytes that the system must
     * copy back to userland.
     * In our sockopt emulation, the initial part of the buffer contains
     * a struct sockopt, followed by the data area.
     */

    len = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    if (len < sizeof(struct sockopt))
    {
	return STATUS_NOT_SUPPORTED; // XXX find better value
    }
    sopt = pIrp->AssociatedIrp.SystemBuffer;

    FunctionCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

    len = sopt->sopt_valsize;

    switch (FunctionCode)
    {
		case IP_FW_SETSOCKOPT:
			ret = do_ipfw_set_ctl(NULL, sopt->sopt_name, sopt+1, len);
			break;
			
		case IP_FW_GETSOCKOPT:
			ret = do_ipfw_get_ctl(NULL, sopt->sopt_name, sopt+1, &len);
			sopt->sopt_valsize = len;
			//sanity check on len
			if (len + sizeof(struct sockopt) <= pIrpSp->Parameters.DeviceIoControl.InputBufferLength)
				BytesReturned = len + sizeof(struct sockopt);
			else
				BytesReturned = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
			break;

		default:
				NtStatus = STATUS_NOT_SUPPORTED;
				break;
    }
    
    pIrp->IoStatus.Information = BytesReturned;
    pIrp->IoStatus.Status = NtStatus;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return NtStatus;
} 

void dummynet(void * unused);
void ipfw_tick(void * vnetx);

VOID dummynet_dpc(
    __in struct _KDPC  *Dpc,
    __in_opt PVOID  DeferredContext,
    __in_opt PVOID  SystemArgument1,
    __in_opt PVOID  SystemArgument2
    )
{
	dummynet(NULL);
}

VOID ipfw_dpc(
    __in struct _KDPC  *Dpc,
    __in_opt PVOID  DeferredContext,
    __in_opt PVOID  SystemArgument1,
    __in_opt PVOID  SystemArgument2
    )
{
	ipfw_tick(DeferredContext);
}
