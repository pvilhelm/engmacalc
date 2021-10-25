
USING IMPORT Std.Io

Int true_0 = 1
Int true_1 = 1
Int false_0 = 0
Int false_1 = 0

IF true_0 DO

ELSE DO
	print("FAIL")
END

/* Test AND */

IF 1 AND 1 DO

ELSE DO
	print("FAIL")
END

IF 1 AND 0 DO
	print("FAIL")
ELSE DO
	
END

IF 0 AND 0 DO
	print("FAIL")
ELSE DO
	
END

IF 0 AND 1 DO
	print("FAIL")
ELSE DO
	
END

IF false_0 AND true_1 DO
	print("FAIL")
ELSE DO
	
END

/* Test OR */

IF 1 OR 0 DO

ELSE DO
	print("FAIL")
END

IF 0 OR 1 DO
	
ELSE DO
	print("FAIL")
END

IF 0 OR 0 DO
	print("FAIL")
ELSE DO
	
END

IF 1 OR 1 DO
	
ELSE DO
	print("FAIL")
END

IF false_0 OR true_1 DO
	
ELSE DO
	print("FAIL")
END

/* Test XOR */

IF 1 XOR 0 DO

ELSE DO
	print("FAIL")
END

IF 0 XOR 1 DO
	
ELSE DO
	print("FAIL")
END

IF 0 XOR 0 DO
	print("FAIL")
ELSE DO
	
END

IF 1 XOR 1 DO
	print("FAIL")
ELSE DO
	
END

IF false_0 OR true_1 DO
	
ELSE DO
	print("FAIL")
END

/* Test NAND */

IF 1 NAND 0 DO

ELSE DO
	print("FAIL")
END

IF 0 NAND 1 DO
	
ELSE DO
	print("FAIL")
END

IF 0 NAND 0 DO
	
ELSE DO
	print("FAIL")
END

IF 1 NAND 1 DO
	print("FAIL")
ELSE DO
	
END

IF false_0 OR true_1 DO
	
ELSE DO
	print("FAIL")
END

/* Test NOR */

IF 1 NOR 0 DO
	print("FAIL")
ELSE DO
	
END

IF 0 NOR 1 DO
	print("FAIL")
ELSE DO
	
END

IF 0 NOR 0 DO
	
ELSE DO
	print("FAIL")
END

IF 1 NOR 1 DO
	print("FAIL")
ELSE DO
	
END

IF false_0 NOR true_1 DO
	print("FAIL")
ELSE DO
	
END

print("DONE")
