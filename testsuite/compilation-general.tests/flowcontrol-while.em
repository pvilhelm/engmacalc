
USING IMPORT Std.Io

Int i = 0

WHILE i < 100 DO
    i = i + 1
END

IF i != 100 DO
    print("FAIL")
END

WHILE 0 DO
    print("FAIL")
END

FUNC foo() DO
    WHILE i < 100 DO
        i = i + 1
    END
    
    IF i != 100 DO
        print("FAIL")
    END
    
    WHILE 0 DO
        print("FAIL")
    END
END
foo()

print("DONE")
