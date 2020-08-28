
FUNC printnl_int(Int i)
FUNC printnl_double(Double d)

FUNC Double c = sin(Double d)
FUNC putchar(Int c)
FUNC Double result = log10(Double x)

Double pi = 3.14159265358979323846
Double e = 2.7182818284590452354

FUNC test2() DO
    printnl_int(3)
    RETURN
END

FUNC Int i = test3() DO
    RETURN 2
END

FUNC Double int = integrate_sin() DO
    Double sum = 0
    Double dx = 1/500.
    Int i = 0
    WHILE i < 500 DO
        Double x = i*2*pi/1000
        sum = sum + sin(x) * dx
        i = i + 1
    END
    RETURN sum
END
/* putchar(74) */
log10(100)
sin(pi/2)

integrate_sin()
