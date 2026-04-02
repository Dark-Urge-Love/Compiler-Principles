fn main() {
    // 关键字与标识符
    let mut decimal_num = 1_000_000;
    
    // 不同的整数进制字面量
    let hex_num = 0x1A3F_CDEF; // 十六进制
    let oct_num = 0o775;       // 八进制
    let bin_num = 0b1010_1100; // 二进制
    
    // 浮点数与字符串字面量
    let pi_approx = 3.14159_26; 
    let msg = "Hello, Rust Lexer!"; 

    // 宏调用
    println!("Message: {}", msg);

    // 操作符与赋值操作符
    decimal_num += 5;
    decimal_num -= 1;
    decimal_num *= 2;
    decimal_num /= 2;
    decimal_num %= 100;

    let is_equal = (decimal_num == 42) || (hex_num != 0) && (oct_num < 1000);
    
    if is_equal {
        // 分隔符测试
        for _ in 0..10 {
            let arr = [1, 2, 3];
            let item = arr[0];
        }
    } else {
        loop {
            break;
        }
    }
}
