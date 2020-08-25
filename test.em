
Double global_double = 2

FUNC Double avg = avg(a,b) DO
    RETURN (a+b)/2
END

FUNC Int sign = sign(a) DO
    IF a > 0 DO
        RETURN 1
    ELSE IF a < 0 DO
        RETURN -1
    ELSE DO
        RETURN 0
    END
END

FUNC Int test = test(a) DO
    IF a > 0 DO
        RETURN 2
    END
    RETURN 3
END

avg(4,5)
sign(-2)
sign(2)
sign(0)
test(0)
test(1)



