
USING IMPORT Std.Io

DO
    Double d = 2 + 3
    Float f = 2 + 3

    Long l = 2 + 3
    Int i = 2 + 3
    Short h = 2 + 3
    Sbyte c = 2 + 3

    Ulong ul_five = 5
    Ulong ul = 2 + 3
    Uint ui = 2 + 3
    Ushort uh = 2 + 3
    Byte uc = 2 + 3

    IF d != 5 DO
        print("FAIL")
    END
    IF f != 5 DO
        print("FAIL")
    END
    IF l != 5 DO
        print("FAIL")
    END
    IF i != 5 DO
        print("FAIL")
    END
    IF h != 5 DO
        print("FAIL")
    END
    IF c != 5 DO
        print("FAIL")
    END
    IF ul != ul_five DO
        print("FAIL")
    END
    IF ui != 5 DO
        print("FAIL")
    END
    IF uh != 5 DO
        print("FAIL")
    END
    IF uc != 5 DO
        print("FAIL")
    END
END

DO
    Double d = -2 + 3
    Float f = -2 + 3

    Long l = -2 + 3
    Int i = -2 + 3
    Short h = -2 + 3
    Sbyte c = -2 + 3

    Ulong ul_one = 1
    Ulong ul = -2 + 3
    Uint ui = -2 + 3
    Ushort uh = -2 + 3
    Byte uc = -2 + 3

    IF d != 1 DO
        print("FAIL")
    END
    IF f != 1 DO
        print("FAIL")
    END
    IF l != 1 DO
        print("FAIL")
    END
    IF i != 1 DO
        print("FAIL")
    END
    IF h != 1 DO
        print("FAIL")
    END
    IF c != 1 DO
        print("FAIL")
    END
    IF ul != ul_one DO
        print("FAIL")
    END
    IF ui != 1 DO
        print("FAIL")
    END
    IF uh != 1 DO
        print("FAIL")
    END
    IF uc != 1 DO
        print("FAIL")
    END
END

DO
    Double d = -2 - 10
    Float f = -2 - 10

    Long l = -2 - 10
    Int i = -2 - 10
    Short h = -2 - 10
    Sbyte c = -2 - 10

    IF d != -12 DO
        print("FAIL")
    END
    IF f != -12 DO
        print("FAIL")
    END
    IF l != -12 DO
        print("FAIL")
    END
    IF i != -12 DO
        print("FAIL")
    END
    IF h != -12 DO
        print("FAIL")
    END
    IF c != -12 DO
        print("FAIL")
    END
END

DO
    Double d = -2. - 10.
    Float f = -2. - 10.

    IF d != -12. DO
        print("FAIL")
    END
    IF f != -12. DO
        print("FAIL")
    END
END

DO
    Double d = 10 - 5
    Float f = 10 - 5

    Long l = 10 - 5
    Int i = 10 - 5
    Short h = 10 - 5
    Sbyte c = 10 - 5

    Ulong ul_five = 5
    Ulong ul = 10 - 5
    Uint ui = 10 - 5
    Ushort uh = 10 - 5
    Byte uc = 10 - 5

    IF d != 5 DO
        print("FAIL")
    END
    IF f != 5 DO
        print("FAIL")
    END
    IF l != 5 DO
        print("FAIL")
    END
    IF i != 5 DO
        print("FAIL")
    END
    IF h != 5 DO
        print("FAIL")
    END
    IF c != 5 DO
        print("FAIL")
    END
    IF ul != ul_five DO
        print("FAIL")
    END
    IF ui != 5 DO
        print("FAIL")
    END
    IF uh != 5 DO
        print("FAIL")
    END
    IF uc != 5 DO
        print("FAIL")
    END
END

DO
    Double d = 2 * 3
    Float f = 2 * 3

    Long l = 2 * 3
    Int i = 2 * 3
    Short h = 2 * 3
    Sbyte c = 2 * 3

    Ulong ul_six = 6
    Ulong ul = 2 * 3
    Uint ui = 2 * 3
    Ushort uh = 2 * 3
    Byte uc = 2 * 3

    IF d != 6 DO
        print("FAIL")
    END
    IF f != 6 DO
        print("FAIL")
    END
    IF l != 6 DO
        print("FAIL")
    END
    IF i != 6 DO
        print("FAIL")
    END
    IF h != 6 DO
        print("FAIL")
    END
    IF c != 6 DO
        print("FAIL")
    END
    IF ul != ul_six DO
        print("FAIL")
    END
    IF ui != 6 DO
        print("FAIL")
    END
    IF uh != 6 DO
        print("FAIL")
    END
    IF uc != 6 DO
        print("FAIL")
    END
END

