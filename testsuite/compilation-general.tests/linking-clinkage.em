
FUNC Int i = c::importedfromc_returns_int_one()

FUNC Int i = c::importedfromc_returns_int_plus_one(Int i)

FUNC Double d = c::importedfromc_returns_doubles_summed(Double d1, Double d2)

FUNC Int i = c::exportedtoc_returns_int_one() DO
    RETURN 1
END

FUNC Int i = c::exportedtoc_returns_int_plus_one(Int i) DO
    RETURN i + 1
END

FUNC Double d = c::exportedtoc_returns_doubles_summed(Double d1, Double d2) DO
    RETURN d1 + d2
END

FUNC Int i = c::exportedbacktoc_returns_int_one() DO
    RETURN importedfromc_returns_int_one()
END

FUNC Int i = c::exportedbacktoc_returns_int_plus_one(Int i) DO
    RETURN importedfromc_returns_int_plus_one(i)
END

FUNC Double d = c::exportedbacktoc_returns_doubles_summed(Double d1, Double d2) DO
    RETURN importedfromc_returns_doubles_summed(d1, d2)
END


