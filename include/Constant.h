//
// Created by zts on 1/2/18.
//

#ifndef VCDB_CONSTANT_H
#define VCDB_CONSTANT_H


/* Units */
#define UNIT_SECONDS 0
#define UNIT_MILLISECONDS 1



/* Command flags. Please check the command table defined in the redis.c file
 * for more information about the meaning of every flag. */
#define CMD_WRITE (1<<0)            /* "w" flag */
#define CMD_READONLY (1<<1)         /* "r" flag */
#define CMD_DENYOOM (1<<2)          /* "m" flag */
#define CMD_MODULE (1<<3)           /* Command exported by module. */
#define CMD_ADMIN (1<<4)            /* "a" flag */
#define CMD_PUBSUB (1<<5)           /* "p" flag */
#define CMD_NOSCRIPT (1<<6)         /* "s" flag */
#define CMD_RANDOM (1<<7)           /* "R" flag */
#define CMD_SORT_FOR_SCRIPT (1<<8)  /* "S" flag */
#define CMD_LOADING (1<<9)          /* "l" flag */
#define CMD_STALE (1<<10)           /* "t" flag */
#define CMD_SKIP_MONITOR (1<<11)    /* "M" flag */
#define CMD_ASKING (1<<12)          /* "k" flag */
#define CMD_FAST (1<<13)            /* "F" flag */
#define CMD_MODULE_GETKEYS (1<<14)  /* Use the modules getkeys interface. */
#define CMD_MODULE_NO_CLUSTER (1<<15) /* Deny on Redis Cluster. */


#endif //VCDB_CONSTANT_H
