package com.example.license.util;

import java.security.SecureRandom;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

/**
 * 授权 Key 生成工具
 * 格式: XXXX-XXXX-XXXX-XXXX-XXXX (5组4字符，共20字符+4个分隔符)
 */
public class LicenseKeyUtil {

    private static final String CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    private static final int GROUP_SIZE = 4;
    private static final int GROUP_COUNT = 5;
    private static final SecureRandom RANDOM = new SecureRandom();

    /**
     * 生成授权 Key 字符串
     */
    public static String generate() {
        String body = IntStream.range(0, GROUP_SIZE * GROUP_COUNT)
                .mapToObj(i -> String.valueOf(CHARSET.charAt(RANDOM.nextInt(CHARSET.length()))))
                .collect(Collectors.joining());

        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < GROUP_COUNT; i++) {
            if (i > 0) sb.append('-');
            sb.append(body, i * GROUP_SIZE, (i + 1) * GROUP_SIZE);
        }
        return sb.toString();
    }

    /**
     * 生成短码（前两组）
     */
    public static String generateShort() {
        return IntStream.range(0, GROUP_SIZE * 2)
                .mapToObj(i -> String.valueOf(CHARSET.charAt(RANDOM.nextInt(CHARSET.length()))))
                .collect(Collectors.joining());
    }
}
