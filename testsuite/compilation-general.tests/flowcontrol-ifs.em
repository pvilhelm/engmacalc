
USING IMPORT Std.Io

IF 0 DO
    print("FAIL")
ELSE DO
    print("ELSE-WORKS")
END

IF 1 DO
    print("IF-WORKS")
ELSE DO
    print("FAIL")
END

IF 0 DO
    print("FAIL")
ELSE IF 0 DO
    print("FAIL")
ELSE IF 1 DO
    print("ELSE-IF-WORKS")
ELSE DO
    print("FAIL")
END

IF 0 DO
    print("FAIL")
ELSE IF 0 DO
    print("FAIL")
ELSE IF 0 DO
    print("FAIL")
ELSE DO
    print("ELSE-IF'S-ELSE-WORKS")
END

FUNC ifs1() DO
    Int i = 0
    IF 0 DO
        print("FAIL")
    ELSE DO
        i = 1
    END
    IF i == 0 DO
        print("FAIL")
    END
    i = 0

    IF 1 DO
        i = 1
    ELSE DO
        print("FAIL")
    END
    IF i == 0 DO
        print("FAIL")
    END
    i = 0

    IF 0 DO
        print("FAIL")
    ELSE IF 0 DO
        print("FAIL")
    ELSE IF 1 DO
        i = 1
    ELSE DO
        print("FAIL")
    END
    IF i == 0 DO
        print("FAIL")
    END
    i = 0

    IF 0 DO
        print("FAIL")
    ELSE IF 0 DO
        print("FAIL")
    ELSE IF 0 DO
        print("FAIL")
    ELSE DO
        i = 1
    END
    IF i == 0 DO
        print("FAIL")
    END
    i = 0

    IF 0 DO
        print("FAIL")
    ELSE IF 0 DO
        print("FAIL")
    ELSE IF 1 DO
        i = 1
    ELSE DO
        print("FAIL")
    END
    IF i == 0 DO
        print("FAIL")
    END
    i = 0
END
ifs1()


FUNC alsos1() DO
    Int i = 0

    IF 0 DO
        print("FAIL")
    ALSO DO
        print("FAIL")
    END

    IF 0 DO
        print("FAIL")
    ELSE IF 0 DO
        print("FAIL")
    ALSO DO
        print("FAIL")
    END

    IF 1 DO
        i = i + 1
    ALSO DO
        IF i != 1 DO
            print("FAIL")
        END
        i = i + 1
    ELSE DO
        print("FAIL")
    END
    IF i != 2 DO
        print("FAIL")
    END
    i = 0

    IF 1 DO
        i = i + 1
    ALSO DO
        IF i != 1 DO
            print("FAIL")
        END
        i = i + 1
    END
    IF i != 2 DO
        print("FAIL")
    END
    i = 0


    IF 0 DO
        print("FAIL")
    ELSE IF 0 DO
        print("FAIL")
    ELSE IF 1 DO
        i = 1 + i
    ALSO DO
        IF i != 1 DO
            print("FAIL")
        END
        i = 1 + i
    ELSE DO
        print("FAIL")
    END
    IF i != 2 DO
        print("FAIL")
    END
    i = 0

    IF 0 DO
        print("FAIL")
    ELSE IF 0 DO
        print("FAIL")
    ELSE IF 0 DO
        print("FAIL")
    ALSO DO
        print("FAIL")
    ELSE DO
       i = 1 
    END
    IF i != 1 DO
        print("FAIL")
    END
    i = 0
END
alsos1()

/* Test scope */
FUNC ifs2() DO
    Int i = 333

    IF 1 DO
        Int i = 222
        IF i != 222 DO
            print("FAIL")
        END
    END
END
ifs2()

print("DONE")
