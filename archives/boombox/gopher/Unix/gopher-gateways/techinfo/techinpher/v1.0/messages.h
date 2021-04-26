/*
 * messages.h
 */

/* This file gives names for the indices of the server's messages.
   The old way was to hard code the indices themselves. */

#define OK                      0
/* The 3 messages NOT_DOCUMENT, SENDFILE_* have no error code 
   compatibility with a bug in nio_send_file() */

#define NOT_AUTH                1
#define BAD_PASSWD              2
#define OTHER_PROV              3
#define DEL_CHILD_1ST           4
#define CANT_REORDER_NODES      5
#define CANT_OPEN               6
#define NOT_DOCUMENT            7
#define CANT_WRITE_WEB          8
#define CANT_FIND_NODE          9
#define BAD_PATH                10
#define ITEM_EXISTS             11
#define CANT_FIND_SOURCE        12
#define HUH                     13
#define VERSION_NOT_FOUND       14
#define CANT_FIND_SERVER_FILE   15
#define SENDFILE_CANT_ACCESS    16
#define SENDFILE_IS_DIRECTORY   17
#define DEST_IS_DIRECTORY       18
#define NODE_IS_PUBLIC          19
#define UNKNOWN_FORMAT          20
#define FUNCTION_DISABLED       21
#define TEXT_SEARCH_FAILED      22
#define MORE                    23
