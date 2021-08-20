

USING IMPORT Std.Io

FUNC Long i = return_long_1() DO
    Long i = 1
    RETURN i
END
FUNC Int i = return_int_1() DO
    Int i = 1
    RETURN i
END
FUNC Short i = return_short_1() DO
    Short i = 1
    RETURN i
END
FUNC Sbyte i = return_sbyte_1() DO
    Sbyte i = 1
    RETURN i
END
FUNC Ulong i = return_ulong_1() DO
    Ulong i = 1
    RETURN i
END
FUNC Uint i = return_uint_1() DO
    Uint i = 1
    RETURN i
END
FUNC Ushort i = return_ushort_1() DO
    Ushort i = 1
    RETURN i
END
FUNC Byte i = return_byte_1() DO
    Byte i = 1
    RETURN i
END
FUNC Double i = return_double_1() DO
    Byte i = 1
    RETURN i
END
FUNC Float i = return_float_1() DO
    Float i = 1
    RETURN i
END

Ulong ul_one = 1
IF return_long_1() != 1 DO
    print("FAIL")
END
IF return_int_1() != 1 DO
    print("FAIL")
END
IF return_short_1() != 1 DO
    print("FAIL")
END
IF return_sbyte_1() != 1 DO
    print("FAIL")
END
IF return_ulong_1() != ul_one DO
    print("FAIL")
END
IF return_uint_1() != 1 DO
    print("FAIL")
END
IF return_ushort_1() != 1 DO
    print("FAIL")
END
IF return_byte_1() != 1 DO
    print("FAIL")
END
IF return_double_1() != 1 DO
    print("FAIL")
END
IF return_float_1() != 1 DO
    print("FAIL")
END

print("DONE")

