#ifndef EV_ERRORS_
#define EV_ERRORS_

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

enum class ErrorType : uint8_t {
  NO_ERR = 0,
  EVENT_MANAGER_ERR,
  LIBURING_SUBMISSION_ERR_ERRNO,
  OPERATION_ERR_ERRNO
};

enum class EventManagerErrors : uint8_t {
  UNKNOWN_ERROR = 0,
  SUBMISSION_QUEUE_FULL = 1,
  SYSTEM_COMMUNICATION_CHANNEL_FAILURE = 2
};

enum class Errnos : uint8_t {
  /*
   * In this context assume there is an error, but we don't
   * know what it is so it's ok since we cannot do anything
   */
  UNKNOWN_ERROR = 0,

  ERR_PERM = 1,         /* Operation not permitted */
  ERR_NOENT = 2,        /* No such file or directory */
  ERR_SRCH = 3,         /* No such process */
  ERR_INTR = 4,         /* Interrupted system call */
  ERR_IO = 5,           /* I/O error */
  ERR_NXIO = 6,         /* No such device or address */
  ERR_2BIG = 7,         /* Argument list too long */
  ERR_NOEXEC = 8,       /* Exec format error */
  ERR_BADF = 9,         /* Bad file number */
  ERR_CHILD = 10,       /* No child processes */
  ERR_AGAIN = 11,       /* Try again */
  ERR_NOMEM = 12,       /* Out of memory */
  ERR_ACCES = 13,       /* Permission denied */
  ERR_FAULT = 14,       /* Bad address */
  ERR_NOTBLK = 15,      /* Block device required */
  ERR_BUSY = 16,        /* Device or resource busy */
  ERR_EXIST = 17,       /* File exists */
  ERR_XDEV = 18,        /* Cross-device link */
  ERR_NODEV = 19,       /* No such device */
  ERR_NOTDIR = 20,      /* Not a directory */
  ERR_ISDIR = 21,       /* Is a directory */
  ERR_INVAL = 22,       /* Invalid argument */
  ERR_NFILE = 23,       /* File table overflow */
  ERR_MFILE = 24,       /* Too many open files */
  ERR_NOTTY = 25,       /* Not a typewriter */
  ERR_TXTBSY = 26,      /* Text file busy */
  ERR_FBIG = 27,        /* File too large */
  ERR_NOSPC = 28,       /* No space left on device */
  ERR_SPIPE = 29,       /* Illegal seek */
  ERR_ROFS = 30,        /* Read-only file system */
  ERR_MLINK = 31,       /* Too many links */
  ERR_PIPE = 32,        /* Broken pipe */
  ERR_DOM = 33,         /* Math argument out of domain of func */
  ERR_RANGE = 34,       /* Math result not representable */
  ERR_DEADLK = 35,      /* Resource deadlock would occur */
  ERR_NAMETOOLONG = 36, /* File name too long */
  ERR_NOLCK = 37,       /* No record locks available */

  /*
   * This error code is special: arch syscall entry code will return
   * -ENOSYS if users try to call a syscall that doesn't exist. To keep
   * failures of syscalls that really do exist distinguishable from
   * failures due to attempts to use a nonexistent syscall, syscall
   * implementations should refrain from returning -ENOSYS.
   */
  ERR_NOSYS = 38, /* Invalid system call number */

  ERR_NOTEMPTY = 39,       /* Directory not empty */
  ERR_LOOP = 40,           /* Too many symbolic links encountered */
  ERR_WOULDBLOCK = EAGAIN, /* Operation would block */
  ERR_NOMSG = 42,          /* No message of desired type */
  ERR_IDRM = 43,           /* Identifier removed */
  ERR_CHRNG = 44,          /* Channel number out of range */
  ERR_L2NSYNC = 45,        /* Level 2 not synchronized */
  ERR_L3HLT = 46,          /* Level 3 halted */
  ERR_L3RST = 47,          /* Level 3 reset */
  ERR_LNRNG = 48,          /* Link number out of range */
  ERR_UNATCH = 49,         /* Protocol driver not attached */
  ERR_NOCSI = 50,          /* No CSI structure available */
  ERR_L2HLT = 51,          /* Level 2 halted */
  ERR_BADE = 52,           /* Invalid exchange */
  ERR_BADR = 53,           /* Invalid request descriptor */
  ERR_XFULL = 54,          /* Exchange full */
  ERR_NOANO = 55,          /* No anode */
  ERR_BADRQC = 56,         /* Invalid request code */
  ERR_BADSLT = 57,         /* Invalid slot */

