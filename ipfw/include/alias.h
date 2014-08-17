#ifndef _ALIAS_H_
#define	_ALIAS_H_

#define LIBALIAS_BUF_SIZE 128

/*
 * If PKT_ALIAS_LOG is set, a message will be printed to /var/log/alias.log
 * every time a link is created or deleted.  This is useful for debugging.
 */
#define	PKT_ALIAS_LOG			0x01

/*
 * If PKT_ALIAS_DENY_INCOMING is set, then incoming connections (e.g. to ftp,
 * telnet or web servers will be prevented by the aliasing mechanism.
 */
#define	PKT_ALIAS_DENY_INCOMING		0x02

/*
 * If PKT_ALIAS_SAME_PORTS is set, packets will be attempted sent from the
 * same port as they originated on.  This allows e.g. rsh to work *99% of the
 * time*, but _not_ 100% (it will be slightly flakey instead of not working
 * at all).  This mode bit is set by PacketAliasInit(), so it is a default
 * mode of operation.
 */
#define	PKT_ALIAS_SAME_PORTS		0x04

/*
 * If PKT_ALIAS_USE_SOCKETS is set, then when partially specified links (e.g.
 * destination port and/or address is zero), the packet aliasing engine will
 * attempt to allocate a socket for the aliasing port it chooses.  This will
 * avoid interference with the host machine.  Fully specified links do not
 * require this.  This bit is set after a call to PacketAliasInit(), so it is
 * a default mode of operation.
 */
#ifndef	NO_USE_SOCKETS
#define	PKT_ALIAS_USE_SOCKETS		0x08
#endif
/*-
 * If PKT_ALIAS_UNREGISTERED_ONLY is set, then only packets with
 * unregistered source addresses will be aliased.  Private
 * addresses are those in the following ranges:
 *
 *		10.0.0.0     ->   10.255.255.255
 *		172.16.0.0   ->   172.31.255.255
 *		192.168.0.0  ->   192.168.255.255
 */
#define	PKT_ALIAS_UNREGISTERED_ONLY	0x10

/*
 * If PKT_ALIAS_RESET_ON_ADDR_CHANGE is set, then the table of dynamic
 * aliasing links will be reset whenever PacketAliasSetAddress() changes the
 * default aliasing address.  If the default aliasing address is left
 * unchanged by this function call, then the table of dynamic aliasing links
 * will be left intact.  This bit is set after a call to PacketAliasInit().
 */
#define	PKT_ALIAS_RESET_ON_ADDR_CHANGE	0x20


/*
 * If PKT_ALIAS_PROXY_ONLY is set, then NAT will be disabled and only
 * transparent proxying is performed.
 */
#define	PKT_ALIAS_PROXY_ONLY		0x40

/*
 * If PKT_ALIAS_REVERSE is set, the actions of PacketAliasIn() and
 * PacketAliasOut() are reversed.
 */
#define	PKT_ALIAS_REVERSE		0x80

#endif				/* !_ALIAS_H_ */
