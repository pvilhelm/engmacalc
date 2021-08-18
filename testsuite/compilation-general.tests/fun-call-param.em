/*
    Check that function argument and parameters work properly.
    
    Tests parameter order.
    Tests overloading.
    Tests pointer parameters.
*/

USING IMPORT Std.Io

FUNC no_param() DO
    println(0)
END

FUNC one_long(Long i) DO
    println(i)
END

FUNC two_longs(Long i1, Long i2) DO
    print(i1)
    print(" ")
    println(i2)
END

FUNC one_int(Int i) DO
    println(i)
END

FUNC two_ints(Int i1, Int i2) DO
    print(i1)
    print(" ")
    println(i2)
END

FUNC one_short(Short i) DO
    println(i)
END

FUNC two_shorts(Short i1, Short i2) DO
    print(i1)
    print(" ")
    println(i2)
END

FUNC one_sbyte(Sbyte i) DO
    println(i)
END

FUNC two_sbyte(Sbyte i1, Sbyte i2) DO
    print(i1)
    print(" ")
    println(i2)
END

FUNC one_ulong(Ulong i) DO
    println(i)
END

FUNC two_ulongs(Ulong i1, Ulong i2) DO
    print(i1)
    print(" ")
    println(i2)
END

FUNC one_uint(Uint i) DO
    println(i)
END

FUNC two_uints(Uint i1, Uint i2) DO
    print(i1)
    print(" ")
    println(i2)
END

FUNC one_ushort(Ushort i) DO
    println(i)
END

FUNC two_ushorts(Ushort i1, Ushort i2) DO
    print(i1)
    print(" ")
    println(i2)
END

FUNC one_usbyte(Byte i) DO
    println(i)
END

FUNC two_usbyte(Byte i1, Byte i2) DO
    print(i1)
    print(" ")
    println(i2)
END

FUNC one_ptrto_long(&Long i) DO
    println(@i)
END

FUNC two_ptrto_longs(&Long i1, &Long i2) DO
    print(@i1)
    print(" ")
    println(@i2)
END

FUNC one_ptrto_int(&Int i) DO
    println(@i)
END

FUNC two_ptrto_ints(&Int i1, &Int i2) DO
    print(@i1)
    print(" ")
    println(@i2)
END

FUNC one_ptrto_short(&Short i) DO
    println(@i)
END

FUNC two_ptrto_shorts(&Short i1, &Short i2) DO
    print(@i1)
    print(" ")
    println(@i2)
END

FUNC one_ptrto_sbyte(&Sbyte i) DO
    println(@i)
END

FUNC two_ptrto_sbyte(&Sbyte i1, &Sbyte i2) DO
    print(@i1)
    print(" ")
    println(@i2)
END

FUNC one_ptrto_ulong(&Ulong i) DO
    println(@i)
END

FUNC two_ptrto_ulongs(&Ulong i1, &Ulong i2) DO
    print(@i1)
    print(" ")
    println(@i2)
END

FUNC one_ptrto_uint(&Uint i) DO
    println(@i)
END

FUNC two_ptrto_uints(&Uint i1, &Uint i2) DO
    print(@i1)
    print(" ")
    println(@i2)
END

FUNC one_ptrto_ushort(&Ushort i) DO
    println(@i)
END

FUNC two_ptrto_ushorts(&Ushort i1, &Ushort i2) DO
    print(@i1)
    print(" ")
    println(@i2)
END

FUNC one_ptrto_usbyte(&Byte i) DO
    println(@i)
END

FUNC two_ptrto_usbyte(&Byte i1, &Byte i2) DO
    print(@i1)
    print(" ")
    println(@i2)
END

FUNC one_long_one_int(Long l, Int i) DO
    print(l)
    print(" ")
    println(i)
END

FUNC one_int_one_long(Int i, Long l) DO
    print(i)
    print(" ")
    println(l)
END

Long l1 = -4
Int i1 = -3
Short h1 = -2
Sbyte c1 = -1
Ulong ul1 = 4
Uint ui1 = 3
Ushort uh1 = 2
Byte uc1 = 1

Long l2 = -44
Int i2 = -33
Short h2 = -22
Sbyte c2 = -11
Ulong ul2 = 44
Uint ui2 = 33
Ushort uh2 = 22
Byte uc2 = 11

no_param()   //0
one_long(l1) //-4
two_longs(l1, l2) //-4 -44
one_int(i1) //-3
two_ints(i1, i2) //-3 -33
one_short(h1) //-2
two_shorts(h1, h2) //-2 -22
one_sbyte(c1) //-1
two_sbyte(c1, c2) //-1 -11

one_ulong(ul1) //4
two_ulongs(ul1, ul2) //4 44
one_uint(ui1) //3
two_uints(ui1, ui2) //3 33
one_ushort(uh1) //2
two_ushorts(uh1, uh2) //2 22
one_usbyte(uc1) //1
two_usbyte(uc1, uc2) //1 11

// Samma utskrifter som ovan
one_ptrto_long(&l1)
two_ptrto_longs(&l1, &l2)
one_ptrto_int(&i1)
two_ptrto_ints(&i1, &i2)
one_ptrto_short(&h1)
two_ptrto_shorts(&h1, &h2)
one_ptrto_sbyte(&c1)
two_ptrto_sbyte(&c1, &c2)

one_ptrto_ulong(&ul1)
two_ptrto_ulongs(&ul1, &ul2)
one_ptrto_uint(&ui1)
two_ptrto_uints(&ui1, &ui2)
one_ptrto_ushort(&uh1)
two_ptrto_ushorts(&uh1, &uh2)
one_ptrto_usbyte(&uc1)
two_ptrto_usbyte(&uc1, &uc2)

// Misc
one_long_one_int(l1, i1) //-4 -3
one_int_one_long(i1, l1) //-3 -4
