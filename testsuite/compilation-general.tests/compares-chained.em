/* 
    Verify that chained compares works.

    Should not print any "FAIL" and print one "DONE" in the end
*/

USING IMPORT Std.Io

FUNC chain_1() DO
    Int a = 3
    IF 2 < a < 4 DO
        
    ELSE DO
        print("FAIL")
    END
END
chain_1()

FUNC chain_2() DO
    Int a = 4
    IF 2 < a < 4 DO
        print("FAIL")
    ELSE DO
        
    END
END
chain_2()

FUNC chain_3() DO
    Int a = 4
    IF 2 <= a <= 4 DO
        
    ELSE DO
        print("FAIL")
    END
END
chain_3()

FUNC chain_4() DO
    Int a = 4
    IF 5 > a > 2 DO
        
    ELSE DO
        print("FAIL")
    END
END
chain_4()

FUNC chain_5() DO
    Int a = 5
    IF 5 >= a >= 2 DO
        
    ELSE DO
        print("FAIL")
    END
END
chain_5()

FUNC chain_6() DO
    Int a = 6
    IF 5 >= a >= 2 DO
        print("FAIL")
    ELSE DO
        
    END
END
chain_6()

FUNC chain_7() DO
    Int a = 1
    IF 5 >= a <= 2 DO

    ELSE DO
        print("FAIL")
    END
END
chain_7()

/* Verify that the terms are only evaluated once */
Int b = 0
FUNC Int b = foo() DO
    b = b + 1
    RETURN b
END

FUNC chain_8() DO
    IF 0 < foo() < 2 DO

    ELSE DO
        print("FAIL")
    END

    IF foo() != 2 DO
        print("FAIL")
    END
END
chain_8()

FUNC chain_9() DO
    Int a = 1
    IF 1 == a == 1 DO

    ELSE DO
        print("FAIL")
    END

    IF 0 != a != 2 DO

    ELSE DO
        print("FAIL")
    END
END
chain_9()

FUNC chain_10() DO
    Int one = 1
    Int two = 2
    Int three = 3

    IF one == two == two DO
        print("FAIL")
    END
    IF one > two < three DO
        print("FAIL")
    END
    IF two < three < one DO
        print("FAIL")
    END
    IF three > one > two DO
        print("FAIL")
    END
    IF three > one < two DO
    
    ELSE DO
        print("FAIL")
    END
END
chain_10()


FUNC chain_11() DO
    IF 1 < 2 < 3 < 4 < 5 < 6 < 7 < 8 < 9 < 10 DO

    ELSE DO
        print("FAIL")
    END
END
chain_11()

FUNC chain_12() DO
    IF 1 < 2 < 3 < 4 > 2 > 1 <= 3 > 2 > 1 > -1 <= 10 == 10 != 11 DO

    ELSE DO
        print("FAIL")
    END
END
chain_12()

print("DONE")
