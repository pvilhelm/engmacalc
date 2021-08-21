/* 
    Verify that compare operators work as supposed.
*/

USING IMPORT Std.Io

/* Generated with Python 3.6:

l = [["long", "Long"],
     ["int", "Int"],
     ["short", "Short"],
     ["sbyte", "Sbyte"],
     ["ulong", "Ulong"],
     ["uint", "Uint"],
     ["ushort", "Ushort"],
     ["byte", "Byte"]]

op = [["eq","=="],
      ["ge", ">="],
      ["le", "<="],
      ["g", ">"],
      ["l", "<"],
      ["ne", "!="]]

for o in op:
    op_name = o[0];
    op_sym = o[1];
    for e in l:
        type = e[1]
        name = e[0]

        print("""
FUNC {}_{}({} i1, {} i2) DO
    IF i1 {} i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END""".format(op_name, name + "s", type,type, op_sym))

for o in op:
    op_name = o[0];
    op_sym = o[1];
    for e in l:
        type = e[1]
        name = e[0]

        print("{}_{}({}1, {}2)".format(op_name, name + "s", name, name))
        print("{}_{}({}2, {}2)".format(op_name, name + "s", name, name))
        print("{}_{}({}2, {}1)".format(op_name, name + "s", name, name))
*/


FUNC eq_longs(Long i1, Long i2) DO
    IF i1 == i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC eq_ints(Int i1, Int i2) DO
    IF i1 == i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC eq_shorts(Short i1, Short i2) DO
    IF i1 == i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC eq_sbytes(Sbyte i1, Sbyte i2) DO
    IF i1 == i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC eq_ulongs(Ulong i1, Ulong i2) DO
    IF i1 == i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC eq_uints(Uint i1, Uint i2) DO
    IF i1 == i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC eq_ushorts(Ushort i1, Ushort i2) DO
    IF i1 == i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC eq_bytes(Byte i1, Byte i2) DO
    IF i1 == i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ge_longs(Long i1, Long i2) DO
    IF i1 >= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ge_ints(Int i1, Int i2) DO
    IF i1 >= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ge_shorts(Short i1, Short i2) DO
    IF i1 >= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ge_sbytes(Sbyte i1, Sbyte i2) DO
    IF i1 >= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ge_ulongs(Ulong i1, Ulong i2) DO
    IF i1 >= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ge_uints(Uint i1, Uint i2) DO
    IF i1 >= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ge_ushorts(Ushort i1, Ushort i2) DO
    IF i1 >= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ge_bytes(Byte i1, Byte i2) DO
    IF i1 >= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC le_longs(Long i1, Long i2) DO
    IF i1 <= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC le_ints(Int i1, Int i2) DO
    IF i1 <= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC le_shorts(Short i1, Short i2) DO
    IF i1 <= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC le_sbytes(Sbyte i1, Sbyte i2) DO
    IF i1 <= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC le_ulongs(Ulong i1, Ulong i2) DO
    IF i1 <= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC le_uints(Uint i1, Uint i2) DO
    IF i1 <= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC le_ushorts(Ushort i1, Ushort i2) DO
    IF i1 <= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC le_bytes(Byte i1, Byte i2) DO
    IF i1 <= i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC g_longs(Long i1, Long i2) DO
    IF i1 > i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC g_ints(Int i1, Int i2) DO
    IF i1 > i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC g_shorts(Short i1, Short i2) DO
    IF i1 > i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC g_sbytes(Sbyte i1, Sbyte i2) DO
    IF i1 > i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC g_ulongs(Ulong i1, Ulong i2) DO
    IF i1 > i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC g_uints(Uint i1, Uint i2) DO
    IF i1 > i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC g_ushorts(Ushort i1, Ushort i2) DO
    IF i1 > i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC g_bytes(Byte i1, Byte i2) DO
    IF i1 > i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC l_longs(Long i1, Long i2) DO
    IF i1 < i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC l_ints(Int i1, Int i2) DO
    IF i1 < i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC l_shorts(Short i1, Short i2) DO
    IF i1 < i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC l_sbytes(Sbyte i1, Sbyte i2) DO
    IF i1 < i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC l_ulongs(Ulong i1, Ulong i2) DO
    IF i1 < i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC l_uints(Uint i1, Uint i2) DO
    IF i1 < i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC l_ushorts(Ushort i1, Ushort i2) DO
    IF i1 < i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC l_bytes(Byte i1, Byte i2) DO
    IF i1 < i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ne_longs(Long i1, Long i2) DO
    IF i1 != i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ne_ints(Int i1, Int i2) DO
    IF i1 != i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ne_shorts(Short i1, Short i2) DO
    IF i1 != i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ne_sbytes(Sbyte i1, Sbyte i2) DO
    IF i1 != i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ne_ulongs(Ulong i1, Ulong i2) DO
    IF i1 != i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ne_uints(Uint i1, Uint i2) DO
    IF i1 != i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ne_ushorts(Ushort i1, Ushort i2) DO
    IF i1 != i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END

