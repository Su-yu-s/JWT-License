package com.example.license.util;

import javax.crypto.Cipher;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Base64;

/**
 * 授权 Key 加密工具类
 * 使用 AES-256-GCM 加密存储明文 Key，防止数据库泄露后明文暴露
 */
public class KeyEncryptionUtil {

    private static final String ALGORITHM = "AES";
    private static final String TRANSFORMATION = "AES/GCM/NoPadding";
    private static final int GCM_TAG_LENGTH = 128;
    private static final int NONCE_LENGTH = 12;

    /**
     * 从配置密钥派生 AES 密钥（SHA-256 哈希）
     */
    private static SecretKeySpec deriveKey(String masterSecret) throws Exception {
        MessageDigest sha256 = MessageDigest.getInstance("SHA-256");
        byte[] keyBytes = sha256.digest(masterSecret.getBytes(StandardCharsets.UTF_8));
        return new SecretKeySpec(keyBytes, ALGORITHM);
    }

    /**
     * 加密明文
     */
    public static String encrypt(String masterSecret, String plaintext) throws Exception {
        Cipher cipher = Cipher.getInstance(TRANSFORMATION);
        SecretKeySpec keySpec = deriveKey(masterSecret);

        byte[] nonce = new byte[NONCE_LENGTH];
        java.security.SecureRandom.getInstanceStrong().nextBytes(nonce);

        cipher.init(Cipher.ENCRYPT_MODE, keySpec, new GCMParameterSpec(GCM_TAG_LENGTH, nonce));
        byte[] encrypted = cipher.doFinal(plaintext.getBytes(StandardCharsets.UTF_8));

        // 格式: nonce(12) || encrypted
        byte[] combined = new byte[nonce.length + encrypted.length];
        System.arraycopy(nonce, 0, combined, 0, nonce.length);
        System.arraycopy(encrypted, 0, combined, nonce.length, encrypted.length);

        return Base64.getEncoder().encodeToString(combined);
    }

    /**
     * 解密密文
     */
    public static String decrypt(String masterSecret, String ciphertext) throws Exception {
        byte[] combined = Base64.getDecoder().decode(ciphertext);
        byte[] nonce = new byte[NONCE_LENGTH];
        byte[] encrypted = new byte[combined.length - NONCE_LENGTH];
        System.arraycopy(combined, 0, nonce, 0, NONCE_LENGTH);
        System.arraycopy(combined, NONCE_LENGTH, encrypted, 0, encrypted.length);

        Cipher cipher = Cipher.getInstance(TRANSFORMATION);
        SecretKeySpec keySpec = deriveKey(masterSecret);
        cipher.init(Cipher.DECRYPT_MODE, keySpec, new GCMParameterSpec(GCM_TAG_LENGTH, nonce));

        byte[] decrypted = cipher.doFinal(encrypted);
        return new String(decrypted, StandardCharsets.UTF_8);
    }
}
