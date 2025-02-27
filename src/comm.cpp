/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe.    *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku flicense in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*    ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*    ROM has been brought to you by the ROM consortium                      *
*        Russ Taylor (rtaylor@hypercube.org)                                *
*        Gabrielle Taylor (gtaylor@hypercube.org)                           *
*        Brian Moore (zump@rom.org)                                         *
*    By using this code, you have agreed to follow the terms of the         *
*    ROM license, in the file Rom24/doc/rom.license                         *
****************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#define PROOLBUFSIZE 1024 // prool

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>                /* OLC -- for close read write etc */
#include <stdarg.h>                /* printf_to_char */
#include <sys/wait.h>

#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "olc.h"
#include "telnet.h"
#include "mccp.h"
#include "riddles.h"
#include "tournament.h"
#include "lookup.h"
#include "account.h"
#ifdef DNS_SLAVE
  #include "dns_slave.h"
#endif

#ifdef THREADED_DNS
  #include "rdns_cache.h"
#endif


/*
 * Malloc debugging stuff.
 */
#if defined(sun)
#undef MALLOC_DEBUG
#endif

#if defined(MALLOC_DEBUG)
#include <malloc.h>
extern int malloc_debug args ((int));
extern int malloc_verify args ((void));
#endif



/*
 * Signal handling.
 * Apollo has a problem with __attribute(atomic) in signal.h,
 *   I dance around it.
 */
#if defined(apollo)
#define __attribute(x)
#endif

#if defined(unix)
#include <signal.h>
#endif

#if defined(apollo)
#undef __attribute
#endif



/*
 * Socket and TCP/IP stuff.
 */
#if    defined(macintosh) || defined(MSDOS)
const char echo_off_str[] = { '\0' };
const char echo_on_str[] = { '\0' };
const char go_ahead_str[] = { '\0' };
#endif


#ifdef MCCP
const   char    eor_on_str      [] = { IAC, WILL, TELOPT_EOR, '\0' };
const   char    compress_on_str [] = { IAC, WILL, TELOPT_COMPRESS, '\0' };
const   char    compress2_on_str [] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };

const   char    compress_do     [] = { IAC, DO, TELOPT_COMPRESS, '\0' };
const   char    compress_dont   [] = { IAC, DONT, TELOPT_COMPRESS, '\0' };

bool    compressStart   args( ( DESCRIPTOR_DATA *d ) );
bool    compressEnd     args( ( DESCRIPTOR_DATA *d ) );
bool    writeCompressed args ((DESCRIPTOR_DATA *d, char *txt, int length));
#endif

/* mxp: Mud HTML */
const   char   mxp_will        []={ IAC, WILL, TELOPT_MXP, '\0' };
const   char   mxp_do          []={ IAC, DO, TELOPT_MXP, '\0' };
const   char   mxp_dont        []={ IAC, DONT, TELOPT_MXP, '\0' };


#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "telnet.h"
const char echo_off_str[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const char echo_on_str[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const char go_ahead_str[] = { IAC, GA, '\0' };

void interp_mxp args ((CHAR_DATA *ch, char *argument));


/*
 * OS-dependent declarations.
 */
#if    defined(_AIX)
#include <sys/select.h>
int accept args ((int s, struct sockaddr * addr, int *addrlen));
int bind args ((int s, struct sockaddr * name, int namelen));
void bzero args ((char *b, int length));
int getpeername args ((int s, struct sockaddr * name, int *namelen));
int getsockname args ((int s, struct sockaddr * name, int *namelen));
int gettimeofday args ((struct timeval * tp, struct timezone * tzp));
int listen args ((int s, int backlog));
int setsockopt args ((int s, int level, int optname, void *optval,
                      int optlen));
int socket args ((int domain, int type, int protocol));
#endif

#if    defined(apollo)
#include <unistd.h>
void bzero args ((char *b, int length));
#endif

#if    defined(__hpux)
int accept args ((int s, void *addr, int *addrlen));
int bind args ((int s, const void *addr, int addrlen));
void bzero args ((char *b, int length));
int getpeername args ((int s, void *addr, int *addrlen));
int getsockname args ((int s, void *name, int *addrlen));
int gettimeofday args ((struct timeval * tp, struct timezone * tzp));
int listen args ((int s, int backlog));
int setsockopt args ((int s, int level, int optname,
                      const void *optval, int optlen));
int socket args ((int domain, int type, int protocol));
#endif

#if    defined(interactive)
#include <net/errno.h>
#include <sys/fnctl.h>
#endif

#if    defined(linux)
/*
    Linux shouldn't need these. If you have a problem compiling, try
    uncommenting these functions.
*/
/*
int    accept        args( ( int s, struct sockaddr *addr, int *addrlen ) );
int    bind        args( ( int s, struct sockaddr *name, int namelen ) );
int    getpeername    args( ( int s, struct sockaddr *name, int *namelen ) );
int    getsockname    args( ( int s, struct sockaddr *name, int *namelen ) );
int    listen        args( ( int s, int backlog ) );
*/

int close args ((int fd));
//int gettimeofday args ((struct timeval * tp, struct timezone * tzp)); // prool
/* int    read        args( ( int fd, char *buf, int nbyte ) ); */
int select args ((int width, fd_set * readfds, fd_set * writefds,
                  fd_set * exceptfds, struct timeval * timeout));
int socket args ((int domain, int type, int protocol));
/* int    write        args( ( int fd, char *buf, int nbyte ) ); *//* read,write in unistd.h */
#endif

#if    defined(macintosh)
#include <console.h>
#include <fcntl.h>
#include <unix.h>
struct timeval {
    time_t tv_sec;
    time_t tv_usec;
};
#if    !defined(isascii)
#define    isascii(c)        ( (c) < 0200 )
#endif
static long theKeys[4];

int gettimeofday args ((struct timeval * tp, void *tzp));
#endif

#if    defined(MIPS_OS)
extern int errno;
#endif

#if    defined(MSDOS)
int gettimeofday args ((struct timeval * tp, void *tzp));
int kbhit args ((void));
#endif

#if    defined(NeXT)
int close args ((int fd));
int fcntl args ((int fd, int cmd, int arg));
#if    !defined(htons)
u_short htons args ((u_short hostshort));
#endif
#if    !defined(ntohl)
u_long ntohl args ((u_long hostlong));
#endif
int read args ((int fd, char *buf, int nbyte));
int select args ((int width, fd_set * readfds, fd_set * writefds,
                  fd_set * exceptfds, struct timeval * timeout));
int write args ((int fd, char *buf, int nbyte));
#endif

#if    defined(sequent)
int accept args ((int s, struct sockaddr * addr, int *addrlen));
int bind args ((int s, struct sockaddr * name, int namelen));
int close args ((int fd));
int fcntl args ((int fd, int cmd, int arg));
int getpeername args ((int s, struct sockaddr * name, int *namelen));
int getsockname args ((int s, struct sockaddr * name, int *namelen));
int gettimeofday args ((struct timeval * tp, struct timezone * tzp));
#if    !defined(htons)
u_short htons args ((u_short hostshort));
#endif
int listen args ((int s, int backlog));
#if    !defined(ntohl)
u_long ntohl args ((u_long hostlong));
#endif
int read args ((int fd, char *buf, int nbyte));
int select args ((int width, fd_set * readfds, fd_set * writefds,
                  fd_set * exceptfds, struct timeval * timeout));
int setsockopt args ((int s, int level, int optname, caddr_t optval,
                      int optlen));
int socket args ((int domain, int type, int protocol));
int write args ((int fd, char *buf, int nbyte));
#endif

/* This includes Solaris Sys V as well */
#if defined(sun)
int accept args ((int s, struct sockaddr * addr, int *addrlen));
int bind args ((int s, struct sockaddr * name, int namelen));
void bzero args ((char *b, int length));
int close args ((int fd));
int getpeername args ((int s, struct sockaddr * name, int *namelen));
int getsockname args ((int s, struct sockaddr * name, int *namelen));
int listen args ((int s, int backlog));
int read args ((int fd, char *buf, int nbyte));
int select args ((int width, fd_set * readfds, fd_set * writefds,
                  fd_set * exceptfds, struct timeval * timeout));

#if !defined(__SVR4)
int gettimeofday args ((struct timeval * tp, struct timezone * tzp));

#if defined(SYSV)
int setsockopt args ((int s, int level, int optname,
                      const char *optval, int optlen));
#else
int setsockopt args ((int s, int level, int optname, void *optval,
                      int optlen));
#endif
#endif
int socket args ((int domain, int type, int protocol));
int write args ((int fd, char *buf, int nbyte));
#endif

#if defined(ultrix)
int accept args ((int s, struct sockaddr * addr, int *addrlen));
int bind args ((int s, struct sockaddr * name, int namelen));
void bzero args ((char *b, int length));
int close args ((int fd));
int getpeername args ((int s, struct sockaddr * name, int *namelen));
int getsockname args ((int s, struct sockaddr * name, int *namelen));
int gettimeofday args ((struct timeval * tp, struct timezone * tzp));
int listen args ((int s, int backlog));
int read args ((int fd, char *buf, int nbyte));
int select args ((int width, fd_set * readfds, fd_set * writefds,
                  fd_set * exceptfds, struct timeval * timeout));
int setsockopt args ((int s, int level, int optname, void *optval,
                      int optlen));
int socket args ((int domain, int type, int protocol));
int write args ((int fd, char *buf, int nbyte));
#endif

void tax   args((CHAR_DATA *ch));

/*
 * Global variables.
 */
DESCRIPTOR_DATA *descriptor_list;    /* All open descriptors     */
DESCRIPTOR_DATA *d_next;        /* Next descriptor in loop  */
FILE *fpReserve;                /* Reserved file handle     */
bool god;                        /* All new chars are gods!  */
bool merc_down;                    /* Shutdown         */
bool wizlock;                    /* Game is wizlocked        */
bool newlock;                    /* Game is newlocked        */
char str_boot_time[MAX_INPUT_LENGTH];
time_t current_time;            /* time of this pulse */
time_t boot_time;            /* time of this pulse */
bool MOBtrigger = TRUE;            /* act() switch                 */

#ifdef DNS_SLAVE
  pid_t slave_pid;
  int slave_socket = -1;
#endif

char *known_name args ((CHAR_DATA *ch, CHAR_DATA *looker));

/*
 * OS-dependent local functions.
 */
#if defined(macintosh) || defined(MSDOS)
void game_loop_mac_msdos args ((void));
bool read_from_descriptor args ((DESCRIPTOR_DATA * d));
bool write_to_descriptor args ((DESCRIPTOR_DATA *d, _ char *txt, int length));
#endif

#if defined(unix)
void game_loop_unix args ((int control));
int init_socket args ((int port));
void init_descriptor args ((int control));
bool read_from_descriptor args ((DESCRIPTOR_DATA * d));
bool write_to_descriptor args ((DESCRIPTOR_DATA *d, char *txt, int length));
#endif




/*
 * Other local functions (OS-independent).
 */
bool load_account_obj args ((DESCRIPTOR_DATA *d, char *name ));
void print_class_stats args (( CHAR_DATA *ch, int cClass ));
bool check_parse_name args ((char *name));
bool check_reconnect args ((DESCRIPTOR_DATA * d, char *name, bool fConn));
bool check_playing args ((DESCRIPTOR_DATA * d, char *name));
int main args ((int argc, char **argv));
void nanny args ((DESCRIPTOR_DATA * d, char *argument));
bool process_output args ((DESCRIPTOR_DATA * d, bool fPrompt));
void read_from_buffer args ((DESCRIPTOR_DATA * d));
void stop_idling args ((CHAR_DATA * ch));
void bust_a_prompt args ((CHAR_DATA * ch));
void save_vote   args ((void));
void save_mudstats   args ((void));
void save_riddle   args ((void));
void save_gameinfo   args ((void));
void save_stores args ((void));
void gen_name args ((DESCRIPTOR_DATA *d, char *argument));
void check_action args(( CHAR_DATA *ch ));
void init_signals   args( (void) );
void sig_handler    args((int sig));
void do_auto_shutdown args( (void) );
void file_to_desc(char *filename, DESCRIPTOR_DATA *desc); // by prool


/* Needs to be global because of do_copyover */
int port, control;


char *dns_gethostname(char *s) {
#ifdef THREADED_DNS
  return gethostname_cached(s,4,FALSE);
#else
  return s;
#endif
}

#ifdef DNS_SLAVE
#define MAX_HOST 60
#define MAX_OPEN_FILES 128
bool get_slave_result( void );
void slave_timeout( void );

void boot_slave( void )
{
  int i, sv[2];

  if( slave_socket != -1 )
  {
    close( slave_socket );
    slave_socket = -1;
  }

  if( socketpair( AF_UNIX, SOCK_DGRAM, 0, sv ) < 0 )
  {
    logf( "boot_slave: socketpair: %s", strerror(errno) );
    return;
  }
  /* set to nonblocking */
  if( fcntl( sv[0], F_SETFL, FNDELAY ) == -1 )
  {
    logf( "boot_slave: fcntl( F_SETFL, FNDELAY ): %s", strerror(errno) );
    close(sv[0]);
    close(sv[1]);
    return;
  }

  slave_pid = fork();

  switch( slave_pid )
  {
    case -1:
      logf( "boot_slave: fork: %s", strerror(errno) );
      close(sv[0]);
      close(sv[1]);
      return;

    case 0:			/* child */
      close(sv[0]);
      close(0);
      close(1);
      if( dup2( sv[1], 0 ) == -1 )
      {
	   logf( "boot_slave: child: unable to dup stdin: %s", strerror(errno) );
	   _exit(1);
      }
      if( dup2( sv[1], 1 ) == -1 )
      {
	   logf( "boot_slave: child: unable to dup stdout: %s", strerror(errno) );
	   _exit(1);
      }
      for( i = 3; i < MAX_OPEN_FILES; ++i )
	close( i );

      execlp( "../src/dns_slave", "dns_slave", NULL );
      logf( "boot_slave: child: unable to exec: %s", strerror(errno) );
      _exit(1);
  }

  close( sv[1] );

  if( fcntl( sv[0], F_SETFL, FNDELAY ) == -1 )
  {
    logf( "boot_slave: fcntl: %s", strerror(errno) );
    close( sv[0] );
    return;
  }
  slave_socket = sv[0];
}

char *printhostaddr( char *hostbuf, struct in_addr *addr )
{
  char *tmp;

  tmp = inet_ntoa( *addr );
  strcpy( hostbuf, tmp );
  return( hostbuf + strlen(hostbuf) );
}

void make_slave_request( struct sockaddr_in *sock )
{
  char buf[512];
  int num;

  sprintf( buf, "%c%s\n", SLAVE_IPTONAME, inet_ntoa( sock->sin_addr ) );

  if( slave_socket != -1 )
  {
    /* ask slave to do a hostname lookup for us */
    if( write( slave_socket, buf, ( num = strlen(buf) ) ) != num )
    {
      logf( "[SLAVE] make_slave_request: Losing slave on write: %s", strerror(errno) );
      close( slave_socket );
      slave_socket = -1;
      return;
    }
  }
}

void check_desc_state( DESCRIPTOR_DATA *d )
{
  if( ( d->site_info & ( HostName | HostNameFail ) ) )
  {
    if( d->connected == CON_GETDNS )
    {
      d->wait = 0;
      d->connected = CON_GET_NAME;

	    /*
  	     * Swiftest: I added the following to ban sites.  I don't
  	     * endorse banning of sites, but Copper has few descriptors now
   	     * and some people from certain sites keep abusing access by
    	     * using automated 'autodialers' and leaving connections hanging.
  	     *
	     * Furey: added suffix check by request of Nickel of HiddenWorlds.
	     */

    	    if (check_ban (d->host, BAN_ALL))
    	    {
        	write_to_buffer (d,
                             "We are sorry, your computer is banned... email gauravguru@gmail.com for questions.\n\r",
                             0);
        	close_socket (d);
        	return;
    	    }

      write_to_buffer(d, "Login name, account or newchar: ", 0 );
    }
  }
}

bool get_slave_result( void )
{
  DESCRIPTOR_DATA *d, *n;
  char buf[MAX_STRING_LENGTH + 1], token[MAX_STRING_LENGTH];
  int len, octet[4];
  u_long addr;

  len = read( slave_socket, buf, MAX_STRING_LENGTH );
//  logf("Buf %s", buf);
  if( len < 0 )
  {
    if( errno == EAGAIN || errno == EWOULDBLOCK )
      return TRUE;

    logf( "[SLAVE] get_slave_result: Losing slave on read: %s", strerror(errno) );
    close( slave_socket );
    slave_socket = -1;
    return TRUE;
  }
  else if( !len )
    return TRUE;

  buf[len] = 0;

  switch (buf[0])
  {
    case SLAVE_IPTONAME_FAIL:
      if( sscanf( buf + 1, "%d.%d.%d.%d", &octet[0], &octet[1], &octet[2], &octet[3] ) >= 4 )
      {
	   addr = (octet[0] << 24) + (octet[1] << 16) + (octet[2] << 8) + (octet[3]);

	   addr = ntohl( addr );

	   for( d = descriptor_list; d; d = n )
	   {
	      n = d->next;
	      if( d->site_info & (HostName | HostNameFail) )
	    	   continue;

	  	if(d->addr != addr )
	    	   continue;

	  	/* Do nothing to host name, addr is still there */
	      d->site_info |= HostNameFail;
	      check_desc_state( d );
	   }
      }
      else
	   logf( "[SLAVE] Invalid: '%s'", buf );

      break;

    case SLAVE_IPTONAME:
      if( sscanf( buf + 1, "%d.%d.%d.%d %s",
		 &octet[0], &octet[1], &octet[2], &octet[3], token) != 5 )
      {
	   logf( "[SLAVE] Invalid: %s", buf );
	   return TRUE;
      }
      addr = ( octet[0] << 24 ) + ( octet[1] << 16 ) + ( octet[2] << 8 ) + ( octet[3] );

      addr = ntohl( addr );

      for( d = descriptor_list; d; d = n )
      {
	   n = d->next;
	   if( d->site_info & HostName )
	      continue;

	   if ( d->addr != addr )
	      continue;

	   strncpy( d->host, token, MAX_HOST );
	   d->host[MAX_HOST] = 0;
	   d->site_info |= HostName;
	   check_desc_state( d );
      }
      break;

    default:
      logf( "[SLAVE] Invalid: %s", buf );
      break;
  }
  return FALSE;
}

void slave_timeout( void )
{
  DESCRIPTOR_DATA *d, *n;
  time_t nowtime, tdiff;
  static long lastTimeout;

  nowtime = time(0);
  for( d = descriptor_list; d; d = n )
  {
    n = d->next;
    if( d->connected == CON_GETDNS )
    {
      /* Worst case, one minute wait */
      tdiff = current_time - d->contime;

      /* hostname timeout: 60 seconds */
      if( ( tdiff > 30 ) || ( ( tdiff > 10 ) &&
			   ( d->site_info & (HostName | HostNameFail) ) ) )
      {
	   d->wait = 0;

	    /*
  	     * Swiftest: I added the following to ban sites.  I don't
  	     * endorse banning of sites, but Copper has few descriptors now
   	     * and some people from certain sites keep abusing access by
    	     * using automated 'autodialers' and leaving connections hanging.
  	     *
	     * Furey: added suffix check by request of Nickel of HiddenWorlds.
	     */

    	    if (check_ban (d->host, BAN_ALL))
    	    {
        	write_to_buffer (d,
                             "We are sorry, your site is banned... email gauravguru@gmail.com for questions.\n\r",
                             0);
        	close_socket (d);
        	return;
    	    }
	   write_to_buffer( d, "Enter your character's name, or type newchar: ", 0 );
	   d->connected = CON_GET_NAME;
           if (current_time > lastTimeout + 60)
           {
             lastTimeout = current_time;
             if (slave_socket != -1)
             {
               log_string("Terminating DNS Slave process.");
               kill(slave_pid, SIGKILL);
               waitpid(slave_pid, NULL, 0);
             }
	     boot_slave();
           }
      }
    }
  }
}
#endif


int main (int argc, char **argv)
{
    struct timeval now_time;
    bool fCopyOver = FALSE;

    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug (2);
#endif

    /*
     * Init time.
     */
    gettimeofday (&now_time, NULL);
    current_time = (time_t) now_time.tv_sec;
    boot_time = current_time;
    strcpy (str_boot_time, ctime (&current_time));

    #ifdef THREADED_DNS
        rdns_cache_set_ttl(12*60*60);
    #endif

    /*
     * Macintosh console initialization.
     */
#if defined(macintosh)
    console_options.nrows = 31;
    cshow (stdout);
    csetmode (C_RAW, stdin);
    cecho2file ("log file", 1, stderr);
#endif

    /*
     * Reserve one channel for our use.
     */
    if ((fpReserve = fopen (NULL_FILE, "r")) == NULL)
    {
        perror (NULL_FILE);
        exit (1);
    }

    /*
     * Get the port number.
     */
    port = 4000;
    if (argc > 1)
    {
        if (!is_number (argv[1]))
        {
            fprintf (stderr, "Usage: %s [port #]\n", argv[0]);
            exit (1);
        }
        else if ((port = atoi (argv[1])) <= 1024)
        {
            fprintf (stderr, "Port number must be above 1024.\n");
            exit (1);
        }

        /* Are we recovering from a copyover? */
        if (argv[2] && argv[2][0])
        {
            fCopyOver = TRUE;
            control = atoi (argv[3]);
        }
        else
            fCopyOver = FALSE;

    }

    /*
     * Run the game.
     */
#if defined(macintosh) || defined(MSDOS)
    boot_db ();
    log_string ("Merc is ready to rock.");
    game_loop_mac_msdos ();
#endif

#if defined(unix)

    if (!fCopyOver)
        control = init_socket (port);

    #ifdef DNS_SLAVE
      boot_slave();
    #endif

    boot_db ();
//    init_web(port - 7575 + 8000);
    #ifdef BIT_COMPILE
    {
      AREA_DATA *pArea;
      for (pArea = area_first; pArea; pArea = pArea->next)
         save_area(pArea);

      fprintf( stderr, "Conversions complete.\n");
      exit(0);
    }
    #endif

    sprintf (log_buf, "ROM is ready to rock on port %d.", port);
    log_string (log_buf);

    if (fCopyOver)
        copyover_recover ();

    game_loop_unix (control);
  //  shutdown_web();
    close (control);
#endif

    /*
     * That's all, folks.
     */
    save_vote();
    save_mudstats();
    save_riddle();
    save_gameinfo();
    save_equipid();
    save_guilds();
//    save_stores();
    ispell_done();
    free_space();
    #ifdef DNS_SLAVE
      if ( slave_socket != -1)
      {
        log_string("Terminating DNS Slave process." );
        kill (slave_pid, SIGKILL );
      }
    #endif
    log_string ("Normal termination of game.");
    exit (0);
    return 0;
}



#if defined(unix)
int init_socket (int port)
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int fd;

    if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("Init_socket: socket");
        exit (1);
    }

    if (setsockopt (fd, SOL_SOCKET, SO_REUSEADDR,
                    (char *) &x, sizeof (x)) < 0)
    {
        perror ("Init_socket: SO_REUSEADDR");
        close (fd);
        exit (1);
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
        struct linger ld;

        ld.l_onoff = 1;
        ld.l_linger = 1000;

        if (setsockopt (fd, SOL_SOCKET, SO_DONTLINGER,
                        (char *) &ld, sizeof (ld)) < 0)
        {
            perror ("Init_socket: SO_DONTLINGER");
            close (fd);
            exit (1);
        }
    }
#endif

    sa = sa_zero;
    sa.sin_family = AF_INET;
    sa.sin_port = htons (port);
//    inet_aton("216.162.171.220", &sa.sin_addr);
    if (bind (fd, (struct sockaddr *) &sa, sizeof (sa)) < 0)
    {
        perror ("Init socket: bind");
        close (fd);
        exit (1);
    }


    if (listen (fd, 3) < 0)
    {
        perror ("Init socket: listen");
        close (fd);
        exit (1);
    }

    return fd;
}
#endif



