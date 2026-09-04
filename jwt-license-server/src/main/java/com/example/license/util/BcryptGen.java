package com.example.license.util;

import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;

/**
 * 一次性工具：生成 BCrypt 密码哈希
 * 用法：mvn exec:java -Dexec.mainClass="com.example.license.util.BcryptGen"
 */
public class BcryptGen {
    public static void main(String[] args) {
        String rawPassword = args.length > 0 ? args[0] : "1234";
        BCryptPasswordEncoder encoder = new BCryptPasswordEncoder();
        String hashed = encoder.encode(rawPassword);
        System.out.println("Plain: " + rawPassword);
        System.out.println("BCrypt: " + hashed);
        System.out.println("Match: " + encoder.matches(rawPassword, hashed));
    }
}
