

USING IMPORT Std.Io

TYPE Each_type = STRUCT
    Long l
    Int i
    Short h
    Sbyte c
    Ulong ul
    Uint ui
    Ushort uh
    Byte uc
END

Each_type each = {-4, -3, -2, -1, 1, 2, 3, 4}

Ulong one_ul = 1

IF each.l != -4 DO
    print("FAIL")
ELSE IF each.i != -3 DO
    print("FAIL") 
ELSE IF each.h != -2 DO
    print("FAIL")
ELSE IF each.c != -1 DO
    print("FAIL")
ELSE IF each.ul != one_ul DO /* No support for Ulong integer literals yet ... */
    print("FAIL")
ELSE IF each.ui != 2 DO
    print("FAIL")
ELSE IF each.uh != 3 DO
    print("FAIL")
ELSE IF each.uc != 4 DO
    print("FAIL")
END

FUNC struct_as_parameter(Each_type e) DO
    IF e.l != -4 DO
        print("FAIL")
    ELSE IF e.i != -3 DO
        print("FAIL") 
    ELSE IF e.h != -2 DO
        print("FAIL")
    ELSE IF e.c != -1 DO
        print("FAIL")
    ELSE IF e.ul != one_ul DO /* No support for Ulong integer literals yet ... */
        print("FAIL")
    ELSE IF e.ui != 2 DO
        print("FAIL")
    ELSE IF e.uh != 3 DO
        print("FAIL")
    ELSE IF e.uc != 4 DO
        print("FAIL")
    END
END
struct_as_parameter(each)

FUNC struct_ptr_as_parameter(&Each_type e) DO
    IF @e.l != -4 DO
        print("FAIL")
    ELSE IF @e.i != -3 DO
        print("FAIL") 
    ELSE IF @e.h != -2 DO
        print("FAIL")
    ELSE IF @e.c != -1 DO
        print("FAIL")
    ELSE IF @e.ul != one_ul DO /* No support for Ulong integer literals yet ... */
        print("FAIL")
    ELSE IF @e.ui != 2 DO
        print("FAIL")
    ELSE IF @e.uh != 3 DO
        print("FAIL")
    ELSE IF @e.uc != 4 DO
        print("FAIL")
    END
END
struct_ptr_as_parameter(&each)


print("DONE")
    