#if defined(macintosh) || defined(MSDOS)
void game_loop_mac_msdos (void)
{
    struct timeval last_time;
    struct timeval now_time;
    static DESCRIPTOR_DATA dcon;

    gettimeofday (&last_time, NULL);
    current_time = (time_t) last_time.tv_sec;

    /*
     * New_descriptor analogue.
     */
    dcon.descriptor = 0;
    dcon.connected = CON_ANSI;
    dcon.ansi = TRUE;
    dcon.host = str_dup ("localhost");
    dcon.outsize = 2000;
    dcon.outbuf = alloc_mem (dcon.outsize);
    dcon.next = descriptor_list;
    dcon.showstr_head = NULL;
    dcon.showstr_point = NULL;
    dcon.pEdit = NULL;            /* OLC */
    dcon.pString = NULL;        /* OLC */
    dcon.editor = 0;            /* OLC */
    descriptor_list = &dcon;

    /*
     * First Contact!
     */
#if 0 // by prool
    {
        extern char *help_greeting;
        if (help_greeting[0] == '.')
          send_to_desc (help_greeting + 1, &dcon);
        else
          send_to_desc (help_greeting, &dcon);
    }
#else
	{
	file_to_desc("greeting.txt", &dcon);
	}
#endif


    /* Main loop */
    while (!merc_down)
    {
        DESCRIPTOR_DATA *d;

        /*
         * Process input.
         */
        for (d = descriptor_list; d != NULL; d = d_next)
        {
            d_next = d->next;
            d->fcommand = FALSE;

#if defined(MSDOS)
            if (kbhit ())
#endif
            {
                if (d->character != NULL)
                    d->character->timer = 0;
                if (!read_from_descriptor (d))
                {
                    if (d->character != NULL && d->connected == CON_PLAYING)
                        save_char_obj (d->character);
                    d->outtop = 0;
                    close_socket (d);
                    continue;
                }
            }

            if (d->character != NULL && d->character->daze > 0)
                --d->character->daze;

            if (d->character != NULL && d->character->wait > 0)
            {
                --d->character->wait;
		if (d->character->level < IMPLEMENTOR)
                  continue;
            }

            read_from_buffer (d);
            if (d->incomm[0] != '\0')
            {
                d->fcommand = TRUE;
                stop_idling (d->character);

                /* OLC */
                if (d->showstr_point)
                    show_string (d, d->incomm);
                else if (d->pString)
                    string_add (d->character, d->incomm);
                else
                    switch (d->connected)
                    {
                        case CON_PLAYING:
                            if (!run_olc_editor (d) && !run_store_editor(d) && !run_riddle_editor(d))
                                substitute_alias (d, d->incomm);
                            break;
                        default:
                            nanny (d, d->incomm);
                            break;
                    }


                d->incomm[0] = '\0';
            }
        }



        /*
         * Autonomous game motion.
         */
        update_handler ();



        /*
         * Output.
         */
        for (d = descriptor_list; d != NULL; d = d_next)
        {
            d_next = d->next;

            if ((d->fcommand || d->outtop > 0))
            {
                if (!process_output (d, TRUE))
                {
                    if (d->character != NULL && d->connected == CON_PLAYING)
                        save_char_obj (d->character);
                    d->outtop = 0;
                    close_socket (d);
                }
            }
        }



        /*
         * Synchronize to a clock.
         * Busy wait (blargh).
         */
        now_time = last_time;
        for (;;)
        {
            int delta;

#if defined(MSDOS)
            if (kbhit ())
#endif
            {
                if (dcon.character != NULL)
                    dcon.character->timer = 0;
                if (!read_from_descriptor (&dcon))
                {
                    if (dcon.character != NULL && d->connected == CON_PLAYING)
                        save_char_obj (d->character);
                    dcon.outtop = 0;
                    close_socket (&dcon);
                }
#if defined(MSDOS)
                break;
#endif
            }

            gettimeofday (&now_time, NULL);
            delta = (now_time.tv_sec - last_time.tv_sec) * 1000 * 1000
                + (now_time.tv_usec - last_time.tv_usec);
            if (delta >= 1000000 / PULSE_PER_SECOND)
                break;
        }
        last_time = now_time;
        current_time = (time_t) last_time.tv_sec;
    }

    return;
}
#endif



