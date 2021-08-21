
USING IMPORT Std.Io

FUNC Int i = test1() DO
    IF 1 DO
        RETURN 1
    END

    RETURN 0
END

IF test1() != 1 DO
    print("FAIL")
END

FUNC Int i = test2() DO
    IF 0 DO
        RETURN 0
    ELSE IF 0 DO

    ELSE IF 0 DO
        RETURN 2
    ELSE DO
        RETURN 1
    END

    RETURN 0
END
IF test2() != 1 DO
    print("FAIL")
END

FUNC Int i = test3() DO
    IF 0 DO
        RETURN 0
    ELSE IF 0 DO

    ELSE IF 0 DO
        RETURN 2
    ELSE DO
         
    END

    RETURN 1
END
IF test3() != 1 DO
    print("FAIL")
END

FUNC Int i = test4() DO
    IF 1 DO
        RETURN 1
    ELSE DO
        RETURN 2
    END
END

IF test4() != 1 DO
    print("FAIL")
END

FUNC Int i = test5() DO
    IF 1 DO
        RETURN 1
    ELSE IF 1 DO
        RETURN 2
    ELSE IF 1 DO
        RETURN 2
    ELSE DO
        RETURN 2
    END
END

IF test5() != 1 DO
    print("FAIL")
END

FUNC Int i = test6() DO
    IF 1 DO
        IF 0 DO
            RETURN 0
        ELSE DO
            RETURN 1
        END
    ELSE IF 1 DO
        RETURN 2
    ELSE IF 1 DO
        RETURN 2
    ELSE DO
        RETURN 2
    END
END

IF test6() != 1 DO
    print("FAIL")
END

FUNC Int i = test7() DO
    IF 0 DO
        IF 0 DO
            RETURN 0
        ELSE DO
            RETURN 1
        END
    ELSE IF 0 DO
        RETURN 2
    ELSE IF 0 DO
        RETURN 2
    ELSE DO
        
    END

    RETURN 11
END

IF test7() != 11 DO
    print("FAIL")
END

FUNC Int i = test8() DO
    IF 0 DO
        IF 0 DO
            RETURN 0
        ELSE DO
            
        END
    ELSE IF 0 DO
        RETURN 2
    ELSE IF 0 DO
        RETURN 2
    END

    RETURN 11
END

IF test8() != 11 DO
    print("FAIL")
END

FUNC Int i = test9() DO
    IF 1 DO
        
    ELSE DO
        RETURN 2
    ALSO DO
        RETURN 11
    END
END
IF test9() != 11 DO
    print("FAIL")
END

FUNC Int i = test10() DO
    IF 0 DO

    ELSE DO
        RETURN 11
    ALSO DO
        
    END

    RETURN 1
END
IF test10() != 11 DO
    print("FAIL")
END


print("DONE")