  ERR_DEADLOCK = EDEADLK,

  ERR_BFONT = 59,          /* Bad font file format */
  ERR_NOSTR = 60,          /* Device not a stream */
  ERR_NODATA = 61,         /* No data available */
  ERR_TIME = 62,           /* Timer expired */
  ERR_NOSR = 63,           /* Out of streams resources */
  ERR_NONET = 64,          /* Machine is not on the network */
  ERR_NOPKG = 65,          /* Package not installed */
  ERR_REMOTE = 66,         /* Object is remote */
  ERR_NOLINK = 67,         /* Link has been severed */
  ERR_ADV = 68,            /* Advertise error */
  ERR_SRMNT = 69,          /* Srmount error */
  ERR_COMM = 70,           /* Communication error on send */
  ERR_PROTO = 71,          /* Protocol error */
  ERR_MULTIHOP = 72,       /* Multihop attempted */
  ERR_DOTDOT = 73,         /* RFS specific error */
  ERR_BADMSG = 74,         /* Not a data message */
  ERR_OVERFLOW = 75,       /* Value too large for defined data type */
  ERR_NOTUNIQ = 76,        /* Name not unique on network */
  ERR_BADFD = 77,          /* File descriptor in bad state */
  ERR_REMCHG = 78,         /* Remote address changed */
  ERR_LIBACC = 79,         /* Can not access a needed shared library */
  ERR_LIBBAD = 80,         /* Accessing a corrupted shared library */
  ERR_LIBSCN = 81,         /* .lib section in a.out corrupted */
  ERR_LIBMAX = 82,         /* Attempting to link in too many shared libraries */
  ERR_LIBEXEC = 83,        /* Cannot exec a shared library directly */
  ERR_ILSEQ = 84,          /* Illegal byte sequence */
  ERR_RESTART = 85,        /* Interrupted system call should be restarted */
  ERR_STRPIPE = 86,        /* Streams pipe error */
  ERR_USERS = 87,          /* Too many users */
  ERR_NOTSOCK = 88,        /* Socket operation on non-socket */
  ERR_DESTADDRREQ = 89,    /* Destination address required */
  ERR_MSGSIZE = 90,        /* Message too long */
  ERR_PROTOTYPE = 91,      /* Protocol wrong type for socket */
  ERR_NOPROTOOPT = 92,     /* Protocol not available */
  ERR_PROTONOSUPPORT = 93, /* Protocol not supported */
  ERR_SOCKTNOSUPPORT = 94, /* Socket type not supported */
  ERR_OPNOTSUPP = 95,      /* Operation not supported on transport endpoint */
  ERR_PFNOSUPPORT = 96,    /* Protocol family not supported */
  ERR_AFNOSUPPORT = 97,    /* Address family not supported by protocol */
  ERR_ADDRINUSE = 98,      /* Address already in use */
  ERR_ADDRNOTAVAIL = 99,   /* Cannot assign requested address */
  ERR_NETDOWN = 100,       /* Network is down */
  ERR_NETUNREACH = 101,    /* Network is unreachable */
  ERR_NETRESET = 102,      /* Network dropped connection because of reset */
  ERR_CONNABORTED = 103,   /* Software caused connection abort */
  ERR_CONNRESET = 104,     /* Connection reset by peer */
  ERR_NOBUFS = 105,        /* No buffer space available */
  ERR_ISCONN = 106,        /* Transport endpoint is already connected */
  ERR_NOTCONN = 107,       /* Transport endpoint is not connected */
  ERR_SHUTDOWN = 108,      /* Cannot send after transport endpoint shutdown */
  ERR_TOOMANYREFS = 109,   /* Too many references: cannot splice */
  ERR_TIMEDOUT = 110,      /* Connection timed out */
  ERR_CONNREFUSED = 111,   /* Connection refused */
  ERR_HOSTDOWN = 112,      /* Host is down */
  ERR_HOSTUNREACH = 113,   /* No route to host */
  ERR_ALREADY = 114,       /* Operation already in progress */
  ERR_INPROGRESS = 115,    /* Operation now in progress */
  ERR_STALE = 116,         /* Stale file handle */
  ERR_UCLEAN = 117,        /* Structure needs cleaning */
  ERR_NOTNAM = 118,        /* Not a XENIX named type file */
  ERR_NAVAIL = 119,        /* No XENIX semaphores available */
  ERR_ISNAM = 120,         /* Is a named type file */
  ERR_REMOTEIO = 121,      /* Remote I/O error */
  ERR_DQUOT = 122,         /* Quota exceeded */