#if defined(unix)
void game_loop_unix (int control)
{
    static struct timeval null_time;
    struct timeval last_time;
    init_signals();
    signal (SIGPIPE, SIG_IGN);
    gettimeofday (&last_time, NULL);
    current_time = (time_t) last_time.tv_sec;


    /* Main loop */
    while (!merc_down)
    {
        fd_set in_set;
        fd_set out_set;
        fd_set exc_set;
        DESCRIPTOR_DATA *d;
        int maxdesc;

#if defined(MALLOC_DEBUG)
        if (malloc_verify () != 1)
            abort ();
#endif

        /*
         * Poll all active descriptors.
         */
        FD_ZERO (&in_set);
        FD_ZERO (&out_set);
        FD_ZERO (&exc_set);
        FD_SET (control, &in_set);
        #ifdef DNS_SLAVE
           if ( slave_socket != -1 )
             FD_SET (slave_socket, &in_set );
        #endif
        maxdesc = control;
        for (d = descriptor_list; d; d = d->next)
        {
            maxdesc = UMAX (maxdesc, d->descriptor);
            FD_SET (d->descriptor, &in_set);
            FD_SET (d->descriptor, &out_set);
            FD_SET (d->descriptor, &exc_set);
        }

        if (select (maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0)
        {
            perror ("Game_loop: select: poll");
            exit (1);
        }

        #ifdef DNS_SLAVE
          /* slave result? */
          if( slave_socket != -1 )
	  {
	    if( FD_ISSET( slave_socket, &in_set ) )
	    {
	       {
                  while( !get_slave_result() )
		     ;
	       }
	    }
	   }
           slave_timeout();
         #endif

        /*
         * New connection?
         */
        if (FD_ISSET (control, &in_set))
            init_descriptor (control);

        /*
         * Kick out the freaky folks.
         */
        for (d = descriptor_list; d != NULL; d = d_next)
        {
            d_next = d->next;
            if (FD_ISSET (d->descriptor, &exc_set))
            {
                FD_CLR (d->descriptor, &in_set);
                FD_CLR (d->descriptor, &out_set);
                if (d->character && d->connected == CON_PLAYING)
                    save_char_obj (d->character);
                d->outtop = 0;
                close_socket (d);
            }
        }

        /*
         * Process input.
         */
        for (d = descriptor_list; d != NULL; d = d_next)
        {
            d_next = d->next;
            d->fcommand = FALSE;

            if (FD_ISSET (d->descriptor, &in_set))
            {
                if (d->character != NULL)
                    d->character->timer = 0;
                if (!read_from_descriptor (d))
                {
                    FD_CLR (d->descriptor, &out_set);
                    if (d->character != NULL && d->connected == CON_PLAYING)
                        save_char_obj (d->character);
                    d->outtop = 0;
                    close_socket (d);
                    continue;
                }
            }

            if (d->character != NULL && d->character->daze > 0)
                --d->character->daze;

            if (d->character != NULL && d->character->wait > 0)
            {
                --d->character->wait;
if (d->character->level < IMPLEMENTOR)
                continue;
            }

            read_from_buffer (d);
            if (d->incomm[0] != '\0')
            {
                d->fcommand = TRUE;
                stop_idling (d->character);

                /* OLC */
                if (d->showstr_point)
                    show_string (d, d->incomm);
                else if (d->pString)
                    string_add (d->character, d->incomm);
                else
                    switch (d->connected)
                    {
                        case CON_PLAYING:
                            if (!run_olc_editor (d) && !run_store_editor(d) && !run_riddle_editor(d))
                                substitute_alias (d, d->incomm);
                            break;
                        default:
                            nanny (d, d->incomm);
                            break;
                    }

                d->incomm[0] = '\0';
            }
        }



        /*
         * Autonomous game motion.
         */
        update_handler ();
      //  handle_web();




        /*
         * Output.
         */
        for (d = descriptor_list; d != NULL; d = d_next)
        {
            d_next = d->next;

            if ((d->fcommand || d->outtop > 0)
                && FD_ISSET (d->descriptor, &out_set))
            {
                if (!process_output (d, TRUE))
                {
                    if (d->character != NULL && d->connected == CON_PLAYING)
                        save_char_obj (d->character);
                    d->outtop = 0;
                    close_socket (d);
                }
            }
        }



        /*
         * Synchronize to a clock.
         * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
         * Careful here of signed versus unsigned arithmetic.
         */
        {
            struct timeval now_time;
            long secDelta;
            long usecDelta;

            gettimeofday (&now_time, NULL);
            usecDelta = ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
                + 1000000 / PULSE_PER_SECOND;
            secDelta = ((int) last_time.tv_sec) - ((int) now_time.tv_sec);
            while (usecDelta < 0)
            {
                usecDelta += 1000000;
                secDelta -= 1;
            }

            while (usecDelta >= 1000000)
            {
                usecDelta -= 1000000;
                secDelta += 1;
            }

            if (secDelta > 0 || (secDelta == 0 && usecDelta > 0))
            {
                struct timeval stall_time;

                stall_time.tv_usec = usecDelta;
                stall_time.tv_sec = secDelta;
                if (checkStall)
                {
                  stall_percent = 100.0 * usecDelta * 4 / 1000000;
		  checkStall = FALSE;
		}

                if (select (0, NULL, NULL, NULL, &stall_time) < 0)
                {
                    perror ("Game_loop: select: stall");
                    exit (1);
                }
            }
        }

        gettimeofday (&last_time, NULL);
        current_time = (time_t) last_time.tv_sec;
    }

    return;
}
#endif



#if defined(unix)

void init_descriptor (int control)
{
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
    #ifndef THREADED_DNS
      #ifndef DNS_SLAVE
        struct hostent *from;
      #endif
    #endif
    #ifdef THREADED_DNS
    #elif defined (DNS_SLAVE)
    #else
      char buf[MSL];
    #endif
    int desc;

    size_t size = sizeof (sock);
    getsockname (control, (struct sockaddr *) &sock, (socklen_t*)&size);
    if ((desc = accept (control, (struct sockaddr *) &sock, (socklen_t*)&size)) < 0)
    {
        perror ("New_descriptor: accept");
        return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    if (fcntl (desc, F_SETFL, FNDELAY) == -1)
    {
        perror ("New_descriptor: fcntl: FNDELAY");
        return;
    }

    /*
     * Cons a new descriptor.
     */
    dnew = new_descriptor ();

    dnew->descriptor = desc;
    dnew->connected = CON_GET_NAME;
    dnew->ansi = TRUE;
    dnew->showstr_head = NULL;
    dnew->showstr_point = NULL;
    dnew->outsize = 2000;
    dnew->pEdit = NULL;            /* OLC */
    dnew->pString = NULL;        /* OLC */
    dnew->editor = 0;            /* OLC */
    dnew->outbuf = (char *) alloc_mem (dnew->outsize);

    size = sizeof (sock);
    if (getpeername (desc, (struct sockaddr *) &sock, (socklen_t*)&size) < 0)
    {
        perror ("New_descriptor: getpeername");
        #ifdef THREADED_DNS
          bzero(dnew->Host, sizeof(dnew->Host));
        #else
          dnew->Host = str_dup("(unknown)");
        #endif
    }
    else
    {
        /*
         * Would be nice to use inet_ntoa here but it takes a struct arg,
         * which ain't very compatible between gcc and system libraries.
         */
        #ifdef THREADED_DNS
        #elif defined (DNS_SLAVE)
        #else
          int addr;
        #endif
    #ifdef THREADED_DNS
         memcpy(dnew->Host,&sock.sin_addr,sizeof(addr));
         gethostname_cached(dnew->Host,4,TRUE);
    #elif defined (DNS_SLAVE)
       dnew->wait		= 1;
       *dnew->host		= '\0';
       printhostaddr( dnew->host, &sock.sin_addr );
       dnew->site_info 	= HostAddr;
       dnew->addr 		= sock.sin_addr.s_addr;
       dnew->contime 	= current_time;
       dnew->Host		= str_dup(dnew->host);
    #else
               addr = ntohl (sock.sin_addr.s_addr);
               sprintf (buf, "%d.%d.%d.%d",
                  (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
                 (addr >> 8) & 0xFF, (addr) & 0xFF);
                sprintf (log_buf, "Sock.sinaddr:  %s", buf);
               log_string (log_buf);
               from = NULL;
//                from = gethostbyaddr( (char *) &sock.sin_addr,
//                    sizeof(sock.sin_addr), AF_INET );
                free_string(dnew->Host);
                dnew->Host = str_dup( from ? from->h_name : buf );
                dnew->host = str_dup( from ? from->h_name : buf );
    #endif
    }

    /*
     * Init descriptor data.
     */
    dnew->next = descriptor_list;
    descriptor_list = dnew;

#ifdef MCCP
//    write_to_buffer(dnew, eor_on_str, 0);
    write_to_buffer(dnew, compress_on_str, 0);
    write_to_buffer(dnew, mxp_will, 0);
//    write_to_buffer(dnew, compress2_on_str, 0);
#endif

    /*
     * First Contact!
     */
    {
#if 0 // prool
        extern char *help_greeting;
        if (help_greeting[0] == '.')
          send_to_desc (help_greeting + 1,dnew);
        else
          send_to_desc (help_greeting, dnew);
#else
	file_to_desc("greeting.txt", dnew);	
#endif
        #ifdef DNS_SLAVE
           make_slave_request( &sock );
           send_to_desc( "`&Please wait - looking up DNS information.`*\n\r", dnew );
           /* Make 'em wait, don't wanna lose site info */
           dnew->wait = 1 << 24;
           dnew->connected = CON_GETDNS;
        #else
          write_to_buffer( dnew, "Login name, account or new: ", 0 );
        #endif
    }

    return;
}
#endif



void close_socket (DESCRIPTOR_DATA * dclose)
{
    CHAR_DATA *ch;

    if (dclose->outtop > 0)
        process_output (dclose, FALSE);


    if (dclose->snoop_by != NULL)
    {
        write_to_buffer (dclose->snoop_by,
                         "Your victim has left the game.\n\r", 0);
    }

    {
        DESCRIPTOR_DATA *d;

        mudstats.current_players = 0;
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->snoop_by == dclose)
                d->snoop_by = NULL;
            if (d->connected == CON_PLAYING)
              mudstats.current_players +=1;
        }
    }


    if ((ch = dclose->character) != NULL)
    {
        mudstats.current_players -= 1;
        sprintf (log_buf, "Closing link to %s.", ch->name);
        log_string (log_buf);
        /* cut down on wiznet spam when rebooting */
        if (dclose->connected == CON_PLAYING && !merc_down)
        {
            act ("$n has lost $s link.", ch, NULL, NULL, TO_ROOM);
            wiznet ("Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0);

            if (IS_DISGUISED(ch))
                REM_DISGUISE(ch);
            ch->desc = NULL;
        }
        else
        {
            free_char (dclose->original ? dclose->original :
                       dclose->character);
        }
    }

    if (d_next == dclose)
        d_next = d_next->next;

    if (dclose == descriptor_list)
    {
        descriptor_list = descriptor_list->next;
    }
    else
    {
        static int dclose_count;
        DESCRIPTOR_DATA *d;

        for (d = descriptor_list; d && d->next != dclose; d = d->next);
        if (d != NULL)
            d->next = dclose->next;
        else
        {
            dclose_count++;
            bug ("Close_socket: dclose not found.", 0);
            if (dclose_count > 5)
            {
              bug("Close_socket: break check by asmo.", 0);
              return;
            }
	}
    }

#ifdef MCCP
    compressEnd(dclose);
#endif

    close (dclose->descriptor);
    free_descriptor (dclose);
#if defined(MSDOS) || defined(macintosh)
    exit (1);
#endif
    return;
}



bool read_from_descriptor (DESCRIPTOR_DATA * d)
{
    unsigned int iStart;

    /* Hold horses if pending command already. */
    if (d->incomm[0] != '\0')
        return TRUE;

    /* Check for overflow. */
    iStart = strlen (d->inbuf);
    if (iStart >= sizeof (d->inbuf) - 10)
    {
        sprintf (log_buf, "%s input overflow!", dns_gethostname(d->host));
        log_string (log_buf);
        write_to_descriptor (d,
                             "\n\r*** PUT A LID ON IT!!! ***\n\r", 0);
        return FALSE;
    }

    /* Snarf input. */
#if defined(macintosh)
    for (;;)
    {
        int c;
        c = getc (stdin);
        if (c == '\0' || c == EOF)
            break;
        putc (c, stdout);
        if (c == '\r')
            putc ('\n', stdout);
        d->inbuf[iStart++] = c;
        if (iStart > sizeof (d->inbuf) - 10)
            break;

    }
#endif

#if defined(MSDOS) || defined(unix)
    for (;;)
    {
        int nRead;

        nRead = read (d->descriptor, d->inbuf + iStart,
                      sizeof (d->inbuf) - 10 - iStart);
        if (nRead > 0)
        {
            iStart += nRead;
            if (d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r')
                break;
        }
        else if (nRead == 0)
        {
            log_string ("EOF encountered on read.");
            if (d->character)
              logf("%s", d->character->name);
            return FALSE;
        }
        else if (errno == EWOULDBLOCK)
            break;
        else
        {
            perror ("Read_from_descriptor");
            return FALSE;
        }
    }
#endif

    d->inbuf[iStart] = '\0';
    return TRUE;
}



/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer (DESCRIPTOR_DATA * d)
{
    int i, j, k;
#ifdef MCCP
//      int iac = 0;
#endif
    bool mxpCommand = FALSE;

    /*
     * Hold horses if pending command already.
     */
    if (d->incomm[0] != '\0')
        return;

    /*
     * Look for at least one new line.
     */
    for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
    {
        if (d->inbuf[i] == '\0')
            return;
    }

    /*
     * Canonical input processing.
     */
    for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
    {
        if (k >= MAX_INPUT_LENGTH - 2)
        {
            write_to_descriptor (d, "Line too long.\n\r", 0);

            /* skip the rest of the line */
            for (; d->inbuf[i] != '\0'; i++)
            {
                if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
                    break;
            }
            d->inbuf[i] = '\n';
            d->inbuf[i + 1] = '\0';
            break;
        }


        if ( d->inbuf[i] == '\b' && k > 0 )
            --k;
        else if ( d->inbuf[i] == '\e' )
        {
          i++;
          mxpCommand = TRUE;
        }
        else if ( isascii(d->inbuf[i]) && isprint(d->inbuf[i]) )
            d->incomm[k++] = d->inbuf[i];
        else if (d->inbuf[i] == (signed char)IAC)
        {
            if (!memcmp(&d->inbuf[i], mxp_do, strlen(mxp_do)))
            {
               char buf[MSL];
               i += strlen(mxp_do) - 1;
               d->mxp=TRUE;
               write_to_buffer(d, "\e[1z<VERSION>", 0);
               logf( "[%d] %s is using MXP",d->descriptor, dns_gethostname(d->host));
               sprintf( buf, "%s is using MXP", dns_gethostname(d->host));
               wiznet(buf,NULL,NULL,WIZ_LOGINS,0,0);
//	       logf("MXP DO", 0);
            }
            else if (!memcmp(&d->inbuf[i], mxp_dont, strlen(mxp_dont)))
            {
               i += strlen(mxp_dont) - 1;
               d->mxp=FALSE;
            }
            else if (!memcmp(&d->inbuf[i], compress_do, strlen(compress_do))) {
                char buf[MSL];
                i += strlen(compress_do) - 1;
                compressStart(d);
                sprintf(buf, "%s is using Compression", dns_gethostname(d->host));
                wiznet(buf, NULL, NULL, WIZ_LOGINS, 0, 0);
            }
            else if (!memcmp(&d->inbuf[i], compress_dont, strlen(compress_dont))) {
                i += strlen(compress_dont) - 1;
                compressEnd(d);
            }
         }
     }

    /*
     * Finish off the line.
     */
    if (k == 0)
        d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */

    if (k > 1 || d->incomm[0] == '!')
    {
        if (d->incomm[0] != '!' && strcmp (d->incomm, d->inlast))
        {
            d->repeat = 0;
        }
        else
        {
            if (++d->repeat >= 25 && d->character
                && d->connected == CON_PLAYING)
            {
                sprintf (log_buf, "%s input spamming!", dns_gethostname(d->host));
                log_string (log_buf);
                wiznet ("Spam spam spam $N spam spam spam spam spam!",
                        d->character, NULL, WIZ_SPAM, 0,
                        get_trust (d->character));
                if (d->incomm[0] == '!')
                    wiznet (d->inlast, d->character, NULL, WIZ_SPAM, 0,
                            get_trust (d->character));
                else
                    wiznet (d->incomm, d->character, NULL, WIZ_SPAM, 0,
                            get_trust (d->character));

                d->repeat = 0;
/*
        write_to_descriptor( d,
            "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
        strcpy( d->incomm, "quit" );
*/
            }
        }
    }

    if (mxpCommand)
    {
      interp_mxp(d->character, d->incomm);
      strcpy( d->incomm, "");
    }

    /*
     * Do '!' substitution.
     */
    if (d->incomm[0] == '!')
        strcpy (d->incomm, d->inlast);
    else
        strcpy (d->inlast, d->incomm);

    /*
     * Shift the input buffer.
     */
    while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
        i++;
    for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++);
    return;
}



/*
 * Low level output function.
 */
bool process_output (DESCRIPTOR_DATA * d, bool fPrompt)
{
    extern bool merc_down;

    /*
     * Bust a prompt.
     */
    if (!merc_down)
    {
        if (d->showstr_point)
            write_to_buffer (d, "\n\r[Hit Return to continue]\n\r", 0);
        else if (fPrompt && d->pString && d->connected == CON_PLAYING)
            write_to_buffer (d, "> ", 2);
        else if (fPrompt && d->connected == CON_PLAYING)
        {
            CHAR_DATA *ch;
            CHAR_DATA *victim;
            ch = d->character;

            /* battle prompt */
            if ( ((victim = ch->fighting) != NULL) && can_see (ch, victim))
            {
                int percent;
                char wound[100];
	        char *pbuff;
                char buf[MSL];
                char buffer[MSL*2];

                if (victim->max_hit > 0)
                    percent = victim->hit * 100 / victim->max_hit;
                else
                    percent = -1;

                if (percent >= 100)
                    sprintf (wound, "is in excellent condition.");
                else if (percent >= 90)
                    sprintf (wound, "has a few scratches.");
                else if (percent >= 75)
                    sprintf (wound, "has some small wounds and bruises.");
                else if (percent >= 50)
                    sprintf (wound, "has quite a few wounds.");
                else if (percent >= 30)
                    sprintf (wound,
                             "has some big nasty wounds and scratches.");
                else if (percent >= 15)
                    sprintf (wound, "looks pretty hurt.");
                else if (percent >= 0)
                    sprintf (wound, "is in awful condition.");
                else
                    sprintf (wound, "is bleeding to death.");

                        if( victim->stunned)
                        {
                                sprintf(buf,"`!%s is s`1tunne`!d`7.{x\n\r",
                                        IS_NPC(victim) ? victim->short_descr : PERS(victim, ch, FALSE));
                                send_to_char(buf, ch);
                        }

                sprintf (buf, "%s %s \n\r",
                         IS_NPC (victim) ? victim->short_descr : PERS(victim, ch, FALSE),
                         wound);
                buf[0] = UPPER (buf[0]);
				pbuff = buffer;
                colourconv (pbuff, buf, CH(d));
                write_to_buffer (d, buffer, 0);
            }


            ch = d->original ? d->original : d->character;
            if (!IS_SET (ch->comm, COMM_COMPACT))
                write_to_buffer (d, "\n\r", 2);

        /* battle prompts =) */
        if ((victim = ch->fighting) != NULL && can_see(ch,victim) && ch->pcdata->barOn)
        {
            int percent;
	    char meter[100];
	    char buf[MAX_STRING_LENGTH];

            if (victim->max_hit > 0)
                percent = victim->hit * 100 / victim->max_hit;
            else
                percent = -1;

            if (IS_IMMORTAL(ch) && IS_SET(ch->act2, PLR_MXP))
            {
              sprintf(buf, "\e[1z<EnmyHp>%ld</EnmyHp>\n\r", victim->hit);
              send_to_char(buf, ch);
            }

            if (percent >= 100)
                sprintf(meter,"[`^**********`*]");
            else if (percent >= 90)
                sprintf(meter,"[`M********* `*]");
            else if (percent >= 80)
                sprintf(meter,"[`#********  `*]");
            else if (percent >= 70)
                sprintf(meter,"[`@*******   `*]");
            else if (percent >= 60)
                sprintf(meter,"[`$******    `*]");
            else if (percent >= 50)
                sprintf(meter,"[`5*****     `*]");
            else if (percent >= 40)
                sprintf(meter,"[`3****      `*]");
            else if (percent >= 30)
                sprintf(meter,"[`2***       `*]");
            else if (percent >= 20)
                sprintf(meter,"[`4**        `*]");
            else if (percent >= 10)
                sprintf(meter,"[`!*         `*]");
            else if (percent >= 0)
                sprintf(meter,"[`*          `*]");
            sprintf(buf,"%s`*: %s`*     ",
	            IS_NPC(victim) ? victim->short_descr : PERS(victim, ch, FALSE),meter);
	    buf[0] = UPPER(buf[0]);
	    send_to_char(buf, ch);
	  /*if (victim->stunned)
	    {
		sprintf(buf,"{f%s is stunned.`*\n\r",
	            IS_NPC(victim) ? victim->short_descr : victim->name);
		send_to_char(buf, ch);
	    }*/
            if (victim->max_hit > 0)
                percent = ch->hit * 100 / ch->max_hit;
            else
                percent = -1;
            if (percent >= 100)
                sprintf(meter,"[`^**********`*]");
            else if (percent >= 90)
                sprintf(meter,"[`M********* `*]");
            else if (percent >= 80)
                sprintf(meter,"[`#********  `*]");
            else if (percent >= 70)
                sprintf(meter,"[`@*******   `*]");
            else if (percent >= 60)
                sprintf(meter,"[`$******    `*]");
            else if (percent >= 50)
                sprintf(meter,"[`5*****     `*]");
            else if (percent >= 40)
                sprintf(meter,"[`3****      `*]");
            else if (percent >= 30)
                sprintf(meter,"[`2***       `*]");
            else if (percent >= 20)
                sprintf(meter,"[`4**        `*]");
            else if (percent >= 10)
                sprintf(meter,"[`!*         `*]");
            else if (percent >= 0)
                sprintf(meter,"[`*          `*]");
            sprintf(buf,"You: %s\n\r",meter);
            buf[0] = UPPER(buf[0]);
            send_to_char(buf, ch);
/*            write_to_buffer( d, buf, 0); */

        }



            if (IS_SET (ch->comm, COMM_PROMPT))
                bust_a_prompt (d->character);

            if (IS_SET (ch->comm, COMM_TELNET_GA))
                write_to_buffer (d, go_ahead_str, 0);
        }
    }

    /*
     * Short-circuit if nothing to write.
     */
    if (d->outtop == 0)
        return TRUE;

    /*
     * Snoop-o-rama.
     */
    if (d->snoop_by != NULL)
    {
        if (d->character != NULL)
            write_to_buffer (d->snoop_by, d->character->name, 0);
        write_to_buffer (d->snoop_by, "> ", 2);
        write_to_buffer (d->snoop_by, d->outbuf, d->outtop);
    }

    /*
     * OS-dependent output.
     */
    if (!write_to_descriptor (d, d->outbuf, d->outtop))
    {
        d->outtop = 0;
        return FALSE;
    }
    else
    {
        d->outtop = 0;
        return TRUE;
    }
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt (CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    const char *str;
    const char *i;
    char *point;
    char *pbuff;
    char buffer[MAX_STRING_LENGTH * 2];
//    char doors[MAX_INPUT_LENGTH];
//    EXIT_DATA *pexit;
//    bool found;
//    const char *dir_name[] = { "N", "E", "S", "W", "U", "D" };
//    int door;

    point = buf;
    str = ch->prompt;
    if (str == NULL || str[0] == '\0')
    {
        sprintf (buf, "`@<`6%ld`^hp `6%d`^m `6%d`^mv`@>`*",
                 ch->hit, ch->mana, ch->move);
	send_to_char(buf,ch);
	sprintf( buf, "%s",
		ch->prefix);
        send_to_char (buf, ch);
        return;
    }

  if (ch->desc && ch->desc->editor != ED_NONE )
   {
     AREA_DATA *pArea;
     ROOM_INDEX_DATA *pRoom;
     RIDDLE_DATA *pRiddle;
     OBJ_INDEX_DATA *pObj;
     MOB_INDEX_DATA *pMob;
     PROG_CODE *pMprog;
     PROG_CODE *pRprog;
     PROG_CODE *pOprog;
     HELP_DATA *pHelp;
     CLAN_DATA *pGuild;
     QPRIZE_DATA *pQuest;

     char buf1[MAX_STRING_LENGTH];

     buf1[0] = '\0';

     switch ( ch->desc->editor )
     {
	case ED_AREA:
	  EDIT_AREA(ch, pArea);
	  sprintf( buf1, "`!<`2Editing AREA: `&%ld`!>`* ",
		pArea ? pArea->vnum : 0 );
	  break;
	case ED_ROOM:
	  EDIT_ROOM(ch, pRoom);
	  sprintf( buf1, "`!<`2Editing Room: `&%ld`!>`* ",
		pRoom ? pRoom->vnum : 0 );
	  break;
	case ED_OBJECT:
	  EDIT_OBJ(ch, pObj);
	  sprintf( buf1, "`!<`2Editing Obj: `&%ld`!>`* ",
		pObj ? pObj->vnum :  0);
	  break;
	case ED_MOBILE:
	  EDIT_MOB(ch, pMob);
	  sprintf( buf1, "`!<`2Editing Mob: `&%ld`!>`* ",
		pMob ? pMob->vnum : 0);
	  break;

	case ED_MPCODE:
	  EDIT_MPCODE(ch, pMprog);
	  sprintf( buf1, "`!<`2Editing MProg: `&%ld`!>`* ",
		pMprog ? pMprog->vnum : 0);
	  break;

	case ED_RPCODE:
	  EDIT_RPCODE(ch, pRprog);
	  sprintf( buf1, "`!<`2Editing RProg: `&%ld`!>`* ",
		pRprog ? pRprog->vnum : 0);
	  break;

	case ED_OPCODE:
	  EDIT_OPCODE(ch, pOprog);
	  sprintf( buf1, "`!<`2Editing OProg: `&%ld`!>`* ",
		pOprog ? pOprog->vnum : 0);
	  break;

	case ED_GUILD:
	  EDIT_GUILD(ch, pGuild);
	  sprintf( buf1, "`!<`2Editing Guild: `&%s`!>`* ",
		pGuild ? pGuild->name : "None");
	  break;

	case ED_QUEST:
	  EDIT_QUEST(ch, pQuest);
	  sprintf( buf1, "`!<`2Editing QPRIZE: `&%s`!>`* ",
		pQuest ? pQuest->keyword : "None");
	  break;
        case ED_HELP:
	  pHelp = (HELP_DATA *) ch->desc->pEdit;
	  sprintf( buf1, "`!<`2Editing Help: `&%s`!>`* ",
		    pHelp ? pHelp->keyword : "None" );
	  break;
	case ED_STORE:
	  sprintf( buf1, "`!<`2Editing Store`!>`* ");
	  break;
	case ED_RIDDLE:
          EDIT_RIDDLE(ch, pRiddle);
	  sprintf( buf1, "`!<`2Editing Riddle: %d`!>`* ", pRiddle ? pRiddle->vnum : 0);
	  break;

     }

     send_to_char( buf1, ch);
     return;
   }


    while (*str != '\0')
    {
        if (*str != '%')
        {
            *point++ = *str++;
            continue;
        }
        ++str;
        switch (*str)
        {
            default:
                i = " ";
                break;

/*
            case 'e':
                found = FALSE;
                doors[0] = '\0';
                for (door = 0; door < 6; door++)
                {
                    if ((pexit = ch->in_room->exit[door]) != NULL
                        && pexit->u1.to_room != NULL
                        && (can_see_room (ch, pexit->u1.to_room)
                            || (IS_AFFECTED (ch, AFF_INFRARED)
                                && !IS_AFFECTED (ch, AFF_BLIND)))
                        && !IS_SET (pexit->exit_info, EX_CLOSED))
                    {
                        found = TRUE;
                        strcat (doors, dir_name[door]);
                    }
                }
                if (!found)
                    strcat (buf, "none");
                sprintf (buf2, "%s", doors);
                i = buf2;
                break;
*/
            case 'c':
                sprintf (buf2, "%s", "\n\r");
                i = buf2;
                break;

	    case 'd':
                if (IS_NPC(ch) || IS_SWITCHED(ch))
                {
                  sprintf(buf2, " ");
                  i = buf2;
                  break;
                }
                sprintf (buf2, "%d", ch->pcdata->damInflicted);
                i = buf2;
                break;

	    case 'D':
                if (IS_NPC(ch) || IS_SWITCHED(ch))
                {
                  sprintf(buf2, " ");
                  i = buf2;
                  break;
                }
                sprintf (buf2, "%d", ch->pcdata->damReceived);
                i = buf2;
                break;


            case 'h':
                if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_MXP))
                  sprintf(buf2, "<Hp>%ld</Hp>", ch->hit);
                else
                  sprintf (buf2, "%ld", ch->hit);
                i = buf2;
                break;
            case 'H':
                if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_MXP))
                  sprintf(buf2, "<MaxHp>%ld</MaxHp>", ch->max_hit);
                else
                  sprintf (buf2, "%ld", ch->max_hit);
                i = buf2;
                break;
            case 'm':
                if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_MXP))
                  sprintf(buf2, "<Mana>%d</Mana>", ch->mana);
                else
                sprintf (buf2, "%d", ch->mana);
                i = buf2;
                break;
            case 'M':
                if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_MXP))
                  sprintf(buf2, "<MaxMana>%d</MaxMana>", ch->max_mana);
                else
                sprintf (buf2, "%d", ch->max_mana);
                i = buf2;
                break;
            case 'v':
                sprintf (buf2, "%d", ch->move);
                i = buf2;
                break;
            case 'V':
                sprintf (buf2, "%d", ch->max_move);
                i = buf2;
                break;
            case 'x':
                sprintf (buf2, "%d", ch->exp);
                i = buf2;
                break;
            case 'X':
                sprintf (buf2, "%d", IS_NPC (ch) ? 0 :
                         (ch->level + 1) * exp_per_level (ch,
                                                          ch->pcdata->
                                                          points) - ch->exp);
                i = buf2;
                break;
            case 'g':
                sprintf (buf2, "%ld", ch->gold);
                i = buf2;
                break;
            case 's':
                sprintf (buf2, "%ld", ch->silver);
                i = buf2;
                break;
            case 'a':
                if (ch->level > 9)
                    sprintf (buf2, "%d", ch->alignment);
                else
                    sprintf (buf2, "%s",
                             IS_GOOD (ch) ? "good" : IS_EVIL (ch) ? "evil" :
                             "neutral");
                i = buf2;
                break;
            case 'r':
                if (ch->in_room != NULL)
                    sprintf (buf2, "%s",
                             ((!IS_NPC
                               (ch) && IS_SET (ch->act, PLR_HOLYLIGHT))
                              || (!IS_AFFECTED (ch, AFF_BLIND)
                                  && !room_is_dark (ch->
                                                    in_room))) ? ch->in_room->
                             name : "darkness");
                else
                    sprintf (buf2, " ");
                i = buf2;
                break;
            case 'R':
                if (IS_IMMORTAL (ch) && ch->in_room != NULL)
                    sprintf (buf2, "%ld", ch->in_room->vnum);
                else
                    sprintf (buf2, " ");
                i = buf2;
                break;
            case 'Q':
                if (IS_NPC(ch) || IS_SWITCHED(ch))
                {
                  sprintf(buf2, " ");
                  i = buf2;
                  break;
                }
                sprintf (buf2, "%ld", ch->questpoints);
		i = buf2;
		break;
            case 'q':
                if (IS_NPC(ch) || IS_SWITCHED(ch))
                {
                  sprintf(buf2, " ");
                  i = buf2;
                  break;
                }
		if (IS_SET(ch->act, PLR_QUESTOR))
                {
                  if (ch->countdown > 0)
                  {
                     sprintf(buf2, "`#%d`*", ch->countdown);
                     i = buf2;
                     break;
                  }
                  else
                  {
                     sprintf(buf2, "%d", ch->nextquest);
                     i = buf2;
                     break;
                  }
                }
                else
                {
                   sprintf(buf2, "%d", ch->nextquest);
                   i = buf2;
                   break;
                }
                break;
            case 't':
                if (IS_NPC(ch) || IS_SWITCHED(ch))
                {
                  sprintf(buf2, " ");
                  i = buf2;
                  break;
                }
                else
                {
                  sprintf(buf2, "%d",
		   IS_MURDERER(ch) ? ch->penalty.murder :
                   IS_THIEF(ch) ? ch->penalty.thief : 0);
                  i = buf2;
                  break;
                }
                break;
            case 'T':
                if (IS_NPC(ch) || IS_SWITCHED(ch))
                {
                  sprintf(buf2, " ");
                  i = buf2;
                  break;
                }
                if (IS_SET(ch->act2, PLR_MXP))
                {
                  if (ch->timed_affect.seconds > 0)
                    sprintf (buf2, "`#<Ticks>%d</Ticks>`*", ch->timed_affect.seconds);
                  else if (ch->pcdata->pk_timer > 0)
                    sprintf (buf2, "`!<Ticks>%d</Ticks>`*", ch->pcdata->pk_timer);
                  else if (ch->pcdata->safe_timer > 0)
                    sprintf (buf2, "`$<Ticks>%d</Ticks>`*", ch->pcdata->safe_timer);
                  else if (ch->pcdata->wait_timer > 0)
                    sprintf (buf2, "`&<Ticks>%d</Ticks>`*", ch->pcdata->wait_timer);
                  else
                    sprintf(buf2, " ");
                }
		else
		{
                  if (ch->timed_affect.seconds > 0)
                    sprintf (buf2, "`#%d`*", ch->timed_affect.seconds);
                  else if (ch->pcdata->pk_timer > 0)
                    sprintf (buf2, "`!%d`*", ch->pcdata->pk_timer);
                  else if (ch->pcdata->safe_timer > 0)
                    sprintf (buf2, "`$%d`*", ch->pcdata->safe_timer);
                  else if (ch->pcdata->wait_timer > 0)
                    sprintf (buf2, "`&%d`*", ch->pcdata->wait_timer);
                  else
                    sprintf(buf2, " ");
		}
                i = buf2;
                break;
            case 'z':
                if (IS_IMMORTAL (ch) && ch->in_room != NULL)
                    sprintf (buf2, "%s", ch->in_room->area->name);
                else
                    sprintf (buf2, " ");
                i = buf2;
                break;
            case '%':
                sprintf (buf2, "%%");
                i = buf2;
                break;
            case 'o':
                sprintf (buf2, "%s", olc_ed_name (ch));
                i = buf2;
                break;
            case 'O':
                sprintf (buf2, "%s", olc_ed_vnum (ch));
                i = buf2;
                break;
            case 'P':
                sprintf (buf2, "%d",ch->rpexp);
                i = buf2;
                break;
        }
        ++str;
        while ((*point = *i) != '\0')
            ++point, ++i;
    }
    if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_MXP))
      send_to_char("\e[1z<Prompt>", ch);
    *point = '\0';
    pbuff = buffer;
    colourconv (pbuff, buf, ch);
    send_to_char ("`*", ch);
    write_to_buffer (ch->desc, buffer, 0);
    if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_MXP))
      send_to_char("</Prompt>", ch);
    if (IS_SET (ch->comm, COMM_AFK))
    {
        send_to_char ("`8<`!A`&F`!K`8>`* ", ch);
    }

    if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_NORP))
    {
        send_to_char ("`8(`1NoRP`8)`* ", ch);
    }

    if (IS_AFFECTED(ch, AFF_INVISIBLE))
     send_to_char("`4(I)`*", ch);
    if (IS_AFFECTED(ch, AFF_STANCE))
     send_to_char("(`&S`7)`*", ch);
    if (IS_AFFECTED(ch, AFF_HIDE))
     send_to_char("`!(H)`*", ch);
    if (IS_AFFECTED(ch, AFF_DIEMCLOAK))
     send_to_char("`!(`8AssMaster`!)`*", ch);
    if (IS_AFFECTED(ch, AFF_SHROUD))
     send_to_char("`8(S)`*", ch);
    if (IS_AFFECTED(ch, AFF_VEIL))
    send_to_char("`5(`%V`5)`*", ch);
    if (ch->invis_level >= LEVEL_HERO)
     send_to_char("`&(`2W`&)`*", ch);
    if (ch->incog_level >= 1)
     send_to_char("`&(`6I`&)`*", ch);
    if (IS_AFFECTED(ch, AFF_SNEAK) || IS_AFFECTED(ch, AFF_STEALTH))
     send_to_char("`5(S)`*", ch);
   if (!IS_NPC(ch))
   {
     if (IS_SET(ch->act, PLR_HUNGER_FLAG) &&
         ch->pcdata->condition[COND_HUNGER] == 0)
       send_to_char("`3(H)", ch);
      if (IS_SET(ch->act, PLR_HUNGER_FLAG) &&
         ch->pcdata->condition[COND_THIRST] == 0)
        send_to_char("`6(T)", ch);
    }


    send_to_char ("`*\n\r", ch);

    if (ch->prefix[0] != '\0')
        write_to_buffer (ch->desc, ch->prefix, 0);
    return;
}



