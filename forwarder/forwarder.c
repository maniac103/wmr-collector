#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#define MAXEVENTS  64
#define MAXCLIENTS 32

#define HEARTBEAT_INTERVAL 25
#define RESET_TIMEOUT      60

static int g_running = 1;

static int
create_and_bind_socket(char *port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* FIXME: All interfaces */

    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0) {
	fprintf (stderr, "getaddrinfo: %s\n", gai_strerror(s));
	return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
	int opt = 1;

	sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	if (sfd == -1) {
	    continue;
	}

	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) == -1) {
	    perror("setsockopt");
	} else if (bind(sfd, rp->ai_addr, rp->ai_addrlen) < 0) {
	    perror("bind");
	} else {
	    /* We managed to bind successfully! */
	    break;
	}

	close(sfd);
    }

    if (rp == NULL) {
	fprintf(stderr, "Could not bind\n");
	return -1;
    }

   freeaddrinfo(result);
   return sfd;
}

static int
make_fd_non_blocking (int fd)
{
    int flags, s;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
	perror("fcntl");
	return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(fd, F_SETFL, flags);
    if (s == -1) {
	perror("fcntl");
	return -1;
    }

    return 0;
}

static int
write_exact(int fd, const char *buf, ssize_t len)
{
    ssize_t written = 0;

    while (written < len) {
	ssize_t n = write(fd, buf, len - written);

	if (n < 0) {
	    if (errno == EAGAIN || errno == EWOULDBLOCK) {
		continue;
	    }
	    return -1;
	} else if (n == 0) {
	    return 0;
	} else {
	    buf += n;
	    written += n;
	}
    }

    return written;
}

static int
send_hmr_reset(int fd)
{
    static const char reset[] = { 0x20, 0x00, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00 };
    return write_exact(fd, reset, sizeof(reset));
}

static int
send_hmr_heartbeat(int fd)
{
    static const char heartbeat[] = { 0x01, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    return write_exact(fd, heartbeat, sizeof(heartbeat));
}

static int
add_fd_to_epoll(int efd, int fd)
{
    struct epoll_event event;

    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event) < 0) {
	perror("epoll_ctl");
	return -1;
    }

    return 0;
}

static void
remove_client(int fd, int *clientfds, int *client_count)
{
    int cl, clnext;

    close (fd);
    for (cl = 0; cl < *client_count; cl++) {
	if (fd == clientfds[cl]) {
	    for (clnext = cl + 1; clnext < *client_count; clnext++) {
		clientfds[cl] = clientfds[clnext];
	    }
	    (*client_count)--;
	    break;
	}
    }
}

static void
event_loop(int sockfd, int hidfd)
{
    int efd, i, s;
    struct epoll_event events[MAXEVENTS];
    struct timeval last_recv, last_heartbeat;
    int clientfds[MAXCLIENTS];
    int client_count = 0;

    efd = epoll_create1(0);
    if (efd == -1) {
	perror("epoll_create");
	goto out;
    }

    if (add_fd_to_epoll(efd, hidfd) < 0 || add_fd_to_epoll(efd, sockfd) < 0) {
	goto out;
    }

    memset(events, 0, sizeof(events));
    memset(&last_recv, 0, sizeof(last_recv));
    memset(&last_heartbeat, 0, sizeof(last_heartbeat));

    /* The event loop */
    while (1) {
	int n, item;

	n = epoll_wait (efd, events, MAXEVENTS, 2000);
	if (n < 0) {
	    if (errno != EINTR) {
		perror("epoll_wait");
	    }
	    goto out;
	}

	if (client_count > 0) {
	    struct timeval now;

	    gettimeofday(&now, NULL);
	    if (now.tv_sec - last_recv.tv_sec > RESET_TIMEOUT) {
		send_hmr_reset(hidfd);
		last_recv = now;
	    }
	    if (now.tv_sec - last_heartbeat.tv_sec > HEARTBEAT_INTERVAL) {
		send_hmr_heartbeat(hidfd);
		last_heartbeat = now;
	    }
	}

	for (item = 0; item < n; item++) {
	    struct epoll_event *ev = &events[item];

	    if (ev->events & (EPOLLERR | EPOLLHUP)) {
		fprintf (stderr, "epoll error\n");
		if (ev->data.fd == sockfd || ev->data.fd == hidfd) {
		    goto out;
		} else {
		    remove_client(ev->data.fd, clientfds, &client_count);
		    continue;
		}
	    } else if (sockfd == ev->data.fd) {
		/* We have a notification on the listening socket, which
		 * means one or more incoming connections. */
		while (1) {
		    struct sockaddr in_addr;
		    socklen_t in_len;
		    int infd;
		    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

		    in_len = sizeof(in_addr);
		    infd = accept(sockfd, &in_addr, &in_len);
		    if (infd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
			    /* We have processed all incoming
			     * connections. */
			    break;
			} else {
			    perror("accept");
			    break;
			}
		    }

		    s = getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf),
				    sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
		    if (s == 0) {
			printf("Accepted connection on descriptor %d "
			       "(host=%s, port=%s)\n", infd, hbuf, sbuf);
		    }

		    /* Make the incoming socket non-blocking and add it to the
		     * list of fds to monitor. */
		    s = make_fd_non_blocking(infd);
		    if (s == -1) {
			close(infd);
			continue;
		    }

		    if (add_fd_to_epoll(efd, infd) < 0) {
			close(infd);
			continue;
		    }
		    if (client_count < MAXCLIENTS) {
			if (client_count == 0) {
			    send_hmr_reset(hidfd);
			    send_hmr_heartbeat(hidfd);
			    gettimeofday(&last_recv, NULL);
			    last_heartbeat = last_recv;
			}
			clientfds[client_count++] = infd;
		    }
		}
	    } else if (hidfd == ev->data.fd) {
		char buf[8];
		ssize_t count = read(hidfd, buf, sizeof(buf));

		gettimeofday(&last_recv, NULL);
		if (count == 8) {
		    int cl, length = buf[0];
		    if (length < 8) {
			for (cl = 0; cl < client_count; ) {
			    if (write_exact(clientfds[cl], buf + 1, length) < 0) {
				remove_client(clientfds[cl], clientfds, &client_count);
				/* keep cl, as remove_client shifted the array */
			    } else {
				cl++;
			    }
			}
		    }
		}
	    } else {
		/* We have data on the fd waiting to be read. Read and
		 * discard it. We must read whatever data is available
		 * completely, as we are running in edge-triggered mode
		 * and won't get a notification again for the same
		 * data. */
		int done = 0;

		while (1) {
		    ssize_t count;
		    char buf[512];

		    count = read (ev->data.fd, buf, sizeof buf);
		    if (count == -1) {
			/* If errno == EAGAIN, that means we have read all
			 * data. So go back to the main loop. */
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
			    perror ("read");
			    done = 1;
			}
			break;
		    } else if (count == 0) {
			/* End of file. The remote has closed the connection */
			done = 1;
			break;
		    }
		}

		if (done) {
		    printf ("Closed connection on descriptor %d\n", ev->data.fd);
		    /* Closing the descriptor will make epoll remove it
		     * from the set of descriptors which are monitored. */
		    remove_client(ev->data.fd, clientfds, &client_count);
		}
	    }
	}
    }

