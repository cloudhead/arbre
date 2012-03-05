/*
 * arbre
 *
 * (c) 2011-2012, Alexis Sellier
 *
 * value.h
 *
 */

/*
 * TValue data-types
 */
typedef enum {
    TYPE_ATOM,
    TYPE_BIN,
    TYPE_TUPLE,
    TYPE_STRING,
    TYPE_NUMBER,
    TYPE_PATH
} TYPE;

/*
 * TValue data-type qualifiers
 */
typedef enum {
    Q_NIL      =  0,
    Q_PTR      =  1 << 4,                  /* Pointer           0001 0000 */
    Q_ARY      =  1 << 5,                  /* Array             0010 0000 */
    Q_RSTREAM  =  1 << 6,                  /* Read Stream       0100 0000 */
    Q_WSTREAM  =  1 << 7,                  /* Write Stream      1000 0000 */
    Q_RWSTREAM =  Q_RSTREAM | Q_WSTREAM,   /* Read/Write Stream 1100 0000 */
} QUAL;

#define  TYPE_QUAL_MASK  0xf0
#define  TYPE_MASK       0x0f

typedef struct {
    int  length;
    char value[];
} String;

typedef union {
    struct TValue  *tval;
    bool            boolean;
    int             number;
    const char     *atom;
    String         *string;
    struct {
        const char *module;
        const char *path;
    } uri;
} Value;

struct TValue {
    TYPE    t;
    Value   v;
};

struct TValueList {
    struct TValue     *head;
    struct TValueList *tail;
};

typedef struct TValueList TValueList;
typedef struct TValue     TValue;

TValue *tvalue(TYPE type, Value val);
