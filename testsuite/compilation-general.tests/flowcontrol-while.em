
USING IMPORT Std.Io

Int i = 0

WHILE i < 10 DO
    i = i + 1
END

IF i != 10 DO
    print("FAIL")
END

WHILE 0 DO
    print("FAIL")
END

FUNC foo() DO
    WHILE i < 10 DO
        i = i + 1
    END
    
    IF i != 10 DO
        print("FAIL")
    END
    
    WHILE 0 DO
        print("FAIL")
    END
END
foo()

FUNC foo2() DO
    Int i
    WHILE 0 DO
        print("FAIL")
    ELSE DO
        i = 1
    END
    
    IF i != 1 DO
        print("FAIL")
    END
    
    i = 0
    WHILE i < 10 DO
        i = i + 1
    ELSE DO
        i = 1
    END
    IF i != 10 DO
        print("FAIL")
    END
END
foo2()

FUNC foo3() DO
    WHILE 0 DO
        RETURN
    ELSE DO
        RETURN
    END
END
foo3()

print("DONE")