out:
    for (i = 0; i < client_count; i++) {
	close(clientfds[i]);
    }
    if (efd >= 0) {
	close(efd);
    }
}

static void
usage(const char *program)
{
    fprintf(stderr, "Usage: %s [-f] [-p port] [-P pidfile] devicename\n", program);
}

static void
handle_sigterm(int signal)
{
    g_running = 0;
}

int
main (int argc, char *argv[])
{
    int opt, hidfd, sfd = -1;
    int daemonize = 1;
    char *port = "9876";
    char *pid_path = NULL;
    char *hid_path = NULL;
    int pidfd = -1;

    while ((opt = getopt(argc, argv, "fp:P:d:")) != -1) {
	switch (opt) {
	    case 'f':
		daemonize = 0;
		break;
	    case 'p':
		port = optarg;
		break;
	    case 'P':
		pid_path = optarg;
		break;
	    default:
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
    }

    if (optind >= argc) {
	usage(argv[0]);
	exit(EXIT_FAILURE);
    }
    hid_path = argv[optind];

    if (pid_path) {
	/* open pidfile */
	pidfd = open(pid_path, O_RDWR | O_CREAT, 0644);
	if (pidfd < 0) {
	    perror("open pidfile");
	} else {
	    /* lock it */
	    if (flock(pidfd, LOCK_EX | LOCK_NB) != 0) {
		if (errno == EWOULDBLOCK) {
		    exit(EXIT_SUCCESS);
		} else {
		    perror("lock pidfile");
		}
	    }
	}
    }

    if (daemonize) {
	if (daemon(1, 1) != 0) {
	    perror("daemonize");
	}
    }

    if (pidfd >= 0) {
	/* truncate pidfile at 0 length */
	ftruncate(pidfd, 0);
	dprintf(pidfd, "%u\n", getpid());
	/* unlock the pidfile */
	flock(pidfd, LOCK_UN);
	close(pidfd);
    }

    signal(SIGINT, handle_sigterm);
    signal(SIGTERM, handle_sigterm);

    while (1) {
	int ok = 0;

	hidfd = open(hid_path, O_RDWR);
	if (hidfd < 0 || make_fd_non_blocking(hidfd) < 0) {
	    perror("open hid");
	} else {
	    sfd = create_and_bind_socket(port);
	    if (sfd < 0 || make_fd_non_blocking(sfd) < 0) {
		perror("open socket");
	    } else if (listen(sfd, SOMAXCONN) < 0) {
		perror("listen");
	    } else {
		ok = 1;
	    }
	}

	if (ok) {
	    event_loop(sfd, hidfd);
	}

	if (hidfd >= 0) {
	    close(hidfd);
	}
	if (sfd >= 0) {
	    close(sfd);
	}
	if (g_running) {
	    sleep(10);
	}
	if (!g_running) {
	    break;
	}
    }

    if (pidfd >= 0) {
	unlink(pid_path);
    }

    return 0;
}

