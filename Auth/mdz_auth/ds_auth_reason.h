#ifndef AUTH_REASONS_H
#define AUTH_REASONS_H


namespace Mantids { namespace Authentication {



enum Reason
{
    REASON_AUTHENTICATED=0,             // AUTHENTICATED!
    REASON_INTERNAL_ERROR=500,          // INTERNAL ERROR (OTHER)
    REASON_NOT_IMPLEMENTED=501,         // AUTHENTICATION NOT IMPLEMENTED YET :(
    REASON_DUPLICATED_SESSION=502,      // DUPLICATED SESSION ID
    REASON_EXPIRED_PASSWORD=100,        // VALIDATE, HOWEVER MUST CHANGE PASSWORD NOW!
    REASON_EXPIRED_ACCOUNT=102,         // ACCOUNT EXPIRED. NOT USABLE
    REASON_DISABLED_ACCOUNT=103,        // ACCOUNT DISABLED BY ADMIN.
    REASON_UNCONFIRMED_ACCOUNT=104,     // ACCOUNT NOT CONFIRMED YET.
    REASON_BAD_ACCOUNT=105,             // INVALID OR NON-EXISTENT ACCOUNT
    REASON_BAD_PASSWORD=106,            // AUTHENTICATION FAILED.
    REASON_PASSWORD_INDEX_NOTFOUND=107, // Password Index not found.
    REASON_INVALID_DOMAIN=994,          // AUTHENTICATION FAILED.
    REASON_INVALID_AUTHENTICATOR=995,
    REASON_SESSIONLIMITS_EXCEEDED=996,
    REASON_ANSWER_TIMEDOUT=997,
    REASON_EXPIRED=998,
    REASON_UNAUTHENTICATED=999
};

static bool IS_PASSWORD_AUTHENTICATED(const Reason & reason)
{
    return reason == REASON_AUTHENTICATED || reason == REASON_EXPIRED_PASSWORD;
}

static const char * cREASON_AUTHENTICATED="Authenticated";
static const char * cREASON_INTERNAL_ERROR="Authentication Internal Error";
static const char * cREASON_NOT_IMPLEMENTED="Authentication not implemented yet";
static const char * cREASON_EXPIRED_PASSWORD="Password expired";
static const char * cREASON_EXPIRED_ACCOUNT = "Account expired";
static const char * cREASON_DISABLED_ACCOUNT = "Account disabled";
static const char * cREASON_UNCONFIRMED_ACCOUNT = "Account unconfirmed";
static const char * cREASON_BAD_ACCOUNT = "Invalid Account";
static const char * cREASON_BAD_PASSWORD = "Invalid password";
static const char * cREASON_PASSWORD_INDEX_NOTFOUND = "Password Index Not Found";
static const char * cREASON_EXPIRED = "Expired authentication";
static const char * cREASON_UNAUTHENTICATED = "Not authenticated yet";
static const char * cREASON_ANSWER_TIMEDOUT = "Answer timed out";
static const char * cREASON_DUPLICATED_SESSION = "Session ID Duplicated Error";
static const char * cREASON_INVALID_AUTHENTICATOR = "Invalid or undefined authenticator";
static const char * cREASON_INVALID_DOMAIN = "Invalid domain name";
static const char * cREASON_SESSIONLIMITS_EXCEEDED = "Sessions limits exceeded";
static const char * cNULL = "";

static const char * getReasonText(const Reason & reason)
{
    switch(reason)
    {
    case REASON_SESSIONLIMITS_EXCEEDED: return cREASON_SESSIONLIMITS_EXCEEDED;
    case REASON_INVALID_AUTHENTICATOR: return cREASON_INVALID_AUTHENTICATOR;
    case REASON_DUPLICATED_SESSION: return cREASON_DUPLICATED_SESSION;
    case REASON_AUTHENTICATED: return cREASON_AUTHENTICATED;
    case REASON_INTERNAL_ERROR: return cREASON_INTERNAL_ERROR;
    case REASON_NOT_IMPLEMENTED: return cREASON_NOT_IMPLEMENTED;
    case REASON_EXPIRED_PASSWORD: return cREASON_EXPIRED_PASSWORD;
    case REASON_EXPIRED_ACCOUNT: return cREASON_EXPIRED_ACCOUNT;
    case REASON_DISABLED_ACCOUNT: return cREASON_DISABLED_ACCOUNT;
    case REASON_UNCONFIRMED_ACCOUNT: return cREASON_UNCONFIRMED_ACCOUNT;
    case REASON_BAD_ACCOUNT: return cREASON_BAD_ACCOUNT;
    case REASON_BAD_PASSWORD: return cREASON_BAD_PASSWORD;
    case REASON_PASSWORD_INDEX_NOTFOUND: return cREASON_PASSWORD_INDEX_NOTFOUND;
    case REASON_ANSWER_TIMEDOUT: return cREASON_ANSWER_TIMEDOUT;
    case REASON_EXPIRED: return cREASON_EXPIRED;
    case REASON_UNAUTHENTICATED: return cREASON_UNAUTHENTICATED;
    case REASON_INVALID_DOMAIN: return cREASON_INVALID_DOMAIN;
    }
    return cNULL;
}

}}

#endif // AUTH_REASONS_H