/*
 * Append onto an output buffer.
 */
void write_to_buffer (DESCRIPTOR_DATA * d, const char *txt, int length)
{
    /*
     * Find length in case caller didn't.
     */
    if (length <= 0)
        length = strlen (txt);

    /*
     * Initial \n\r if needed.
     */
    if (d->outtop == 0 && !d->fcommand)
    {
        d->outbuf[0] = '\n';
        d->outbuf[1] = '\r';
        d->outtop = 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while (d->outtop + length >= d->outsize)
    {
        char *outbuf;

        if (d->outsize >= 32000)
        {
            bug ("Buffer overflow. Closing.\n\r", 0);
            close_socket (d);
            return;
        }
        outbuf = (char *) alloc_mem (2 * d->outsize);
        strncpy (outbuf, d->outbuf, d->outtop);
        free_mem (d->outbuf, d->outsize);
        d->outbuf = outbuf;
        d->outsize *= 2;
    }

    /*
     * Copy.
     */
    strncpy (d->outbuf + d->outtop, txt, length);
    d->outtop += length;
    return;
}

/*  Trying another version

#ifdef MCCP
#define COMPRESS_BUF_SIZE 1024
bool write_to_descriptor( DESCRIPTOR_DATA *d, char *txt, int length )
{
    int     iStart = 0;
    int     nWrite = 0;
    int     nBlock;
    int     len;

    if (length <= 0)
        length = strlen(txt);

    if (d && d->out_compress)
    {
        d->out_compress->next_in = (unsigned char *)txt;
        d->out_compress->avail_in = length;
        while (d->out_compress->avail_in)
        {
            d->out_compress->avail_out = COMPRESS_BUF_SIZE - (d->out_compress->next_out - d->out_compress_buf);

            if (d->out_compress->avail_out)
            {
                int status = deflate(d->out_compress, Z_SYNC_FLUSH);

                if (status != Z_OK)
                  return FALSE;
            }

            len = d->out_compress->next_out - d->out_compress_buf;
            if (len > 0)
            {
                for (iStart = 0; iStart < len; iStart += nWrite)
                {
                    nBlock = UMIN (len - iStart, 4096);
                    if ((nWrite = write(d->descriptor, d->out_compress_buf + iStart, nBlock)) < 0)
                    {
                        perror( "Write_to_descriptor: compressed" );
                        return FALSE;
                    }

                    if (!nWrite)
                        break;
                }

                if (!iStart)
                    break;

                if (iStart < len)
                    memmove(d->out_compress_buf, d->out_compress_buf+iStart, len - iStart);

                d->out_compress->next_out = d->out_compress_buf + len - iStart;
            }
        }
        return TRUE;
    }
    for (iStart = 0; iStart < length; iStart += nWrite)
    {
        nBlock = UMIN (length - iStart, 4096);
        if ((nWrite = write(d->descriptor, txt + iStart, nBlock)) < 0)
        {
            perror( "Write_to_descriptor" );
            return FALSE;
        }
    }

    return TRUE;
}

#else

*/

#ifdef MCCP
bool write_to_descriptor_2( int desc, char *txt, int length )
{
    int iStart;
    int nWrite;
    int nBlock;

#if defined(macintosh) || defined(MSDOS)
    if ( desc == 0 )
        desc = 1;
#endif

    if ( length <= 0 )
        length = strlen(txt);

    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
        nBlock = UMIN( length - iStart, 4096 );
        if ( ( nWrite = write( desc, txt + iStart, nBlock ) ) < 0 )
            { perror( "Write_to_descriptor" ); return FALSE; }
    }

    return TRUE;
}

bool write_to_descriptor(DESCRIPTOR_DATA *d, char *txt, int length)
{
    if (d->out_compress)
        return writeCompressed(d, txt, length);
    else
        return write_to_descriptor_2(d->descriptor, txt, length);
}


#else

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor (DESCRIPTOR_DATA *d, char *txt, int length)
{
    int iStart;
    int nWrite;
    int nBlock;

#if defined(macintosh) || defined(MSDOS)
    if (desc == 0)
        desc = 1;
#endif

    if (length <= 0)
        length = strlen (txt);

    for (iStart = 0; iStart < length; iStart += nWrite)
    {
        nBlock = UMIN (length - iStart, 4096);
        if ((nWrite = write (d->descriptor, txt + iStart, nBlock)) < 0)
        {
            perror ("Write_to_descriptor");
            return FALSE;
        }
    }
    return TRUE;

}

#endif