FUNC ne_bytes(Byte i1, Byte i2) DO
    IF i1 != i2 DO
        println("true")
    ELSE DO
        println("false")
    END
END


Int int2 = 2
Int int1 = 1
Long long1 = 1
Long long2 = 2
Short short1 = 1
Short short2 = 2
Sbyte sbyte1 = 1
Sbyte sbyte2 = 2

Ulong ulong1 = 1
Ulong ulong2 = 2
Uint uint1 = 1
Uint uint2 = 2
Ushort ushort1 = 1
Ushort ushort2 = 2
Byte byte1 = 1
Byte byte2 = 2

/* Test some integer literals */
eq_ints(2,2)                           /* true */
eq_ints(2,3)                           /* false */
eq_longs(long1, long2)                 /* false */
eq_longs(long2, long2)                 /* true */
eq_longs(long2, long1)                 /* false */
eq_ints(int1, int2)                    /* false */
eq_ints(int2, int2)                    /* true */
eq_ints(int2, int1)                    /* false */
eq_shorts(short1, short2)              /* false */
eq_shorts(short2, short2)              /* true */
eq_shorts(short2, short1)              /* false */
eq_sbytes(sbyte1, sbyte2)              /* false */
eq_sbytes(sbyte2, sbyte2)              /* true */
eq_sbytes(sbyte2, sbyte1)              /* false */
eq_ulongs(ulong1, ulong2)              /* false */
eq_ulongs(ulong2, ulong2)              /* true */
eq_ulongs(ulong2, ulong1)              /* false */
eq_uints(uint1, uint2)                 /* false */
eq_uints(uint2, uint2)                 /* true */
eq_uints(uint2, uint1)                 /* false */
eq_ushorts(ushort1, ushort2)           /* false */
eq_ushorts(ushort2, ushort2)           /* true */
eq_ushorts(ushort2, ushort1)           /* false */
eq_bytes(byte1, byte2)                 /* false */
eq_bytes(byte2, byte2)                 /* true */
eq_bytes(byte2, byte1)                 /* false */
ge_longs(long1, long2)                 /* false */
ge_longs(long2, long2)                 /* true */
ge_longs(long2, long1)                 /* true */
ge_ints(int1, int2)                    /* false */
ge_ints(int2, int2)                    /* true */
ge_ints(int2, int1)                    /* true */
ge_shorts(short1, short2)              /* false */
ge_shorts(short2, short2)              /* true */
ge_shorts(short2, short1)              /* true */
ge_sbytes(sbyte1, sbyte2)              /* false */
ge_sbytes(sbyte2, sbyte2)              /* true */
ge_sbytes(sbyte2, sbyte1)              /* true */
ge_ulongs(ulong1, ulong2)              /* false */
ge_ulongs(ulong2, ulong2)              /* true */
ge_ulongs(ulong2, ulong1)              /* true */
ge_uints(uint1, uint2)                 /* false */
ge_uints(uint2, uint2)                 /* true */
ge_uints(uint2, uint1)                 /* true */
ge_ushorts(ushort1, ushort2)           /* false */
ge_ushorts(ushort2, ushort2)           /* true */
ge_ushorts(ushort2, ushort1)           /* true */
ge_bytes(byte1, byte2)                 /* false */
ge_bytes(byte2, byte2)                 /* true */
ge_bytes(byte2, byte1)                 /* true */
le_longs(long1, long2)                 /* true */
le_longs(long2, long2)                 /* true */
le_longs(long2, long1)                 /* false */
le_ints(int1, int2)                    /* true */
le_ints(int2, int2)                    /* true */
le_ints(int2, int1)                    /* false */
le_shorts(short1, short2)              /* true */
le_shorts(short2, short2)              /* true */
le_shorts(short2, short1)              /* false */
le_sbytes(sbyte1, sbyte2)              /* true */
le_sbytes(sbyte2, sbyte2)              /* true */
le_sbytes(sbyte2, sbyte1)              /* false */
le_ulongs(ulong1, ulong2)              /* true */
le_ulongs(ulong2, ulong2)              /* true */
le_ulongs(ulong2, ulong1)              /* false */
le_uints(uint1, uint2)                 /* true */
le_uints(uint2, uint2)                 /* true */
le_uints(uint2, uint1)                 /* false */
le_ushorts(ushort1, ushort2)           /* true */
le_ushorts(ushort2, ushort2)           /* true */
le_ushorts(ushort2, ushort1)           /* false */
le_bytes(byte1, byte2)                 /* true */
le_bytes(byte2, byte2)                 /* true */
le_bytes(byte2, byte1)                 /* false */
g_longs(long1, long2)                  /* false */
g_longs(long2, long2)                  /* false */
g_longs(long2, long1)                  /* true */
g_ints(int1, int2)                     /* false */
g_ints(int2, int2)                     /* false */
g_ints(int2, int1)                     /* true */
g_shorts(short1, short2)               /* false */
g_shorts(short2, short2)               /* false */
g_shorts(short2, short1)               /* true */
g_sbytes(sbyte1, sbyte2)               /* false */
g_sbytes(sbyte2, sbyte2)               /* false */
g_sbytes(sbyte2, sbyte1)               /* true */
g_ulongs(ulong1, ulong2)               /* false */
g_ulongs(ulong2, ulong2)               /* false */
g_ulongs(ulong2, ulong1)               /* true */
g_uints(uint1, uint2)                  /* false */
g_uints(uint2, uint2)                  /* false */
g_uints(uint2, uint1)                  /* true */
g_ushorts(ushort1, ushort2)            /* false */
g_ushorts(ushort2, ushort2)            /* false */
g_ushorts(ushort2, ushort1)            /* true */
g_bytes(byte1, byte2)                  /* false */
g_bytes(byte2, byte2)                  /* false */
g_bytes(byte2, byte1)                  /* true */
l_longs(long1, long2)                  /* true */
l_longs(long2, long2)                  /* false */
l_longs(long2, long1)                  /* false */
l_ints(int1, int2)                     /* true */
l_ints(int2, int2)                     /* false */
l_ints(int2, int1)                     /* false */
l_shorts(short1, short2)               /* true */
l_shorts(short2, short2)               /* false */
l_shorts(short2, short1)               /* false */
l_sbytes(sbyte1, sbyte2)               /* true */
l_sbytes(sbyte2, sbyte2)               /* false */
l_sbytes(sbyte2, sbyte1)               /* false */
l_ulongs(ulong1, ulong2)               /* true */
l_ulongs(ulong2, ulong2)               /* false */
l_ulongs(ulong2, ulong1)               /* false */
l_uints(uint1, uint2)                  /* true */
l_uints(uint2, uint2)                  /* false */
l_uints(uint2, uint1)                  /* false */
l_ushorts(ushort1, ushort2)            /* true */
l_ushorts(ushort2, ushort2)            /* false */
l_ushorts(ushort2, ushort1)            /* false */
l_bytes(byte1, byte2)                  /* true */
l_bytes(byte2, byte2)                  /* false */
l_bytes(byte2, byte1)                  /* false */
ne_longs(long1, long2)                 /* true */
ne_longs(long2, long2)                 /* false */
ne_longs(long2, long1)                 /* true */
ne_ints(int1, int2)                    /* true */
ne_ints(int2, int2)                    /* false */
ne_ints(int2, int1)                    /* true */
ne_shorts(short1, short2)              /* true */
ne_shorts(short2, short2)              /* false */
ne_shorts(short2, short1)              /* true */
ne_sbytes(sbyte1, sbyte2)              /* true */
ne_sbytes(sbyte2, sbyte2)              /* false */
ne_sbytes(sbyte2, sbyte1)              /* true */
ne_ulongs(ulong1, ulong2)              /* true */
ne_ulongs(ulong2, ulong2)              /* false */
ne_ulongs(ulong2, ulong1)              /* true */
ne_uints(uint1, uint2)                 /* true */
ne_uints(uint2, uint2)                 /* false */
ne_uints(uint2, uint1)                 /* true */
ne_ushorts(ushort1, ushort2)           /* true */
ne_ushorts(ushort2, ushort2)           /* false */
ne_ushorts(ushort2, ushort1)           /* true */
ne_bytes(byte1, byte2)                 /* true */
ne_bytes(byte2, byte2)                 /* false */
ne_bytes(byte2, byte1)                 /* true */





