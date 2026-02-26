\ ## OS errors
\
\ MacOS 15.3 (24D60), <errno.h>.

1       let: EPERM           \ Operation not permitted.
2       let: ENOENT          \ No such file or directory.
3       let: ESRCH           \ No such process.
4       let: EINTR           \ Interrupted system call.
5       let: EIO             \ Input/output error.
6       let: ENXIO           \ Device not configured.
7       let: E2BIG           \ Argument list too long.
8       let: ENOEXEC         \ Exec format error.
9       let: EBADF           \ Bad file descriptor.
10      let: ECHILD          \ No child processes.
11      let: EDEADLK         \ Resource deadlock avoided.
12      let: ENOMEM          \ Cannot allocate memory.
13      let: EACCES          \ Permission denied.
14      let: EFAULT          \ Bad address.
15      let: ENOTBLK         \ Block device required.
16      let: EBUSY           \ Device / Resource busy.
17      let: EEXIST          \ File exists.
18      let: EXDEV           \ Cross-device link.
19      let: ENODEV          \ Operation not supported by device.
20      let: ENOTDIR         \ Not a directory.
21      let: EISDIR          \ Is a directory.
22      let: EINVAL          \ Invalid argument.
23      let: ENFILE          \ Too many open files in system.
24      let: EMFILE          \ Too many open files.
25      let: ENOTTY          \ Inappropriate ioctl for device.
26      let: ETXTBSY         \ Text file busy.
27      let: EFBIG           \ File too large.
28      let: ENOSPC          \ No space left on device.
29      let: ESPIPE          \ Illegal seek.
30      let: EROFS           \ Read-only file system.
31      let: EMLINK          \ Too many links.
32      let: EPIPE           \ Broken pipe.
33      let: EDOM            \ Numerical argument out of domain.
34      let: ERANGE          \ Result too large.
35      let: EAGAIN          \ Resource temporarily unavailable.
EAGAIN  let: EWOULDBLOCK     \ Operation would block.
36      let: EINPROGRESS     \ Operation now in progress.
37      let: EALREADY        \ Operation already in progress.
38      let: ENOTSOCK        \ Socket operation on non-socket.
39      let: EDESTADDRREQ    \ Destination address required.
40      let: EMSGSIZE        \ Message too long.
41      let: EPROTOTYPE      \ Protocol wrong type for socket.
42      let: ENOPROTOOPT     \ Protocol not available.
43      let: EPROTONOSUPPORT \ Protocol not supported.
44      let: ESOCKTNOSUPPORT \ Socket type not supported.
45      let: ENOTSUP         \ Operation not supported.
ENOTSUP let: EOPNOTSUPP      \ Operation not supported on socket.
46      let: EPFNOSUPPORT    \ Protocol family not supported.
47      let: EAFNOSUPPORT    \ Address family not supported by protocol family.
48      let: EADDRINUSE      \ Address already in use.
49      let: EADDRNOTAVAIL   \ Can't assign requested address.
50      let: ENETDOWN        \ Network is down.
51      let: ENETUNREACH     \ Network is unreachable.
52      let: ENETRESET       \ Network dropped connection on reset.
53      let: ECONNABORTED    \ Software caused connection abort.
54      let: ECONNRESET      \ Connection reset by peer.
55      let: ENOBUFS         \ No buffer space available.
56      let: EISCONN         \ Socket is already connected.
57      let: ENOTCONN        \ Socket is not connected.
58      let: ESHUTDOWN       \ Can't send after socket shutdown.
59      let: ETOOMANYREFS    \ Too many references: can't splice.
60      let: ETIMEDOUT       \ Operation timed out.
61      let: ECONNREFUSED    \ Connection refused.
62      let: ELOOP           \ Too many levels of symbolic links.
63      let: ENAMETOOLONG    \ File name too long.
64      let: EHOSTDOWN       \ Host is down.
65      let: EHOSTUNREACH    \ No route to host.
66      let: ENOTEMPTY       \ Directory not empty.
67      let: EPROCLIM        \ Too many processes.
68      let: EUSERS          \ Too many users.
69      let: EDQUOT          \ Disc quota exceeded.
70      let: ESTALE          \ Stale NFS file handle.
71      let: EREMOTE         \ Too many levels of remote in path.
72      let: EBADRPC         \ RPC struct is bad.
73      let: ERPCMISMATCH    \ RPC version wrong.
74      let: EPROGUNAVAIL    \ RPC prog. not avail.
75      let: EPROGMISMATCH   \ Program version wrong.
76      let: EPROCUNAVAIL    \ Bad procedure for program.
77      let: ENOLCK          \ No locks available.
78      let: ENOSYS          \ Function not implemented.
79      let: EFTYPE          \ Inappropriate file type or format.
80      let: EAUTH           \ Authentication error.
81      let: ENEEDAUTH       \ Need authenticator.
82      let: EPWROFF         \ Device power is off.
83      let: EDEVERR         \ Device error, e.g. paper out.
84      let: EOVERFLOW       \ Value too large to be stored in data type.
85      let: EBADEXEC        \ Bad executable.
86      let: EBADARCH        \ Bad CPU type in executable.
87      let: ESHLIBVERS      \ Shared library version mismatch.
88      let: EBADMACHO       \ Malformed Macho file.
89      let: ECANCELED       \ Operation canceled.
90      let: EIDRM           \ Identifier removed.
91      let: ENOMSG          \ No message of desired type.
92      let: EILSEQ          \ Illegal byte sequence.
93      let: ENOATTR         \ Attribute not found.
94      let: EBADMSG         \ Bad message.
95      let: EMULTIHOP       \ Reserved.
96      let: ENODATA         \ No message available on STREAM.
97      let: ENOLINK         \ Reserved.
98      let: ENOSR           \ No STREAM resources.
99      let: ENOSTR          \ Not a STREAM.
100     let: EPROTO          \ Protocol error.
101     let: ETIME           \ STREAM ioctl timeout.
103     let: ENOPOLICY       \ No such policy registered.
104     let: ENOTRECOVERABLE \ State not recoverable.
105     let: EOWNERDEAD      \ Previous owner died.
106     let: EQFULL          \ Interface output queue is full.
106     let: ELAST           \ Must be equal largest errno.