/* void logf (char *fmt, ...)
{
    char buf[2 * MSL];
    va_list args;
    va_start (args, fmt);
    vsprintf (buf, fmt, args);
    va_end (args);

    log_string (buf);
}

*/

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny (DESCRIPTOR_DATA * d, char *argument)
{
    DESCRIPTOR_DATA *d_old, *d_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    int iClass, race, i, weapon;
    bool fOld;

    while (isspace (*argument))
        argument++;

    ch = d->character;

    switch (d->connected)
    {

        default:
            bug ("Nanny: bad d->connected %d.", d->connected);
            close_socket (d);
            return;

        #ifdef DNS_SLAVE
          case CON_GETDNS:
            return;
        #endif

        case CON_GET_NAME:

            if (argument[0] == '\0')
            {
                close_socket (d);
                return;
            }

            argument[0] = UPPER (argument[0]);

            fOld = load_char_obj (d, argument);
            ch = d->character;
            if (ch->level > 81)
               ch->level -= 5;
/*            if (IS_SET  ( ch->act, PLR_TOURNEY) )
              REMOVE_BIT(ch->act, PLR_TOURNEY);
            if (IS_SET  ( ch->act2, PLR_TOURNAMENT_START) )
              REMOVE_BIT(ch->act2, PLR_TOURNAMENT_START);
*/

            if (IS_SET (ch->act, PLR_DENY))
            {
                sprintf (log_buf, "Denying access to %s@%s.", argument,
                         dns_gethostname(d->host));
                log_string (log_buf);
                send_to_desc ("You are denied access.\n\r", d);
                close_socket (d);
                return;
            }

            if (check_ban (dns_gethostname(d->host), BAN_PERMIT)
                && !IS_SET (ch->act, PLR_PERMIT))
            {
                send_to_desc ("Your site has been banned from this mud.\n\r",
                              d);
                close_socket (d);
                return;
            }

            if (check_reconnect (d, argument, FALSE))
            {
                fOld = TRUE;
            }
            else
            {
                if (wizlock && !IS_IMMORTAL (ch))
                {
                    send_to_desc ("The game is wizlocked.\n\r", d);
                    close_socket (d);
                    return;
                }
            }

            if (fOld)
            {
                /* Old player */
                send_to_desc ("Password: ", d);
                write_to_buffer (d, echo_off_str, 0);
                d->connected = CON_GET_OLD_PASSWORD;
                return;
            }
            else
            {
	       if (!str_cmp(argument, "account"))
	       {
	       	 send_to_desc("`$Entering account mode.`*\n\r", d);
                 send_to_desc("`&Please enter account name:`* ", d);
	       	 d->connected = CON_GET_ACCOUNT_NAME;
	       	 break;
	       }

               if (strcmp(argument, "New") && strcmp(argument, "Newchar"))
               {
		 send_to_desc("Name not found, please retype name or type newchar to\n\r"
			      "create a new character.\n\r"
			      "Login:", d);
		 d->connected = CON_GET_NAME;
		 break;
	       }


                /* New player */
               if (newlock)
               {
                   send_to_desc ("The game is newlocked.\n\r", d);
                   close_socket (d);
                   return;
               }

               if (check_ban (dns_gethostname(d->host), BAN_NEWBIES))
               {
                   send_to_desc
                       ("New players are not allowed from your site.\n\r",
                        d);
                   close_socket (d);
                   return;
               }
               d->connected = CON_GET_NEW_NAME;
	       {
		 extern char *help_name;
                 if (help_name[0] == '.')
                   send_to_desc (help_name + 1, d);
                 else
                   send_to_desc (help_name, d);
	       }

             }
             break;
        case CON_GET_ACCOUNT_NAME:
             get_account_name(d, argument);
             break;

        case CON_GET_ACCOUNT_PASSWORD:
             get_account_password(d, argument);
             break;

        case CON_SHOW_ACCOUNT:
             show_account(d, argument);
             break;

        case CON_ACCOUNT_ACTION:
             get_account_action(d, argument);
             break;

        case CON_GET_NEW_ACCOUNT_PASSWORD:
             get_new_account_password(d, argument);
             break;

        case CON_CONFIRM_NEW_ACCOUNT:
             confirm_new_account(d, argument);
             break;

        case CON_CONFIRM_NEW_ACCOUNT_PASSWORD:
             confirm_new_account_password(d, argument);
             break;

        case CON_CHECK_ATTACH_PASSWORD:
             check_attach_password(d, argument);
             break;

        case CON_GET_NEW_NAME:
	     {

               argument[0] = UPPER (argument[0]);

               if (!str_cmp(argument, "male"))
               {
                 gen_name(d, "male");
		 send_to_desc("Name or command: ",d );
		 return;
               }
               else if (!str_cmp(argument, "female"))
	       {
		 gen_name(d, "female");
		 send_to_desc("Name or command: ",d );
		 return;
	       }
	       else if (!check_parse_name (argument))
               {
                 send_to_desc ("Illegal name, try another.\n\rName: ", d);
                 return;
               }

               fOld = load_char_obj (d, argument);
   	       if (fOld)
	       {
		 send_to_desc("That name already exists choose a new name.\n\r", d);
		 send_to_desc("Name or command: ", d);
		 free_char(d->character);
	         return;
	       }
                sprintf (buf, "Did I get that right, %s (Y/N)? ", argument);
                send_to_desc (buf, d);
                d->connected = CON_CONFIRM_NEW_NAME;
                return;
            }
            break;


        case CON_GET_OLD_PASSWORD:
#if defined(unix)
            write_to_buffer (d, "\n\r", 2);
#endif

            if (strcmp (crypt (argument, ch->pcdata->pwd), ch->pcdata->pwd))
            {
                bugf("%s booted for wrong password", ch->name);
                send_to_desc ("Wrong password.\n\r", d);
                close_socket (d);
                return;
            }

            write_to_buffer (d, echo_on_str, 0);

            if (check_playing (d, IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name))
                return;

            if (check_reconnect (d, IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name, TRUE))
                return;


            mudstats.current_players += 1;
            mudstats.current_logins += 1;
            mudstats.total_logins +=1;
            if (mudstats.current_players > mudstats.current_max_players)
            {
               mudstats.current_max_players = mudstats.current_players;
               if (mudstats.total_max_players < mudstats.current_max_players)
                 mudstats.total_max_players = mudstats.current_max_players;
            }


            sprintf (log_buf, "%s@%s has connected.", ch->name, dns_gethostname(d->host));
            log_string (log_buf);

            if (IS_SET(ch->act2, PLR_NOIP))
              sprintf (log_buf, "%s has connected.", ch->name);
            wiznet (log_buf, NULL, NULL, WIZ_SITES, 0, get_trust (ch));

    /*        if (ch->desc->ansi)
                SET_BIT (ch->act, PLR_COLOUR);
            else
                REMOVE_BIT (ch->act, PLR_COLOUR);
*/
            ch->pcdata->Host = str_dup(dns_gethostname(ch->desc->host));
            if (IS_IMMORTAL (ch))
            {

                sprintf( buf, "\n\r`!Welcome `@%s`7@`8%s`7!\n\r", ch->name, dns_gethostname(d->host));
                send_to_char( buf, ch );
                sprintf( buf, "\n\r`&Please be advised `@%s`& that upon enterance, you may be logged`7\n\r", ch->name);
                send_to_char( buf, ch );
                do_function (ch, &do_mhelp, "* imotd");
                d->connected = CON_READ_IMOTD;
                d->connected = OPTION_UPDATE1;
            }
            else
            {
                sprintf( buf, "\n\rWelcome %s@%s!\n\r", ch->name, dns_gethostname(d->host));
                send_to_char( buf, ch );
		do_function (ch, &do_mhelp, "* 'intro note'");
                d->connected = CON_READ_MOTD;
                d->connected = OPTION_UPDATE1;
          }


            if (voteinfo.on)
            {
              send_to_char("\n\r`&There is currently a vote running.  Type`*\n\r", ch);
              send_to_char("`&vote show to see it or vote for syntax.`*\n\r", ch);
            }
            break;





/* RT code for breaking link */

        case CON_BREAK_CONNECT:
            switch (*argument)
            {
                case 'y':
                case 'Y':
                    for (d_old = descriptor_list; d_old != NULL;
                         d_old = d_next)
                    {
                        d_next = d_old->next;
                        if (d_old == d || d_old->character == NULL)
                            continue;

                        if (str_cmp (IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name,
                        d_old->original
                        ? IS_DISGUISED(d_old->original)?d_old->original->pcdata->disguise.orig_name:d_old->original->name
                        : IS_DISGUISED(d_old->character)?d_old->character->pcdata->disguise.orig_name:d_old->character->name))
                            continue;

                        close_socket (d_old);
                    }
                    if (check_reconnect (d, IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name, TRUE))
                        return;
                    send_to_desc ("Reconnect attempt failed.\n\rName: ", d);
                    if (d->character != NULL)
                    {
                        free_char (d->character);
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                case 'n':
                case 'N':
                    send_to_desc ("Name: ", d);
                    if (d->character != NULL)
                    {
                        free_char (d->character);
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    send_to_desc ("Please type Y or N? ", d);
                    break;
            }
            break;

        case CON_CONFIRM_NEW_NAME:
            switch (*argument)
            {
                case 'y':
                case 'Y':
                    sprintf (buf,
                             "New character.\n\rGive me a password for %s: %s",
                             ch->name, echo_off_str);
                    send_to_desc (buf, d);
                    d->connected = CON_GET_NEW_PASSWORD;
                    if (ch->desc->ansi)
                        SET_BIT (ch->act, PLR_COLOUR);
                    break;

                case 'n':
                case 'N':
        	   {
		      extern char *help_name;
                      if (help_name[0] == '.')
                        send_to_desc (help_name + 1, d);
                      else
                        send_to_desc (help_name, d);
	            }

                    send_to_desc ("Ok, what IS it, then? ", d);
                    free_char (d->character);
                    d->character = NULL;
                    d->connected = CON_GET_NEW_NAME;
                    break;

                default:
                    send_to_desc ("Please type Yes or No? ", d);
                    break;
            }
            break;

        case CON_GET_NEW_PASSWORD:
#if defined(unix)
            write_to_buffer (d, "\n\r", 2);
#endif

            if (strlen (argument) < 5)
            {
                send_to_desc
                    ("Password must be at least five characters long.\n\rPassword: ",
                     d);
                return;
            }

            pwdnew = crypt (argument, ch->name);
            for (p = pwdnew; *p != '\0'; p++)
            {
                if (*p == '~')
                {
                    send_to_desc
                        ("New password not acceptable, try again.\n\rPassword: ",
                         d);
                    return;
                }
            }

            free_string (ch->pcdata->pwd);
            ch->pcdata->pwd = str_dup (pwdnew);
            send_to_desc ("Please retype password: ", d);
            d->connected = CON_CONFIRM_NEW_PASSWORD;
            break;

        case CON_CONFIRM_NEW_PASSWORD:
#if defined(unix)
            write_to_buffer (d, "\n\r", 2);
#endif

            if (strcmp (crypt (argument, ch->pcdata->pwd), ch->pcdata->pwd))
            {
                send_to_desc ("Passwords don't match.\n\rRetype password: ",
                              d);
                d->connected = CON_GET_NEW_PASSWORD;
                return;
            }


            write_to_buffer (d, echo_on_str, 0);
                        send_to_desc ("Please select your class:\n\r", d);
            send_to_desc ("\n\r", d);

            send_to_desc ("--------------------------------------------------------\n\r",d);
            send_to_desc ("|                                                      |\n\r",d);
            send_to_desc ("| Archer      - Expert in ranged, weak with magic      |\n\r",d);
            send_to_desc ("| Assassin    - Expert in stealth and killing          |\n\r",d);
            send_to_desc ("| Cleric      - Expert in healing and protection       |\n\r",d);
            send_to_desc ("| Form Master - Expert in hand to hand combat          |\n\r",d);
            send_to_desc ("| Mage        - Expert in channeling, weak fighting    |\n\r",d);
            send_to_desc ("| Rogue       - Expert in stealth, deception           |\n\r",d);
            send_to_desc ("| Warrior     - Expert in melee, weak with magic       |\n\r", d);
            send_to_desc ("| Dragon      - Specialty class Application Only       |\n\r", d);
            send_to_desc ("| Forsaken    - Specialty class Application Only       |\n\r", d);
            send_to_desc ("|                                                      |\n\r", d);
            send_to_desc ("--------------------------------------------------------\n\r", d);
            send_to_desc ("\n\r", d);
            send_to_desc ("Which class?\n\r", d);
            d->connected = CON_GET_NEW_CLASS;
            break;

        case CON_GET_NEW_RACE:
            one_argument (argument, arg);

            if (!strcmp (arg, "help"))
            {
                argument = one_argument (argument, arg);
                if (argument[0] == '\0')
                    do_function (ch, &do_mhelp, "* 'race help'");
                else
                    do_function (ch, &do_mhelp, argument);
                send_to_desc
                    ("What is your race (help for more information)? ", d);
                break;
            }

            race = race_lookup (argument);

            if (race == 0 || !race_table[race].pc_race)
            {
                send_to_desc ("That is not a valid race.\n\r", d);
                send_to_desc ("The following races are available:\n\r  ", d);
                for (race = 1; race_table[race].name != NULL; race++)
                {
                    if (!race_table[race].pc_race)
                        break;
                    write_to_buffer (d, race_table[race].name, 0);
                    write_to_buffer (d, " ", 1);
                }
                write_to_buffer (d, "\n\r", 0);
                send_to_desc
                    ("What is your race? (help for more information) ", d);
                break;
            }

            ch->race = race;
            /* initialize stats */
            for (i = 0; i < MAX_STATS; i++)
                ch->perm_stat[i] += pc_race_table[race].modstats[i];

	    for (i = 0; i < MAX_STATS; i++)
                ch->perm_stat[i] = URANGE(18, ch->perm_stat[i], 25);
// UBit            ch->affected_by = ch->affected_by | race_table[race].aff;
//            STR_OR_STR( ch->affected_by, race_table[race].aff, AFF_FLAGS);

            ch->imm_flags = ch->imm_flags | race_table[race].imm;
            ch->res_flags = ch->res_flags | race_table[race].res;
            ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
            ch->form = race_table[race].form;
            ch->parts = race_table[race].parts;

            /* add skills */
            for (i = 0; i < 5; i++)
            {
                if (pc_race_table[race].skills[i] == NULL)
                    break;
                group_add (ch, pc_race_table[race].skills[i], FALSE);
            }
            /* add cost */
            ch->pcdata->points = pc_race_table[race].points;
            ch->size = pc_race_table[race].size;

            send_to_desc ("What is your sex (M/F)? ", d);
            d->connected = CON_GET_NEW_SEX;
            break;


        case CON_GET_NEW_SEX:
            switch (argument[0])
            {
                case 'm':
                case 'M':
                    ch->sex = SEX_MALE;
                    ch->pcdata->true_sex = SEX_MALE;
                    break;
                case 'f':
                case 'F':
                    ch->sex = SEX_FEMALE;
                    ch->pcdata->true_sex = SEX_FEMALE;
                    break;
                default:
                    send_to_desc ("That is not a sex.\n\rWhat IS your sex?",d);
                    return;
            }

            send_to_desc ("You may be good, neutral, or evil.\n\r", d);
            send_to_desc ("Which alignment (G/N/E)? ", d);
            d->connected = CON_GET_ALIGNMENT;
            break;

        case CON_GET_NEW_CLASS:
            iClass = class_lookup (argument);

            if (iClass == -1 || iClass == class_lookup("Illuminator"))
            {
                send_to_desc ("That is not a class.\n\rWhat IS your class? ",
                              d);
                return;
            }

            if (iClass == MAX_CLASS - 3)
            {
              send_to_desc("Enter password:\n\r", d);
              d->connected = CON_GET_FORSAKEN_PASSWORD;
              ch->cClass = iClass;
              break;
            }

            if (iClass == MAX_CLASS - 2)
            {
              send_to_desc("Enter password:\n\r", d);
              d->connected = CON_GET_DRAGON_PASSWORD;
              ch->cClass = iClass;
              break;
            }

            ch->cClass = iClass;
            for (i = 0; i < MAX_STATS; i++)
                ch->perm_stat[i] = class_table[iClass].modstat[i];

            sprintf (log_buf, "%s@%s new player.", ch->name, dns_gethostname(d->host));
            log_string (log_buf);
            wiznet ("Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
            wiznet (log_buf, NULL, NULL, WIZ_SITES, 0, get_trust (ch));

            send_to_desc("\n\r", d);
            print_class_stats(ch, ch->cClass);
            send_to_desc("Choose your homeland: ", d);
            d->connected = CON_GET_NEW_RACE;
            break;

        case CON_GET_FORSAKEN_PASSWORD:
            if (!str_cmp(argument, "impsonlypass"))
            {
               int sn, i;

               ch->alignment = -750;
               group_add (ch, "rom basics", FALSE);
               group_add (ch, class_table[ch->cClass].base_group, FALSE);
               group_add (ch, class_table[ch->cClass].default_group, TRUE);
               for (sn = 0; sn < MAX_SKILL; sn++)
               {
                 if (skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0)
                   ch->pcdata->learned[sn] = 75;
               }

               for (i = 0;i < MAX_STATS;i++)
               {
                 ch->perm_stat[i] = 25;
               }

               ch->max_hit = 300;
               ch->hit = 300;
               ch->max_mana = 300;
               ch->mana = 300;

                    write_to_buffer (d, "\n\r", 2);
                    sprintf(buf, "Choose your homeland:");
                    write_to_buffer (d, buf, 0);
                    d->connected = CON_GET_NEW_RACE;
                    break;
             }
             else
             {
		send_to_desc("That is not the password.\n\r", d);
                wiznet("Forsaken Password Incorrectly entered.", NULL,
                        NULL, WIZ_LOGINS, 0, 100);
                log_string("Forsaken Password Incorrectly entered.");
                close_socket(d);
                return;
             }
             break;

    case CON_GET_DRAGON_PASSWORD:
            if (!str_cmp(argument, "impsonlypass"))
            {
               int sn, i;

               ch->alignment = 750;
               group_add (ch, "rom basics", FALSE);
               group_add (ch, class_table[ch->cClass].base_group, FALSE);
               group_add (ch, class_table[ch->cClass].default_group, TRUE);
               for (sn = 0; sn < MAX_SKILL; sn++)
               {
                 if (skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0)
                   ch->pcdata->learned[sn] = 80;
               }

               for (i = 0;i < MAX_STATS;i++)
               {
                 ch->perm_stat[i] = 25;
               }

               ch->max_hit = 700;
               ch->hit = 700;
               ch->max_mana = 700;
               ch->mana = 700;

                    write_to_buffer (d, "\n\r", 2);
                    sprintf(buf, "Choose your homeland:");
                    write_to_buffer (d, buf, 0);
                    d->connected = CON_GET_NEW_RACE;
                    break;
             }
             else
             {
            		send_to_desc("That is not the password.\n\r", d);
                wiznet("Dragon Password Incorrectly entered.", NULL,
                        NULL, WIZ_LOGINS, 0, 100);
                log_string("Dragon Password Incorrectly entered.");
                close_socket(d);
                return;
             }
             break;

        case CON_GET_POWER:
        {
          int i;
  	  int sum = 0;
  	  char arg[MAX_WEAVES][MIL];
  	  int num[MAX_WEAVES];
  	  int points;
  	  char buf[MSL];

  	  if (ch->cClass == class_lookup("Dragon"))
    	    points = 20;
  	  else if (ch->cClass == class_lookup("Forsaken"))
    	    points = 15;
  	  else
   	    points = 10;

	  for (i = 0;i < MAX_WEAVES;i++)
	    argument = one_argument(argument, arg[i]);

	  for (i = 0;i < MAX_WEAVES;i++)
	  {
	    if (!is_number(arg[i]))
	    {
	      sprintf(buf, "All 5 arguments, must be numbers\n\r"
		 "Syntax is power <fire> <earth> <air> <water> <spirit>\n\r"
		 "The numbers must add up to %d no negative numbers\n\r",
		 points);
	      send_to_desc(buf, d);
	      send_to_desc("Please re-enter your 5 weave power values.:", d);
	      return;
	    }
	  }

	  for (i = 0;i < MAX_WEAVES;i++)
	  {
	    num[i] = atoi(arg[i]);
	  }


	  for (i = 0; i < MAX_WEAVES; i++)
	  {
	    if (num[i] < 0)
	    {
	      send_to_desc("No negative numbers.\n\r", d);
	      send_to_desc("Please re-enter your 5 weave power values.:", d);
	      return;
	    }
	  }

	  for (i = 0; i < MAX_WEAVES; i++)
	  {
	    if (num[i] > points / 3)
	    {
	      sprintf(buf, "You cannot exceed %d per stat.\n\r", points / 3);
	      send_to_desc(buf, d);
	      send_to_desc("Please re-enter your 5 weave power values.:", d);
	      return;
 	    }
	 }

	  for (i = 0; i < MAX_WEAVES; i++)
	  {
	    sum += num[i];
	  }

	  if (sum != points)
	  {
	    sprintf(buf, "Power values must add up to %d.\n\r", points);
	    send_to_desc(buf, d);
	    send_to_desc("Please re-enter your 5 weave power values.:", d);
	    return;
	  }

	  for (i = 0; i < MAX_WEAVES; i++)
	  {
	    ch->pcdata->weaves[i] = num[i];
	  }
	  send_to_desc("Power values changed.\n\r", d);

	  write_to_buffer (d, "\n\r", 2);
                    write_to_buffer (d,
                                     "Please pick a weapon from the following choices:\n\r",
                                     0);
          buf[0] = '\0';
          for (i = 0; weapon_table[i].name != NULL; i++)
            if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
            {
              strcat (buf, weapon_table[i].name);
              strcat (buf, " ");
            }
          strcat (buf, "\n\rYour choice? ");
          write_to_buffer (d, buf, 0);
	  d->connected = CON_PICK_WEAPON;
          break;
        }
        case CON_GET_ALIGNMENT:
            switch (argument[0])
            {
                case 'g':
                case 'G':
                    ch->alignment = 750;
                    break;
                case 'n':
                case 'N':
                    ch->alignment = 0;
                    break;
                case 'e':
                case 'E':
                    ch->alignment = -750;
                    break;
                default:
                    send_to_desc ("That is not a valid alignment.\n\r", d);
                    send_to_desc ("Which alignment (G/N/E)? ", d);
                    return;
            }

            write_to_buffer (d, "\n\r", 0);

            if (IS_DRAGON(ch) || IS_FORSAKEN(ch))
            {
                    sprintf(buf, "\n\r\n\rEnter your power allocations adding up to 15 FS 20 Dragon\n\r"
                    		 "Ex: 2 2 2 2 2 or 3 1 2 2 2\n\r\n\r"
                    		 "Values:");
                    write_to_buffer (d, buf, 0);
                    d->connected = CON_GET_POWER;
                    break;
            }

            group_add (ch, "rom basics", FALSE);
            group_add (ch, class_table[ch->cClass].base_group, FALSE);
            ch->pcdata->learned[gsn_recall] = 50;
            send_to_desc ("Do you wish to customize this character?\n\r", d);
            send_to_desc
                ("Customization takes time, but allows a wider range of skills and abilities.\n\r",
                 d);
            send_to_desc ("Customize (Y/N)? ", d);
            d->connected = CON_DEFAULT_CHOICE;
            break;

        case CON_DEFAULT_CHOICE:
            write_to_buffer (d, "\n\r", 2);
            switch (argument[0])
            {
                case 'y':
                case 'Y':
                    ch->gen_data = new_gen_data ();
                    ch->gen_data->points_chosen = ch->pcdata->points;
                    do_function (ch, &do_mhelp, "* 'group header'");
                    list_group_costs (ch);
                    write_to_buffer (d,
                                     "You already have the following skills:\n\r",
                                     0);
                    do_function (ch, &do_skills, "");
                    do_function (ch, &do_mhelp, "* 'menu choice'");
                    d->connected = CON_GEN_GROUPS;
                    break;
                case 'n':
                case 'N':
                    group_add (ch, class_table[ch->cClass].default_group,
                               TRUE);
                    sprintf(buf, "In the Age of Darkness each character has the ability to channel the weaves\n\r"
                		 "However they are not all of the same strength in each of the 5 powers:\n\r"
                		 "Fire, Earth, Air, Water, Spirit\n\r");
                    write_to_buffer (d, buf, 0);
                    sprintf(buf, "\n\rThese five powers will add up to your existing stats which are:\n\r");
                    send_to_desc(buf, d);
                    for (i = 0; i < MAX_WEAVES; i++)
                    {
                      sprintf(buf, "%d ",  class_table[ch->cClass].weaves[i] + sex_table[ch->sex].weaves[i]);
                      send_to_desc(buf, d);
                    }
                    sprintf(buf, "\n\r\n\rEnter your power allocations adding up to 10\n\r"
                 		 "Ex: 2 2 2 2 2 or 3 1 2 2 2\n\r\n\r"
                    		 "Values:");
                    write_to_buffer (d, buf, 0);

                    d->connected = CON_GET_POWER;
                    break;
                default:
                    write_to_buffer (d, "Please answer (Y/N)? ", 0);
                    return;
            }
            break;

        case CON_PICK_WEAPON:
            write_to_buffer (d, "\n\r", 2);
            weapon = weapon_lookup (argument);
            if (weapon == -1
                || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0)
            {
                write_to_buffer (d,
                                 "That is not a valid selection. Choices are:\n\r",
                                 0);
                buf[0] = '\0';
                for (i = 0; weapon_table[i].name != NULL; i++)
                    if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
                    {
                        strcat (buf, weapon_table[i].name);
                        strcat (buf, " ");
                    }
                strcat (buf, "\n\rYour choice? ");
                write_to_buffer (d, buf, 0);
                return;
            }

            ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
	    SET_BIT(ch->act, PLR_NOPK);
            write_to_buffer (d, "\n\r", 2);
            do_function (ch, &do_mhelp, "* 'intro note'");
            d->connected = CON_READ_MOTD;
            break;

        case CON_GEN_GROUPS:
            send_to_char ("\n\r", ch);

            if (!str_cmp (argument, "done"))
            {
                if (ch->pcdata->points == pc_race_table[ch->race].points)
                {
                    send_to_char ("You didn't pick anything.\n\r", ch);
                    break;
                }

                if (ch->pcdata->points < 40+ pc_race_table[ch->race].points)
                {
                    sprintf (buf,
                             "You must take at least %d points of skills and groups",
                             40+pc_race_table[ch->race].points);
                    send_to_char (buf, ch);
                    break;
                }

                sprintf (buf, "Creation points: %d\n\r", ch->pcdata->points);
                send_to_char (buf, ch);
                sprintf (buf, "Experience per level: %d\n\r",
                         exp_per_level (ch, ch->gen_data->points_chosen));
                if (ch->pcdata->points < 60)
                    ch->train = (60 - ch->pcdata->points + 1) / 2;
		if (ch->train < 5) ch->train = 5;
                free_gen_data (ch->gen_data);
                ch->gen_data = NULL;
                send_to_char (buf, ch);
                write_to_buffer (d, "\n\r", 2);

                sprintf(buf, "In the Age of Darkness each character has the ability to channel the weaves\n\r"
                		 "However they are not all of the same strength in each of the 5 powers:\n\r"
                		 "Fire, Earth, Air, Water, Spirit\n\r");
                write_to_buffer (d, buf, 0);
                sprintf(buf, "\n\rThese five powers will add up to your existing stats which are:\n\r");
                send_to_desc(buf, d);
                for (i = 0; i < MAX_WEAVES; i++)
                {
                  sprintf(buf, "%d ",  class_table[ch->cClass].weaves[i] + sex_table[ch->sex].weaves[i]);
                  send_to_desc(buf, d);
                }
                sprintf(buf, "\n\r\n\rEnter your power allocations adding up to 10\n\r"
                 		 "Ex: 2 2 2 2 2 or 3 1 2 2 2\n\r\n\r"
                    		 "Values:");
                write_to_buffer (d, buf, 0);
                d->connected = CON_GET_POWER;

                break;
            }

            if (!parse_gen_groups (ch, argument))
                send_to_char
                    ("Choices are: list,learned,premise,add,drop,info,help, and done.\n\r",
                     ch);

            do_function (ch, &do_mhelp, "* 'menu choice'");
            break;

/* Diem's Grand Section for Pfile Updating!!
*
*
* This section to be used for general game updates, and optional changes to pfiles
* based on a characters choice.
*
* Update #1 - Optional Looting (On/Off)
*/
            case OPTION_UPDATE1:
             if (ch->update < 1)
             {
             send_to_char("`&Update:`7 Would you like to have the ability to loot others\n\r", ch);
             send_to_char("and in-turn allow those people to loot you? If this is turned on\n\r", ch);
             send_to_char("it can never be turned off, or visaversa. (Y/N)\n\r", ch);
             }
            if (ch->update < 1)
            {
             switch (*argument)

             {
                      case 'y':
                      case 'Y':
                      SET_BIT(ch->act2, PLR_LOOTABLE);
                      send_to_char("`&LOOTING ENABLED`7\n\r", ch);
                      send_to_char("Thank you, continue on please.\n\r", ch);
                      ch->update = 1;
                      break;

                      case 'n':
                      case 'N':
                      send_to_char("`&LOOTING DISABLED`7\n\r", ch);
                      send_to_char("Thank you, continue on please.\n\r", ch);
                      ch->update = 1;
                      break;

                      default:
                      send_to_char("Please type yes or no.\n\r", ch);
                      return;
                 }
                 break;

            }

        case CON_READ_IMOTD:
            write_to_buffer (d, "\n\r", 2);
	    if (IS_IMMORTAL(ch))
	            do_function (ch, &do_mhelp, "* imotd");
	    else
	  	    do_function (ch, &do_mhelp, "* information");
            d->connected = CON_READ_MOTD;
            break;

        case CON_READ_MOTD:
            if (ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0')
            {
                write_to_buffer (d, "Warning! Null password!\n\r", 0);
                write_to_buffer (d,
                                 "Please report old password with bug.\n\r",
                                 0);
                write_to_buffer (d,
                                 "Type 'password null <new password>' to fix.\n\r",
                                 0);
            }

            write_to_buffer (d,
                             "\n\rWelcome to Age of Darkness.\n\r",
                             0);
            ch->next = char_list;
            char_list = ch;
            d->connected = CON_PLAYING;
            reset_char (ch);

            if (ch->level == 0)
            {

                SET_BIT (ch->act, PLR_COLOUR);
		SET_BIT (ch->comm, COMM_NOVICE);
              //  ch->perm_stat[class_table[ch->cClass].attr_prime] += 3;

                mudstats.current_newbies += 1;
                mudstats.total_newbies +=1;
                mudstats.current_players += 1;
                mudstats.current_logins += 1;
                mudstats.total_logins +=1;
                if (mudstats.current_players > mudstats.current_max_players)
                {
                 mudstats.current_max_players = mudstats.current_players;
                 if (mudstats.total_max_players < mudstats.current_max_players)
                   mudstats.total_max_players = mudstats.current_max_players;
                }

                ch->level = 1;
                ch->exp = exp_per_level (ch, ch->pcdata->points);
                ch->hit = ch->max_hit;
                ch->mana = ch->max_mana;
                ch->move = ch->max_move;
                ch->train = 3;
                ch->practice = 5;
                sprintf (buf, "the Newcomer.");
                set_title (ch, buf);

                do_function (ch, &do_outfit, "");
                obj_to_char (create_object (get_obj_index (OBJ_VNUM_MAP), 0),
                             ch);

                char_to_room (ch, get_room_index (ROOM_VNUM_SCHOOL));
                send_to_char ("\n\r", ch);
                do_function (ch, &do_mhelp, "* information");
                send_to_char ("\n\r", ch);
            }
            else if (ch->in_room != NULL)
            {
                char_to_room (ch, ch->in_room);
            }
            else if (IS_IMMORTAL (ch))
            {
                char_to_room (ch, get_room_index (ROOM_VNUM_CHAT));
            }
            else
            {
                char_to_room (ch, get_room_index (ROOM_VNUM_TEMPLE));
            }

            act ("$n has entered the game.", ch, NULL, NULL, TO_ROOM);
            if (!IS_IMMORTAL(ch))
              tax(ch);
            do_function (ch, &do_look, "auto");
            ch->pcdata->wait_timer = 90;
            wiznet ("$N has left real life behind.", ch, NULL,
                    WIZ_LOGINS, WIZ_SITES, get_trust (ch));

            if (ch->pet != NULL)
            {
                char_to_room (ch->pet, ch->in_room);
                act ("$n has entered the game.", ch->pet, NULL, NULL,
                     TO_ROOM);
            }

            do_function (ch, &do_unread, "");
            if (IS_SET(ch->act2, PLR_MXP))
            {
              do_function(ch, &do_elements, "auto");
            }
            check_action(ch);
            break;
    }

    return;
}



/*
 * Parse a name for acceptability.
 */
bool check_parse_name (char *name)
{
    int clan;

    /*
     * Reserved words.
     */
    if (is_exact_name (name,
                       "all auto immortal self someone something the you loner none someone new fuck shit suck cock balls cunt asshole assmo account newchar"))
    {
        return FALSE;
    }

    /* check clans */
    for (clan = 0; clan < top_clan; clan++)
    {
        if (LOWER (name[0]) == LOWER (clan_table[clan].name[0])
            && !str_cmp (name, clan_table[clan].name))
            return FALSE;
    }

    for (int i = 0; i < top_illegal; i++)
    {
      if(!str_cmp(name, illegal_names[i]))
        return FALSE;
    }

    if (str_cmp (capitalize (name), "Alander") && (!str_prefix ("Alan", name)
                                                   || !str_suffix ("Alander",
                                                                   name)))
        return FALSE;

    /*
     * Length restrictions.
     */

    if (strlen (name) < 4)
        return FALSE;

#if defined(MSDOS)
    if (strlen (name) > 8)
        return FALSE;
#endif

#if defined(macintosh) || defined(unix)
    if (strlen (name) > 12)
        return FALSE;
#endif

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
        char *pc;
        bool fIll, adjcaps = FALSE, cleancaps = FALSE;
        unsigned int total_caps = 0;

        fIll = TRUE;
        for (pc = name; *pc != '\0'; pc++)
        {
            if (!isalpha (*pc))
                return FALSE;

            if (isupper (*pc))
            {                    /* ugly anti-caps hack */
                if (adjcaps)
                    cleancaps = TRUE;
                total_caps++;
                adjcaps = TRUE;
            }
            else
                adjcaps = FALSE;

            if (LOWER (*pc) != 'i' && LOWER (*pc) != 'l')
                fIll = FALSE;
        }

        if (fIll)
            return FALSE;

        if (cleancaps
            || (total_caps > (strlen (name)) / 2
                && strlen (name) < 3)) return FALSE;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    {
        extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
        MOB_INDEX_DATA *pMobIndex;
        int iHash;

        for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
        {
            for (pMobIndex = mob_index_hash[iHash];
                 pMobIndex != NULL; pMobIndex = pMobIndex->next)
            {
                if (is_exact_name (name, pMobIndex->player_name))
                    return FALSE;
            }
        }
    }

    return TRUE;
}




/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect (DESCRIPTOR_DATA * d, char *name, bool fConn)
{
    CHAR_DATA *ch;

    for (ch = char_list; ch != NULL; ch = ch->next)
    {
        if (!IS_NPC (ch)
            && (!fConn || ch->desc == NULL)
            && !str_cmp (name, IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name))
        {
            if (fConn == FALSE)
            {
                free_string (d->character->pcdata->pwd);
                d->character->pcdata->pwd = str_dup (ch->pcdata->pwd);
            }
            else
            {
              mudstats.current_players += 1;
              mudstats.current_logins += 1;
              mudstats.total_logins +=1;
              if (mudstats.current_players > mudstats.current_max_players)
              {
                mudstats.current_max_players = mudstats.current_players;
                if (mudstats.total_max_players < mudstats.current_max_players)
                  mudstats.total_max_players = mudstats.current_max_players;
              }

                free_char (d->character);
                d->character = ch;
                ch->desc = d;
                ch->timer = 0;
                send_to_char
                    ("Reconnecting. Type replay to see missed tells.\n\r",
                     ch);
                act ("$n has reconnected.", ch, NULL, NULL, TO_ROOM);

                sprintf (log_buf, "%s@%s reconnected.", ch->name, dns_gethostname(d->host));
                log_string (log_buf);

                wiznet ("$N groks the fullness of $S link.",
                        ch, NULL, WIZ_LINKS, 0, 0);
                if (IS_SET(ch->act, PLR_TOURNEY))
                {
                  if (tournament.status != TOURNAMENT_ON)
                    REMOVE_BIT(ch->act, PLR_TOURNEY);
                  else
		    tournament.players += 1;
                }

		if (IS_SET(ch->act2, PLR_TOURNAMENT_START))
                {
                  if (tournament.status != TOURNAMENT_STARTING)
		    REMOVE_BIT(ch->act2, PLR_TOURNAMENT_START);
		  else
          	     tournament.players += 1;
		}

                d->connected = CON_PLAYING;
            }
            return TRUE;
        }
    }

    return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing (DESCRIPTOR_DATA * d, char *name)
{
    DESCRIPTOR_DATA *dold;

    for (dold = descriptor_list; dold; dold = dold->next)
    {
        if (dold != d
            && dold->character != NULL
            && dold->connected != CON_GET_NAME
            && dold->connected != CON_GET_OLD_PASSWORD
            && !str_cmp (name, dold->original
                         ? IS_DISGUISED(dold->original)?dold->original->pcdata->disguise.orig_name:dold->original->name
                         : IS_DISGUISED(dold->character)?dold->character->pcdata->disguise.orig_name:dold->character->name))
        {
            write_to_buffer (d, "That character is already playing.\n\r", 0);
            write_to_buffer (d, "Do you wish to connect anyway (Y/N)?", 0);
            d->connected = CON_BREAK_CONNECT;
            return TRUE;
        }
    }

    return FALSE;
}



void stop_idling (CHAR_DATA * ch)
{
    if (ch == NULL
        || ch->desc == NULL
        || ch->desc->connected != CON_PLAYING
        || ch->was_in_room == NULL
        || ch->in_room != get_room_index (ROOM_VNUM_LIMBO)) return;

    ch->timer = 0;
    char_from_room (ch);
    char_to_room (ch, ch->was_in_room);
    ch->was_in_room = NULL;
    act ("$n has returned from the void.", ch, NULL, NULL, TO_ROOM);
    return;
}



/*
 * Write to one char.
 */
void send_to_char_bw (const char *txt, CHAR_DATA * ch)
{
    if (txt != NULL && ch->desc != NULL)
        write_to_buffer (ch->desc, txt, strlen (txt));
    return;
}

/*
 * Send a page to one char.
 */
void page_to_char_bw (const char *txt, CHAR_DATA * ch)
{
    if (txt == NULL || ch->desc == NULL)
        return;

    if (ch->lines == 0)
    {
        send_to_char_bw (txt, ch);
        return;
    }

#if defined(macintosh)
    send_to_char_bw (txt, ch);
#else
    if (ch->desc->showstr_head)
      free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head) + 1);
    ch->desc->showstr_head = (char *) alloc_mem (strlen (txt) + 1);
    strcpy (ch->desc->showstr_head, txt);
    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string (ch->desc, "");
#endif
}

/*
 * Page to one char, new colour version, by Lope.
 */
void send_to_char (const char *txt, CHAR_DATA * ch)
{
    const char *point;
    char *point2;
    char buf[MAX_STRING_LENGTH * 4];
    int skip = 0;

    buf[0] = '\0';
    point2 = buf;
    if (txt && ch->desc)
    {
        if (IS_SET (ch->act, PLR_COLOUR) ||
            (IS_SWITCHED(ch) && IS_SET(ch->desc->original->act, PLR_COLOUR)))
        {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    skip = colour (*point, ch, point2);
                    while (skip-- > 0)
                        ++point2;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            write_to_buffer (ch->desc, buf, point2 - buf);
        }
        else
        {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            write_to_buffer (ch->desc, buf, point2 - buf);
        }
    }
    return;
}

/*
 * Page to one descriptor using Lope's color.
 */
void send_to_desc (const char *txt, DESCRIPTOR_DATA * d)
{
    const char *point;
    char *point2;
    char buf[MAX_STRING_LENGTH * 4];
    int skip = 0;

    buf[0] = '\0';
    point2 = buf;
    if (txt && d)
    {
        if (d->ansi == TRUE)
        {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    if (*point == '\0')
                      continue;
                    skip = colour (*point, NULL, point2);
                    while (skip-- > 0)
                        ++point2;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            write_to_buffer (d, buf, point2 - buf);
        }
        else
        {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            write_to_buffer (d, buf, point2 - buf);
        }
    }
    return;
}

void page_to_char (const char *txt, CHAR_DATA * ch)
{
    const char *point;
    char *point2;
    char buf[MAX_STRING_LENGTH * 4];
    int skip = 0;

#if defined(macintosh)
    send_to_char (txt, ch);
#else
    buf[0] = '\0';
    point2 = buf;
    if (txt && ch->desc)
    {
        if (IS_SET (ch->act, PLR_COLOUR) ||
           (IS_SWITCHED(ch) && IS_SET(ch->desc->original->act, PLR_COLOUR)))
        {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    skip = colour (*point, ch, point2);
                    while (skip-- > 0)
                        ++point2;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            if (ch->desc->showstr_head)
              free_string(ch->desc->showstr_head);
            ch->desc->showstr_head = (char *) alloc_mem (strlen (buf) + 1);
            strcpy (ch->desc->showstr_head, buf);
            ch->desc->showstr_point = ch->desc->showstr_head;
            show_string (ch->desc, "");
        }
        else
        {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    continue;
                }
                *point2 = *point;
                *++point2 = '\0';
            }
            *point2 = '\0';
            if (ch->desc->showstr_head)
              free_string(ch->desc->showstr_head);
            ch->desc->showstr_head = (char *) alloc_mem (strlen (buf) + 1);
            strcpy (ch->desc->showstr_head, buf);
            ch->desc->showstr_point = ch->desc->showstr_head;
            show_string (ch->desc, "");
        }
    }
#endif
    return;
}

/* string pager */
void show_string (struct descriptor_data *d, char *input)
{
    char buffer[4 * MAX_STRING_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;

    one_argument (input, buf);
    if (buf[0] != '\0')
    {
        if (d->showstr_head)
        {
            free_mem (d->showstr_head, strlen (d->showstr_head));
            d->showstr_head = 0;
        }
        d->showstr_point = 0;
        return;
    }

    if (d->character)
        show_lines = d->character->lines;
    else
        show_lines = 0;

    for (scan = buffer;; scan++, d->showstr_point++)
    {
        if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
            && (toggle = -toggle) < 0)
            lines++;

        else if (!*scan || (show_lines > 0 && lines >= show_lines))
        {
            *scan = '\0';
            write_to_buffer (d, buffer, strlen (buffer));
            for (chk = d->showstr_point; isspace (*chk); chk++);
            {
                if (!*chk)
                {
                    if (d->showstr_head)
                    {
                        free_mem (d->showstr_head, strlen (d->showstr_head));
                        d->showstr_head = 0;
                    }
                    d->showstr_point = 0;
                }
            }
            return;
        }
    }
    return;
}


/* quick sex fixer */
void fix_sex (CHAR_DATA * ch)
{
    if (ch->sex < 0 || ch->sex > 2)
        ch->sex = IS_NPC (ch) ? 0 : ch->pcdata->true_sex;
}

void act_new (const char *format, CHAR_DATA * ch, const void *arg1,
              const void *arg2, int type, int min_pos)
{
    static char *const he_she[] = { "it", "he", "she" };
    static char *const him_her[] = { "it", "him", "her" };
    static char *const his_her[] = { "its", "his", "her" };

    char buf[MAX_STRING_LENGTH];
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA *) arg2;
    const char *str;
    const char *i;
    char *point;
    char *pbuff;
    char buffer[MAX_STRING_LENGTH * 2];
    bool fColour = FALSE;
    char pretit[MSL];


    /*
     * Discard null and zero-length messages.
     */
    if (format == NULL || format[0] == '\0')
        return;

    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
        return;

    to = ch->in_room->people;
    if (type == TO_VICT)
    {
        if (vch == NULL)
        {
            bug ("Act: null vch with TO_VICT.", 0);
            return;
        }

        if (vch->in_room == NULL)
            return;

        to = vch->in_room->people;
    }

    for (; to != NULL; to = to->next_in_room)
    {
        if ((!IS_NPC (to) && to->desc == NULL)
            || (IS_NPC (to) && !HAS_TRIGGER_MOB (to, TRIG_ACT) && to->desc == NULL)
            || to->position < min_pos)
            continue;

        if ((type == TO_CHAR) && to != ch)
            continue;
        if (type == TO_VICT && (to != vch || to == ch))
            continue;
        if (type == TO_ROOM && to == ch)
            continue;
        if (type == TO_NOTVICT && (to == ch || to == vch))
            continue;

        point = buf;
        str = format;


        while (*str != '\0')
        {
            if (*str != '$')
            {
                *point++ = *str++;
                continue;
            }
            fColour = TRUE;
            ++str;
            i = " <@@@> ";

            if (arg2 == NULL && *str >= 'A' && *str <= 'Z')
            {
                bug ("Act: missing arg2 for code %d.", *str);
                i = " <@@@> ";
            }
            else
            {
                switch (*str)
                {
                    default:
                        bug ("Act: bad code %d.", *str);
                        i = " <@@@> ";
                        break;
                        /* Thx alex for 't' idea */
                    case 't':
                        i = (char *) arg1;
                        break;
                    case 'T':
                        i = (char *) arg2;
                        break;
                    case 'n':
                        i = IS_DISGUISED(ch) ? ch->short_descr:PERS (ch, to, FALSE);
                        break;
                    case 'N':
                        i = IS_DISGUISED(vch) ? vch->short_descr:PERS (vch, to, FALSE);
                        break;
		    case 'r':
                        if (IS_NULLSTR(ch->pretit) ||
			    !str_cmp(ch->pretit, "(null)") ||
			    !can_see(to, ch) ||
			    !known_name(ch, to))
			{
			  i = "";
			}
			else
			{
			  sprintf(pretit, "%s ", ch->pretit);
			  i = pretit;
			}
			break;
		    case 'R':
                        if (IS_NULLSTR(vch->pretit) ||
			    !str_cmp(vch->pretit, "(null)") ||
			    !can_see(to, vch) ||
			    !known_name(vch, to))
			{
			  i = "";
			}
			else
			{
			  sprintf(pretit, "%s ", vch->pretit);
			  i = pretit;
			}
			break;
                    case 'e':
                        i = he_she[URANGE (0, ch->sex, 2)];
                        break;
                    case 'E':
                        i = he_she[URANGE (0, vch->sex, 2)];
                        break;
                    case 'm':
                        i = him_her[URANGE (0, ch->sex, 2)];
                        break;
                    case 'M':
                        i = him_her[URANGE (0, vch->sex, 2)];
                        break;
                    case 's':
                        i = his_her[URANGE (0, ch->sex, 2)];
                        break;
                    case 'S':
                        i = his_her[URANGE (0, vch->sex, 2)];
                        break;

                    case 'p':
                        i = can_see_obj (to, obj1)
                            ? obj1->short_descr : "something";
                        break;

                    case 'P':
                        i = can_see_obj (to, obj2)
                            ? obj2->short_descr : "something";
                        break;

                    case 'd':
                        if (arg2 == NULL || ((char *) arg2)[0] == '\0')
                        {
                            i = "door";
                        }
                        else
                        {
                            one_argument ((char *) arg2, fname);
                            i = fname;
                        }
                        break;
                }
            }

            ++str;
            while ((*point = *i) != '\0')
                ++point, ++i;
        }

        *point++ = '\n';
        *point++ = '\r';
        *point = '\0';
        buf[0] = UPPER (buf[0]);
        pbuff = buffer;
        colourconv (pbuff, buf, to);
        if (to->desc != NULL)
            /*write_to_buffer( to->desc, buf, point - buf );*/
            write_to_buffer( to->desc, buffer, 0 );
        else if (MOBtrigger)
            p_act_trigger (buf, to, NULL, NULL, ch, arg1, arg2, TRIG_ACT);
    }

    if ( type == TO_ROOM || type == TO_NOTVICT )
    {
        OBJ_DATA *obj, *obj_next;
        CHAR_DATA *tch, *tch_next;

         point   = buf;
         str     = format;
         while( *str != '\0' )
         {
             *point++ = *str++;
         }
         *point   = '\0';

        for( obj = ch->in_room->contents; obj; obj = obj_next )
        {
            obj_next = obj->next_content;
            if ( HAS_TRIGGER_OBJ( obj, TRIG_ACT ) )
                p_act_trigger( buf, NULL, obj, NULL, ch, NULL, NULL, TRIG_ACT );
        }

        for( tch = ch; tch; tch = tch_next )
        {
            tch_next = tch->next_in_room;

            for ( obj = tch->carrying; obj; obj = obj_next )
            {
                obj_next = obj->next_content;
                if ( HAS_TRIGGER_OBJ( obj, TRIG_ACT ) )
                    p_act_trigger( buf, NULL, obj, NULL, ch, NULL, NULL, TRIG_ACT );
            }
        }

         if ( HAS_TRIGGER_ROOM( ch->in_room, TRIG_ACT ) )
             p_act_trigger( buf, NULL, NULL, ch->in_room, ch, NULL, NULL, TRIG_ACT );
    }

    return;
}