DO
    Double d = 7. % 3.
    Float f = 7. % 3.

    Long l = 7 % 3
    Int i = 7 % 3
    Short h = 7 % 3
    Sbyte c = 7 % 3

    Ulong ul_one = 1
    Ulong ul = 7 % 3
    Uint ui = 7 % 3
    Ushort uh = 7 % 3
    Byte uc = 7 % 3

    IF d != 1 DO
        print("FAIL")
    END
    IF f != 1 DO
        print("FAIL")
    END
    IF l != 1 DO
        print("FAIL")
    END
    IF i != 1 DO
        print("FAIL")
    END
    IF h != 1 DO
        print("FAIL")
    END
    IF c != 1 DO
        print("FAIL")
    END
    IF ul != ul_one DO
        print("FAIL")
    END
    IF ui != 1 DO
        print("FAIL")
    END
    IF uh != 1 DO
        print("FAIL")
    END
    IF uc != 1 DO
        print("FAIL")
    END
END

DO
    Double d = 7 // 6
    Float f = 7 // 6
    Double dd = 7. // 6
    Float ff = 7. // 6

    Long l = 7 // 6
    Int i = 7 // 6
    Short h = 7 // 6
    Sbyte c = 7 // 6

    Ulong ul_one = 1
    Ulong ul = 7 // 6
    Uint ui = 7 // 6
    Ushort uh = 7 // 6
    Byte uc = 7 // 6

    IF d != 1 DO
        print("FAIL")
    END
    IF f != 1 DO
        print("FAIL")
    END
    IF dd != 1 DO
        print("FAIL")
    END
    /* Note: No cast support yet. Or Float literals */
    IF ff != 1 DO
        print("FAIL")
    END


    IF l != 1 DO
        print("FAIL")
    END
    IF i != 1 DO
        print("FAIL")
    END
    IF h != 1 DO
        print("FAIL")
    END
    IF c != 1 DO
        print("FAIL")
    END
    IF ul != ul_one DO
        print("FAIL")
    END
    IF ui != 1 DO
        print("FAIL")
    END
    IF uh != 1 DO
        print("FAIL")
    END
    IF uc != 1 DO
        print("FAIL")
    END
END

DO
    Double d = 7 / 6
    Float f = 7 / 6
    Double dd = 7. / 6
    Float ff = 7. / 6

    Long l = 7 / 6
    Int i = 7 / 6
    Short h = 7 / 6
    Sbyte c = 7 / 6

    Ulong ul_one = 1
    Ulong ul = 7 / 6
    Uint ui = 7 / 6
    Ushort uh = 7 / 6
    Byte uc = 7 / 6

    IF d != 1.1666666666666666666666666666667 DO
        print("FAIL")
    END
    /* Note: No cast support yet. Or Float literals */
    IF f != 1.16666662693023681640625 DO
        print("FAIL")
    END
    IF dd != 1.1666666666666666666666666666667 DO
        print("FAIL")
    END
    /* Note: No cast support yet. Or Float literals */
    IF ff != 1.16666662693023681640625 DO
        print("FAIL")
    END


    IF l != 1 DO
        print("FAIL")
    END
    IF i != 1 DO
        print("FAIL")
    END
    IF h != 1 DO
        print("FAIL")
    END
    IF c != 1 DO
        print("FAIL")
    END
    IF ul != ul_one DO
        print("FAIL")
    END
    IF ui != 1 DO
        print("FAIL")
    END
    IF uh != 1 DO
        print("FAIL")
    END
    IF uc != 1 DO
        print("FAIL")
    END
END

/* Floating point const expressions */

DO
    Float f = 2. + 3.
    Double d = 2. + 3.
    IF d != 5. DO
        print("FAIL")
    END
    IF f != 5. DO
        print("FAIL")
    END
END
DO
    Float f = 2. + 3
    Double d = 2. + 3
    IF d != 5. DO
        print("FAIL")
    END
    IF f != 5. DO
        print("FAIL")
    END
END
DO
    Float f = 2 + 3.
    Double d = 2 + 3.
    IF d != 5. DO
        print("FAIL")
    END
    IF f != 5. DO
        print("FAIL")
    END
END
DO
    Float f = 2. + 3.
    Double d = 2. + 3.
    IF d != 5 DO
        print("FAIL")
    END
    IF f != 5 DO
        print("FAIL")
    END
END
DO
    Float f = 2. + 3
    Double d = 2. + 3
    IF d != 5 DO
        print("FAIL")
    END
    IF f != 5 DO
        print("FAIL")
    END
END
DO
    Float f = 2 + 3.
    Double d = 2 + 3.
    IF d != 5 DO
        print("FAIL")
    END
    IF f != 5 DO
        print("FAIL")
    END
END


DO
    Float f = 2. + 3. * 2. / 2.
    Double d = 2. + 3. * 2. / 2.
    IF d != 5 DO
        print("FAIL")
    END
    IF f != 5 DO
        print("FAIL")
    END
    IF d != 5. DO
        print("FAIL")
    END
    IF f != 5. DO
        print("FAIL")
    END
    IF d != 2. + 3. DO
        print("FAIL")
    END
    IF f != 2. + 3. DO
        print("FAIL")
    END
END

print("DONE")
