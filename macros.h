#ifndef MACROS_H
#define MACROS_H

#include <string>

const std::string HTTP_VERSION = "HTTP/1.1";
const std::string SERVE_DIR = "example_dir";

#define CR      '\r'
#define CRCR    "\r\r"
#define CRLF    "\r\n"
#define LF      '\n'
#define LFLF    "\n\n"

/* RFC7231 standard status codes
 * Reference:
 * https://tools.ietf.org/html/rfc7231#section-8.1 [p48]
 */
#define STATUS_CONTINUE                        100
#define STATUS_SWITCHING_PROTOCOLS             101
#define STATUS_OK                              200
#define STATUS_CREATED                         201
#define STATUS_ACCEPTED                        202
#define STATUS_NONAUTHORITATIVE_INFORMATION    203
#define STATUS_NO_CONTENT                      204
#define STATUS_RESET_CONTENT                   205
#define STATUS_PARTIAL_CONTENT                 206
#define STATUS_MULTIPLE_CHOICES                300
#define STATUS_MOVED_PERMANENTLY               301
#define STATUS_FOUND                           302
#define STATUS_SEE_OTHER                       303
#define STATUS_NOT_MODIFIED                    304
#define STATUS_USE_PROXY                       305
#define STATUS_TEMPORARY_REDIRECT              307
#define STATUS_BAD_REQUEST                     400
#define STATUS_UNAUTHORIZED                    401
#define STATUS_PAYMENT_REQUIRED                402
#define STATUS_FORBIDDEN                       403
#define STATUS_NOT_FOUND                       404
#define STATUS_METHOD_NOT_ALLOWED              405
#define STATUS_NOT_ACCEPTABLE                  406
#define STATUS_PROXY_AUTHENTICATION_REQUIRED   407
#define STATUS_REQUEST_TIMEOUT                 408
#define STATUS_CONFLICT                        409
#define STATUS_GONE                            410
#define STATUS_LENGTH_REQUIRED                 411
#define STATUS_PRECONDITION_FAILED             412
#define STATUS_REQUEST_ENTITY_TOO_LARGE        413
#define STATUS_REQUESTURI_TOO_LARGE            414
#define STATUS_UNSUPPORTED_MEDIA_TYPE          415
#define STATUS_REQUESTED_RANGE_NOT_SATISFIABLE 416
#define STATUS_EXPECTATION_FAILED              417
#define STATUS_INTERNAL_SERVER_ERROR           500
#define STATUS_NOT_IMPLEMENTED                 501
#define STATUS_BAD_GATEWAY                     502
#define STATUS_SERVICE_UNAVAILABLE             503
#define STATUS_GATEWAY_TIMEOUT                 504
#define STATUS_HTTP_VERSION_NOT_SUPPORTED      505

#define DEBUG_ERR(x) std::cout << "[ERROR] " << x << std::endl
#define TO_BE_IMPL(x) std::cout << "[ERROR] " << x << \
    " is not implemented yet!" << x << std::endl

#endif