void chan_glob_act (const char *format, CHAR_DATA * ch, const void *arg1,
              const void *arg2, int type, long comm)
{
    static char *const he_she[] = { "it", "he", "she" };
    static char *const him_her[] = { "it", "him", "her" };
    static char *const his_her[] = { "its", "his", "her" };

    char buf[MAX_STRING_LENGTH];
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA *) arg2;
    const char *str;
    const char *i;
    char *point;
    char *pbuff;
    char buffer[MAX_STRING_LENGTH * 2];
    bool fColour = FALSE;
    int min_pos = POS_DEAD;


    /*
     * Discard null and zero-length messages.
     */
    if (format == NULL || format[0] == '\0')
        return;

    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
        return;

    to = char_list;
    if (type == TO_VICT)
    {
        if (vch == NULL)
        {
            bug ("Act: null vch with TO_VICT.", 0);
            return;
        }

        if (vch->in_room == NULL)
            return;

        to = char_list;
    }

    for (; to != NULL; to = to->next)
    {
        if ((!IS_NPC (to) && to->desc == NULL)
            || (IS_NPC (to) && !HAS_TRIGGER_MOB (to, TRIG_ACT))
            || to->position < min_pos)
            continue;

	if (to->comm & comm) continue;
	if (comm == COMM_NOWIZ && get_trust(to) < LEVEL_IMMORTAL) continue;
	if (comm == COMM_NOIMP && get_trust(to) < MAX_LEVEL) continue;
        if ((type == TO_CHAR) && to != ch)
            continue;
        if (type == TO_VICT && (to != vch || to == ch))
            continue;
        if (type == TO_ROOM && to == ch)
            continue;
        if (type == TO_NOTVICT && (to == ch || to == vch))
            continue;

        point = buf;
        str = format;
        while (*str != '\0')
        {
            if (*str != '$')
            {
                *point++ = *str++;
                continue;
            }
            fColour = TRUE;
            ++str;
            i = " <@@@> ";

            if (arg2 == NULL && *str >= 'A' && *str <= 'Z')
            {
                bug ("Act: missing arg2 for code %d.", *str);
                i = " <@@@> ";
            }
            else
            {
                switch (*str)
                {
                    default:
                        bug ("Act: bad code %d.", *str);
                        i = " <@@@> ";
                        break;
                        /* Thx alex for 't' idea */
                    case 't':
                        i = (char *) arg1;
                        break;
                    case 'T':
                        i = (char *) arg2;
                        break;
                    case 'n':
                        i = IS_DISGUISED(ch)?ch->short_descr:PERS (ch, to, FALSE);
                        break;
                    case 'N':
                        i = IS_DISGUISED(vch)?vch->short_descr:PERS (vch, to, FALSE);
                        break;
                    case 'e':
                        i = he_she[URANGE (0, ch->sex, 2)];
                        break;
                    case 'E':
                        i = he_she[URANGE (0, vch->sex, 2)];
                        break;
                    case 'm':
                        i = him_her[URANGE (0, ch->sex, 2)];
                        break;
                    case 'M':
                        i = him_her[URANGE (0, vch->sex, 2)];
                        break;
                    case 's':
                        i = his_her[URANGE (0, ch->sex, 2)];
                        break;
                    case 'S':
                        i = his_her[URANGE (0, vch->sex, 2)];
                        break;

                    case 'p':
                        i = can_see_obj (to, obj1)
                            ? obj1->short_descr : "something";
                        break;

                    case 'P':
                        i = can_see_obj (to, obj2)
                            ? obj2->short_descr : "something";
                        break;

                    case 'd':
                        if (arg2 == NULL || ((char *) arg2)[0] == '\0')
                        {
                            i = "door";
                        }
                        else
                        {
                            one_argument ((char *) arg2, fname);
                            i = fname;
                        }
                        break;
                }
            }

            ++str;
            while ((*point = *i) != '\0')
                ++point, ++i;
        }

        *point++ = '\n';
        *point++ = '\r';
        *point = '\0';
        buf[0] = UPPER (buf[0]);
        pbuff = buffer;
        colourconv (pbuff, buf, to);
        if (to->desc != NULL)
            /*write_to_buffer( to->desc, buf, point - buf );*/
            write_to_buffer( to->desc, buffer, 0 );
/*        else if (MOBtrigger)
            p_act_trigger (buf, to, NULL, NULL, ch, arg1, arg2, TRIG_ACT); */
    }
    return;
}


