
USING IMPORT Std.Io

FUNC test1() DO
    println(2 + 2 * 3) /* 8 */
    println(2 - 2 * 3) /* -4 */
    println(2 + 2 // 2) /* 3 */
    println(2 - 2 // 2) /* 1 */
    println(2 * 3 + 2) /* 8 */
    println(-2 * 3 + 2) /* -4 */
    println(2 // 2 + 2) /* 3 */
    println(-2 // 2 + 2) /* 1 */
    println(4 // 2 * 3) /* 6 */
    println(4 * 2 // 3) /* 2 */
    println(2^2. + 2) /* 6 */
    println(2^2. * 2) /* 8 */
    println(2^2. / 2) /* 2 */
    println(2^2. - 2) /* 2 */
    println(2 + 2^2.) /* 6 */
    println(2 * 2^2.) /* 8 */
    println(2^2. / 2) /* 2 */
    println(2^2. - 2) /* 2 */
    println(4 % 3 + 1)/* 2 */
    println(4 % 3 - 1)/* 0 */
END

test1()