  ERR_NOMEDIUM = 123,    /* No medium found */
  ERR_MEDIUMTYPE = 124,  /* Wrong medium type */
  ERR_CANCELED = 125,    /* Operation Canceled */
  ERR_NOKEY = 126,       /* Required key not available */
  ERR_KEYEXPIRED = 127,  /* Key has expired */
  ERR_KEYREVOKED = 128,  /* Key has been revoked */
  ERR_KEYREJECTED = 129, /* Key was rejected by service */

  /* for robust mutexes */
  ERR_OWNERDEAD = 130,      /* Owner died */
  ERR_NOTRECOVERABLE = 131, /* State not recoverable */

  ERR_RFKILL = 132, /* Operation not possible due to RF-kill */

  ERR_HWPOISON = 133, /* Memory page has hardware error */
};

template <ErrorType> struct ErrorTypes;

template <> struct ErrorTypes<ErrorType::NO_ERR> { using type = std::nullptr_t; };

template <> struct ErrorTypes<ErrorType::EVENT_MANAGER_ERR> { using type = EventManagerErrors; };

template <> struct ErrorTypes<ErrorType::LIBURING_SUBMISSION_ERR_ERRNO> { using type = Errnos; };

template <> struct ErrorTypes<ErrorType::OPERATION_ERR_ERRNO> { using type = Errnos; };

template <ErrorType Et> using ErrorTypeMap = typename ErrorTypes<Et>::type;

using ErrorCodes = std::variant<std::nullptr_t, EventManagerErrors, Errnos, Errnos>;

namespace ErrorProcessing {

bool is_there_an_error(const ErrorCodes &error) {
  return static_cast<ErrorType>(error.index()) != ErrorType::NO_ERR;
}

template <ErrorType Et, typename SetErrorType = ErrorTypeMap<Et>>
ErrorCodes set_error_from_enum(ErrorCodes error, SetErrorType error_val) {
  constexpr size_t INDEX = static_cast<size_t>(Et);
  error.emplace<INDEX>(error_val);
  return error;
}

template <ErrorType Et, typename SetErrorType = ErrorTypeMap<Et>>
ErrorCodes set_error_from_num(ErrorCodes error, int error_num) {
  constexpr size_t INDEX = static_cast<size_t>(Et);
  error.emplace<INDEX>(static_cast<SetErrorType>(error_num));
  return error;
}

ErrorType get_error_type(ErrorCodes &error) { return static_cast<ErrorType>(error.index()); }

template <ErrorType Et, typename SetErrorType = ErrorTypeMap<Et>>
std::optional<SetErrorType> get_contained_error_code(const ErrorCodes &error) {
  constexpr size_t INDEX = static_cast<size_t>(Et);
  if (auto data = std::get_if<INDEX>(&error)) {
    return *data;
  }
  return std::nullopt;
}

} // namespace ErrorProcessing
#endif