int colour (char type, CHAR_DATA * ch, char *string)
{
    PC_DATA *col;
    char code[20];
    char *p = NULL;

    if (ch && IS_NPC (ch) && ch->desc->original == ch->desc->character)
        return (0);

    if (!type)
    {
      return 0;
    }

    col = ch ? ch->pcdata : NULL;

    switch (type)
    {
        default:
            strcpy (code, CLEAR);
            break;
        case '*':
            strcpy (code, CLEAR);
            break;
/*        case 'p':
            if (col->prompt[2])
                sprintf (code, "\e[%d;3%dm%c", col->prompt[0],
                         col->prompt[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->prompt[0], col->prompt[1]);
            break;
        case 's':
            if (col->room_title[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->room_title[0], col->room_title[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->room_title[0],
                         col->room_title[1]);
            break;
        case 'S':
            if (col->room_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->room_text[0], col->room_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->room_text[0],
                         col->room_text[1]);
            break;
        case 'd':
            if (col->gossip[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->gossip[0], col->gossip[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->gossip[0], col->gossip[1]);
            break;
        case '9':
            if (col->gossip_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->gossip_text[0], col->gossip_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->gossip_text[0],
                         col->gossip_text[1]);
            break;
        case 'Z':
            if (col->wiznet[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->wiznet[0], col->wiznet[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->wiznet[0], col->wiznet[1]);
            break;
        case 'i':
            if (col->immtalk_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->immtalk_text[0], col->immtalk_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm",
                         col->immtalk_text[0], col->immtalk_text[1]);
            break;
        case 'I':
            if (col->immtalk_type[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->immtalk_type[0], col->immtalk_type[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm",
                         col->immtalk_type[0], col->immtalk_type[1]);
            break;
        case '2':
            if (col->fight_yhit[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->fight_yhit[0], col->fight_yhit[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->fight_yhit[0],
                         col->fight_yhit[1]);
            break;
        case '3':
            if (col->fight_ohit[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->fight_ohit[0], col->fight_ohit[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->fight_ohit[0],
                         col->fight_ohit[1]);
            break;
        case '4':
            if (col->fight_thit[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->fight_thit[0], col->fight_thit[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->fight_thit[0],
                         col->fight_thit[1]);
            break;
        case '5':
            if (col->fight_skill[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->fight_skill[0], col->fight_skill[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->fight_skill[0],
                         col->fight_skill[1]);
            break;
        case '1':
            if (col->fight_death[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->fight_death[0], col->fight_death[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->fight_death[0],
                         col->fight_death[1]);
            break;
        case '6':
            if (col->say[2])
                sprintf (code, "\e[%d;3%dm%c", col->say[0], col->say[1],
                         '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->say[0], col->say[1]);
            break;
        case '7':
            if (col->say_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->say_text[0], col->say_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->say_text[0],
                         col->say_text[1]);
            break;
        case 'k':
            if (col->tell[2])
                sprintf (code, "\e[%d;3%dm%c", col->tell[0], col->tell[1],
                         '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->tell[0], col->tell[1]);
            break;
        case 'K':
            if (col->tell_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->tell_text[0], col->tell_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->tell_text[0],
                         col->tell_text[1]);
            break;
        case 'l':
            if (col->reply[2])
                sprintf (code, "\e[%d;3%dm%c", col->reply[0],
                         col->reply[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->reply[0], col->reply[1]);
            break;
        case 'L':
            if (col->reply_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->reply_text[0], col->reply_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->reply_text[0],
                         col->reply_text[1]);
            break;
        case 'n':
            if (col->gtell_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->gtell_text[0], col->gtell_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->gtell_text[0],
                         col->gtell_text[1]);
            break;
        case 'N':
            if (col->gtell_type[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->gtell_type[0], col->gtell_type[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->gtell_type[0],
                         col->gtell_type[1]);
            break;
        case 'q':
            if (col->question[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->question[0], col->question[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->question[0],
                         col->question[1]);
            break;
        case 'Q':
            if (col->question_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->question_text[0], col->question_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm",
                         col->question_text[0], col->question_text[1]);
            break;
        case 'f':
            if (col->answer[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->answer[0], col->answer[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->answer[0], col->answer[1]);
            break;
        case 'F':
            if (col->answer_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->answer_text[0], col->answer_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->answer_text[0],
                         col->answer_text[1]);
            break;
        case 'e':
            if (col->music[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->music[0], col->music[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->music[0], col->music[1]);
            break;
        case 'E':
            if (col->music_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->music_text[0], col->music_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->music_text[0],
                         col->music_text[1]);
            break;
        case 'h':
            if (col->quote[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->quote[0], col->quote[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->quote[0], col->quote[1]);
            break;
        case 'H':
            if (col->quote_text[2])
                sprintf (code, "\e[%d;3%dm%c",
                         col->quote_text[0], col->quote_text[1], '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->quote_text[0],
                         col->quote_text[1]);
            break;
        case 'j':
            if (col->info[2])
                sprintf (code, "\e[%d;3%dm%c", col->info[0], col->info[1],
                         '\a');
            else
                sprintf (code, "\e[%d;3%dm", col->info[0], col->info[1]);
            break;  */

        case '4':
            strcpy (code, C_BLUE);
            break;
        case '6':
            strcpy (code, C_CYAN);
            break;
        case '2':
            strcpy (code, C_GREEN);
            break;
        case '5':
            strcpy (code, C_MAGENTA);
            break;
        case '1':
            strcpy (code, C_RED);
            break;
        case '7':
            strcpy (code, C_WHITE);
            break;
        case '3':
            strcpy (code, C_YELLOW);
            break;
        case '$':
            strcpy (code, C_B_BLUE);
            break;
        case 'B':
            strcpy (code, C_B_BLUE);
            break;
        case '^':
            strcpy (code, C_B_CYAN);
            break;
        case '@':
            strcpy (code, C_B_GREEN);
            break;
        case '%':
            strcpy (code, C_B_MAGENTA);
            break;
        case 'M':
            strcpy (code, C_B_MAGENTA);
            break;
        case '!':
            strcpy (code, C_B_RED);
            break;
        case '&':
            strcpy (code, C_B_WHITE);
            break;
        case '#':
            strcpy (code, C_B_YELLOW);
            break;
        case '8':
            strcpy (code, C_D_GREY);
            break;
			/*thanx goes to Kari for random color*/
		case '?':
                switch (number_range(1,14))
                { default : strcpy(code, C_D_GREY); break;
                  case 1 : strcpy(code, C_RED); break;
                  case 2 : strcpy(code, C_B_RED); break;
                  case 3 : strcpy(code, C_GREEN); break;
                  case 4 : strcpy(code, C_B_GREEN); break;
                  case 5 : strcpy(code, C_YELLOW); break;
                  case 6 : strcpy(code, C_B_YELLOW); break;
                  case 7 : strcpy(code, C_BLUE); break;
                  case 8 : strcpy(code, C_B_BLUE); break;
                  case 9 : strcpy(code, C_MAGENTA); break;
                  case 10 : strcpy(code, C_B_MAGENTA); break;
                  case 11 : strcpy(code, C_CYAN); break;
                  case 12 : strcpy(code, C_B_CYAN); break;
                  case 13 : strcpy(code, C_B_WHITE); break;
                  case 14 : strcpy(code, C_D_GREY); break; }
            break;
        case 'D':
            sprintf (code, "%c", '\a');
            break;
        case 'o':
            sprintf (code, "\xD6");
            break;
        case 'a':
            sprintf (code, "\xE4");
            break;
        case 'A':
            sprintf (code, "\xE5");
            break;
        case '/':
            strcpy (code, "\n\r");
            break;
        case '-':
            sprintf (code, "%c", '~');
            break;
        case '`':
            sprintf (code, "%c", '`');
            break;
    }

    p = code;

    while (*p != '\0')
    {
        *string = *p++;
        *++string = '\0';
    }

    return (strlen (code));
}

