
USING IMPORT Std.Io

Int a = 1
IF a != 1 DO
    print("FAIL")
END
Int i
DO
    Int a = 2
    IF a != 2 DO
        print("FAIL")
    END
    i = i + 1
    DO 
        Int a = 3
        IF a != 3 DO
            print("FAIL")
        END
        i = i + 1
    END
END

IF i != 2 DO
    print("FAIL")
END

FUNC foo() DO
    Int a = 1
    IF a != 1 DO
        print("FAIL")
    END
    Int i
    DO
        Int a = 2
        IF a != 2 DO
            print("FAIL")
        END
        i = i + 1
        DO 
            Int a = 3
            IF a != 3 DO
                print("FAIL")
            END
            i = i + 1
        END
    END

    IF i != 2 DO
        print("FAIL")
    END
END
foo()

print("DONE")