void colourconv (char *buffer, const char *txt, CHAR_DATA * ch)
{
    const char *point;
    int skip = 0;

    if (ch && ch->desc && txt)
    {

       if (IS_SET (ch->act, PLR_COLOUR)
       ||  (IS_SWITCHED(ch) && IS_SET(ch->desc->original->act, PLR_COLOUR)))
       {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    skip = colour (*point, ch, buffer);
                    while (skip-- > 0)
                        ++buffer;
                    continue;
                }
                *buffer = *point;
                *++buffer = '\0';
            }
            *buffer = '\0';
        }
        else
        {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    continue;
                }
                *buffer = *point;
                *++buffer = '\0';
            }
            *buffer = '\0';
        }
    }
    return;
}

void colourstrip(char *buffer, const char *txt)
{
    const char *point;

    if (txt)
    {
            for (point = txt; *point; point++)
            {
                if (*point == '`')
                {
                    point++;
                    continue;
                }
                *buffer = *point;
                *++buffer = '\0';
            }
            *buffer = '\0';
        }
    return;
}



/*
 * Macintosh support functions.
 */
#if defined(macintosh)
int gettimeofday (struct timeval *tp, void *tzp)
{
    tp->tv_sec = time (NULL);
    tp->tv_usec = 0;
}
#endif

/* source: EOD, by John Booth <???> */

void printf_to_char (CHAR_DATA * ch, char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH];
    va_list args;
    va_start (args, fmt);
    vsprintf (buf, fmt, args);
    va_end (args);

    send_to_char (buf, ch);
}

void bugf (char *fmt, ...)
{
    char buf[2 * MSL];
    va_list args;
    va_start (args, fmt);
    vsprintf (buf, fmt, args);
    va_end (args);

    bug (buf, 0);
}

void client_information(CHAR_DATA *ch, char *argument)
{
  char version[MIL];
  char client[MIL];
  char client_version[MIL];
  int i;

  if (!ch)
  {
    logf("Client_Info: No Char", 0);
    return;
  }

  while (*argument != '\0')
  {
    if (*argument == '=')
    {
      argument++;
      break;
    }
    argument++;
  }

  i = 0;
  while (*argument != '\0')
  {
    if (*argument == ' ')
    {
      argument++;
      break;
    }
    version[i] = *argument;
    i++;
    argument++;
  }
  version[i] = 0;
//  logf("Version %s", version);

  while (*argument != '\0')
  {
    if (*argument == '=')
    {
      argument++;
      break;
    }
    argument++;
  }

  i = 0;
  while (*argument != '\0')
  {
    if (*argument == ' ')
    {
      argument++;
      break;
    }
    client[i] = *argument;
    i++;
    argument++;
  }
  client[i] = 0;
//  logf("Client %s", client);

  while (*argument != '\0')
  {
    if (*argument == '=')
    {
      argument++;
      break;
    }
    argument++;
  }

  i = 0;
  while (*argument != '\0')
  {
    if (*argument == ' ')
    {
      argument++;
      break;
    }
    client_version[i] = *argument;
    i++;
    argument++;
  }
  client_version[i] = 0;
//  logf("Client Version %s", client_version);

  ch->pcdata->mxpVersion = str_dup(version);
  ch->pcdata->clientVersion = str_dup(client_version);
  ch->pcdata->client = str_dup(client);

}


void interp_mxp(CHAR_DATA *ch, char *argument)
{
  char tag[MIL];
  int i;

  if (!ch)
  {
    logf("Interp_MXP: No Char", 0);
    return;
  }

  while ( *argument == ' ')
     argument++;

//  logf("%s", argument);
/*  if ( *argument == '[' )
     argument++;
  else
  {
    logf("Interp_MXP: Looking for [ got %c", *argument);
    return;
  }
*/
  if ( *argument == '1' )
    argument++;
  else
  {
    logf("Interp_MXP: Looking for 1 got %c", *argument);
    return;
  }

  if ( *argument == 'z' )
    argument++;
  else
  {
    logf("Interp_MXP: Looking for z got %c", *argument);
    return;
  }

  if ( *argument == '<' )
    argument++;
  else
  {
    logf("Interp_MXP: Looking for < got %c", *argument);
    return;
  }
//  logf("Text %c", *argument);

  i = 0;
  while ( *argument != '\0')
  {
    if (*argument == ' ')
    {
      argument++;
      break;
    }
    tag[i] = *argument;
    i++;
    argument++;
  }
//  logf("Text2 %c", *argument);
  tag[i] = '\0';

  if (!str_cmp(tag, "version"))
  {
    client_information(ch, argument);
    return;
  }
  else
  {
   send_to_char(tag, ch);
   logf("Interp_MXP: No tag %s", tag);
  }

}

void init_signals()
{
  signal(SIGFPE,sig_handler);
  signal(SIGILL,sig_handler);
  signal(SIGINT,sig_handler);
  signal(SIGBUS,sig_handler);
  signal(SIGTERM,sig_handler);
  signal(SIGABRT,sig_handler);
  // signal(SIGSEGV,sig_handler);
}


void sig_handler(int signum)
{
    if (!str_cmp (last_command,""))
        return;
switch(signum)
  {
  case SIGFPE:
    bug("Sig handler SIGFPE.",0);
    strcat(last_command,"Sig handler SIGFPE.\n");
    signal(SIGFPE,SIG_DFL);
    do_auto_shutdown();
    break;
  case SIGILL:
    bug("Sig handler SIGILL.",0);
    strcat(last_command,"Sig handler SIGILL.\n");
    signal(SIGILL,SIG_DFL);
    do_auto_shutdown();
    break;
  case SIGINT:
    bug("Sig handler SIGINT.",0);
    strcat(last_command,"Sig handler SIGINT.\n");
    signal(SIGINT,SIG_DFL);
    do_auto_shutdown();
    break;
  case SIGBUS:
    bug("Sig handler SIGBUS.",0);
    strcat(last_command,"Sig handler SIGBUS.\n");
    signal(SIGBUS,SIG_DFL);
    do_auto_shutdown();
    break;
  case SIGTERM:
    bug("Sig handler SIGTERM.",0);
    strcat(last_command,"Sig handler SIGTERM.\n");
    signal(SIGTERM,SIG_DFL);
    do_auto_shutdown();
    break;
  case SIGABRT:
    bug("Sig handler SIGABRT",0);
    strcat(last_command,"Sig handler SIGABRT.\n");
    signal(SIGABRT,SIG_DFL);
    do_auto_shutdown();
    break;
  case SIGSEGV:
    bug("Sig handler SIGSEGV",0);
    strcat(last_command,"Sig handler SIGSEGV.\n");
    signal(SIGSEGV,SIG_DFL);
    do_auto_shutdown();
    break;
  }
}

void do_auto_shutdown()
{

/*This allows for a shutdown without somebody in-game actually calling it.
		-Ferric*/
    FILE *fp;
    CHAR_DATA *vch;
    //extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;


    /* This is to write to the file. */
    if (!str_cmp(last_command,""))
    	return;

    fclose(fpReserve);
    if((fp = fopen(LAST_COMM_FILE,"w")) == NULL)
      bug("Error in do_auto_save opening last_command.txt",0);

      fprintf(fp,"Last Command: %s\n",
            last_command);

    fclose( fp );
    fpReserve = fopen( NULL_FILE, "r" );

    system_note ("System Note",NOTE_NOTE,"Immortal","Last Command Before Crash",last_command);

    sprintf(last_command, " ");
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if (vch != NULL)
        {
            send_to_char("Saving...\n\r",vch);
            save_char_obj (vch);
        }
        close_socket (d);
    }
    return;
}
void echoall( char * argument)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;
    for (d = descriptor_list; d; d = d->next)
    {
    	ch = d->original ? d->original : d->character;
        if (ch!=NULL)
        {
            send_to_char (argument, ch);
            send_to_char ("\n\r", ch);
        }
    }
    return;
}

void file_to_desc(char *filename, DESCRIPTOR_DATA *desc) // by prool
	{
	FILE *fp;
	char proolbuf [PROOLBUFSIZE];

	if (filename==0) return;
	if (*filename==0) return;
	fp=fopen(filename,"r");
	if (fp==NULL)
		{
		send_to_desc("file_to_desc: file not found\r\n\r\n\r\n", desc);
		printf("prool debug: File '%s' not found\r\n", filename);
		}
	else
		{
		while (1)
			{
			proolbuf[0]=0;
			fgets(proolbuf,PROOLBUFSIZE,fp);
			if (proolbuf[0]==0) break;
			send_to_desc(proolbuf, desc);
			}
		}
	}

